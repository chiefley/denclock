import test from 'node:test';
import assert from 'node:assert/strict';
import { parseRadarMetadata, radarLayout, radarTileUrl, RadarTiles, RadarAnimation, RadarView } from '../radar.js';
const latest=Date.parse('2026-11-03T12:00Z')/1000;
const metadata=()=>({host:'https://tilecache.rainviewer.com',radar:{past:Array.from({length:13},(_,i)=>({time:latest-(12-i)*600,path:`/v2/radar/hash-${i}`})),nowcast:[{time:latest+600,path:'/v2/radar/future'}]}});
const settle=()=>new Promise(r=>setImmediate(r));

test('metadata uses dynamic host and opaque paths, chronological past only, at most 13 frames',()=>{
 const d=metadata(); d.host='https://other.rainviewer.com/'; d.radar.past.reverse();
 d.radar.past.push(d.radar.past[0],{time:'bad',path:'/v2/radar/bad'},{time:latest,path:'//evil.test/path'});
 const parsed=parseRadarMetadata(d);
 assert.equal(parsed.host,'https://other.rainviewer.com');assert.equal(parsed.frames.length,13);
 assert.equal(parsed.frames.at(-1).time,latest);
 const url=radarTileUrl(parsed,parsed.frames[0],{x:3,y:4});
 assert.equal(url,'https://other.rainviewer.com/v2/radar/hash-0/256/7/3/4/2/1_1.png');
 assert.ok(!JSON.stringify(parsed).includes('future'));
 for(const bad of [{},{host:'http://insecure.test',radar:{past:[]}},{...metadata(),radar:{past:[]}}]) assert.throws(()=>parseRadarMetadata(bad));
});

test('regional map adapts to size, wraps dateline, clamps poles and keeps bounded tile requests',()=>{
 for(const [width,height] of [[930,273],[1450,600],[600,1000]]) {
  const l=radarLayout({latitude:41,longitude:-81},width,height);
  assert.ok(l.tiles.length<=9);assert.equal(l.width,width);assert.equal(l.height,height);
  assert.ok(l.tiles.every(t=>t.x>=0&&t.x<128&&t.y>=0&&t.y<128));
 }
 for(const longitude of [-180,180]) assert.ok(radarLayout({latitude:90,longitude},930,300).tiles.every(t=>t.x>=0&&t.x<128));
 assert.equal(radarLayout({latitude:NaN,longitude:0},900,300),null);
 assert.equal(radarLayout({latitude:0,longitude:0},0,300),null);
});

function scheduler() {
 let time=0,id=0;const tasks=new Map();
 return {now:()=>time,schedule:(fn,delay)=>{tasks.set(++id,{fn,delay});return id;},cancel:id=>tasks.delete(id),tasks,
  next:()=>{const [id,{fn,delay}]=tasks.entries().next().value;tasks.delete(id);time+=delay;fn();}};
}

test('animation renders two chronological cycles then holds newest; replay and stop release timers',()=>{
 const timer=scheduler(),drawn=[];const a=new RadarAnimation(f=>drawn.push(f),timer);
 a.play([1,2,3]);while(timer.tasks.size) timer.next();
 assert.deepEqual(drawn,[1,2,3,1,2,3]);assert.equal(a.timer,null);
 a.play([1,2,3]);assert.equal(drawn.at(-1),1);a.stop();assert.equal(timer.tasks.size,0);
 a.play([9]);assert.equal(drawn.at(-1),9);assert.equal(timer.tasks.size,0);
});

test('tile cache deduplicates, throttles starts and respects failure cooldown and disposal',async()=>{
 const timer=scheduler(),starts=[];
 const tiles=new RadarTiles({...timer,load:async url=>{starts.push([url,timer.now()]);if(url==='bad')throw Error('offline');return url;}});
 assert.equal(await tiles.get('one'),'one');await settle();
 assert.equal(await tiles.get('one'),'one');assert.equal(starts.length,1);
 const second=tiles.get('two');timer.next();assert.equal(await second,'two');await settle();
 assert.ok(starts[1][1]-starts[0][1]>=800);
 const bad=tiles.get('bad');const rejected=assert.rejects(bad);timer.next();await rejected;await settle();
 await assert.rejects(tiles.get('bad'));assert.equal(starts.filter(s=>s[0]==='bad').length,1);
 const cancelled=tiles.get('cancelled',()=>false);await assert.rejects(cancelled);
 const pending=tiles.get('later');const cancelledLater=assert.rejects(pending);tiles.dispose();await cancelledLater;
 assert.equal(timer.tasks.size,0);assert.equal(tiles.cache.size,0);
});

function fakeRoot() {
 const nodes=new Map();const observer={observe:()=>{},disconnect:()=>{observer.closed=true;}};
 const canvas=()=>({width:0,height:0,hidden:false,getContext:()=>({scale:()=>{},drawImage:()=>{},clearRect:()=>{}})});
 for(const id of ['radar-view','radar-base','radar-image','radar-home','radar-status','radar-time'])
  nodes.set(id,{...canvas(),textContent:'',clientWidth:930,clientHeight:273});
 return {hidden:false,getElementById:id=>nodes.get(id),createElement:()=>canvas(),observer};
}
function makeView() {
 const root=fakeRoot(), urls=[],state={mode:'ok',data:metadata(),now:0},timer=scheduler();
 const tiles={get:async(url,valid)=>{assert.ok(valid());urls.push(url);if(state.mode==='tiles-fail')throw Error('offline');return {};},dispose:()=>{state.disposed=true;}};
 const original=globalThis.setInterval;let interval;
 globalThis.setInterval=fn=>{interval=fn;return null;};
 let view;
 try{view=new RadarView(root,{tiles,observe:callback=>{root.resize=callback;return root.observer;},now:()=>state.now,
  fetchData:async()=>{if(state.mode==='metadata-fail')throw Error('offline');return {ok:true,json:async()=>state.data};}});}finally{globalThis.setInterval=original;}
 view.animation=new RadarAnimation(f=>view.draw(f),timer);
 const update=(visible=true,location={latitude:41,longitude:-81})=>view.update({visible,location,utc:latest*1000,timezone:'America/New_York'});
 return {root,view,urls,state,timer,update,interval};
}

test('radar loads only when visible, caches replay, preserves data on failure and expires old imagery',async()=>{
 const f=makeView();await settle();assert.equal(f.urls.length,0);
 f.update();await settle();assert.equal(f.view.frames.length,13);assert.equal(f.view.loading,false);
 assert.equal(f.root.getElementById('radar-image').hidden,false);
 assert.ok(f.root.getElementById('radar-time').textContent.includes('AM'));
 const count=f.urls.length;
 f.update(false);assert.equal(f.timer.tasks.size,0);
 f.update();await settle();assert.equal(f.urls.length,count);assert.equal(f.view.displayed.time,latest-7200);
 while(f.timer.tasks.size)f.timer.next();assert.equal(f.view.displayed.time,latest);
 f.state.mode='metadata-fail';await f.view.refresh();f.update();
 assert.match(f.root.getElementById('radar-status').textContent,/showing last available imagery/);
 f.view.update({visible:true,location:{latitude:41,longitude:-81},utc:(latest+1800)*1000});
 assert.equal(f.root.getElementById('radar-image').hidden,true);
 assert.match(f.root.getElementById('radar-status').textContent,/expired/);
 f.view.dispose();assert.equal(f.timer.tasks.size,0);assert.ok(f.state.disposed&&f.root.observer.closed);
});

test('location/resize rebuild the map and new metadata recovers from expired or missing tiles',async()=>{
 const f=makeView();await settle();f.update();await settle();
 f.root.getElementById('radar-view').clientWidth=1200;f.root.resize();await settle();
 assert.match(f.view.readyKey,/1200\/273/);
 f.update(true,{latitude:40,longitude:-80});await settle();assert.match(f.view.readyKey,/40,-80/);
 const before=f.view.frames;
 f.state.mode='tiles-fail';f.state.data.radar.past=f.state.data.radar.past.map(frame=>({...frame,path:frame.path+'-new'}));
 await f.view.refresh();await settle();assert.equal(f.view.frames,before);
 assert.match(f.root.getElementById('radar-status').textContent,/showing last available/);
 f.state.mode='ok';await f.view.refresh();await settle();assert.notEqual(f.view.frames,before);
 f.view.dispose();
});

test('fresh metadata replaces expired imagery; stale frames are identified even with refreshed metadata',async()=>{
 const f=makeView();await settle();f.update();await settle();
 f.view.utc=(latest+1200)*1000;f.view.updateStatus();
 assert.match(f.root.getElementById('radar-status').textContent,/showing last available/);
 f.view.utc=(latest+1800)*1000;f.view.updateStatus();
 assert.equal(f.root.getElementById('radar-image').hidden,true);
 f.state.data.radar.past=f.state.data.radar.past.map(frame=>({...frame,time:frame.time+1800,path:frame.path+'-fresh'}));
 await f.view.refresh();await settle();
 assert.equal(f.root.getElementById('radar-image').hidden,false);
 assert.equal(f.view.frames.at(-1).time,latest+1800);
 f.view.dispose();
});
