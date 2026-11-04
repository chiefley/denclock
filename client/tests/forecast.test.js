import test from 'node:test';
import assert from 'node:assert/strict';
import { ForecastClient, parseForecast, selectHours, chartMarkup, forecastUrl, selectDays, dailyChartMarkup, initializeForecast } from '../forecast.js';
const start = Date.parse('2026-11-01T04:00:00Z');
const data = () => ({timezone:'America/New_York',hourly_units:{temperature_2m:'°C',precipitation_probability:'%'},hourly:{time:Array.from({length:72},(_,i)=>start/1000+i*3600),temperature_2m:Array(72).fill(12),precipitation_probability:Array(72).fill(30)}});
test('24 elapsed hours, 3-hour ticks, flat temperatures and DST timezone',()=>{
 const sample=parseForecast(data()), hours=selectHours(sample,start+1800000);
 assert.equal(hours.length,24); assert.equal(hours[0].time,start);
 const markup=chartMarkup(hours,'F',sample.timezone);
 assert.equal((markup.match(/forecast-value/g)||[]).length,8);
 assert.ok(markup.includes('54°')); assert.ok(!markup.includes('NaN'));
 assert.equal(selectHours(sample,start+72*3600000).length,0);
});
test('validation, missing values and line gaps',()=>{
 const d=data(); d.hourly.temperature_2m[3]=null; d.hourly.precipitation_probability[4]=null;
 const sample=parseForecast(d); assert.equal(sample.hours[3].temperature,null);
 assert.equal((chartMarkup(sample.hours.slice(0,24),'C',sample.timezone).match(/M[0-9]/g)||[]).length,2);
 d.hourly.time[4]++; assert.throws(()=>parseForecast(d));
 assert.equal(forecastUrl(null),null); assert.equal(forecastUrl({latitude:NaN,longitude:0}),null);
});
test('outage and malformed payload retain sample; retry recovers without overlap', async()=>{
 let mode='ok',release; const c=new ForecastClient({latitude:41,longitude:-81},{fetchData:async()=>{
 if(mode==='wait') await new Promise(r=>release=r);
 if(mode==='fail') throw Error('offline');
 return {ok:true,json:async()=>mode==='bad'?{}:data()};
 }});
 assert.equal(await c.refresh(),true); const old=c.sample;
 mode='fail'; assert.equal(await c.refresh(),false); assert.equal(c.sample,old);
 mode='bad'; assert.equal(await c.refresh(),false); assert.equal(c.sample,old);
 mode='wait'; const pending=c.refresh(); assert.equal(await c.refresh(),false); release(); await pending;
 assert.equal(c.lastError,null);
});

test('location lookup supports city/state and ZIP and checks forecast before returning', async () => {
 const { resolveForecastLocation } = await import('../forecast.js');
 for (const query of ['Akron, OH', '44308']) {
  const urls=[];
  const location=await resolveForecastLocation(query,async url=>{
   urls.push(url);
   return {ok:true,json:async()=>url.includes('geocoding') ? {results:[{name:'Akron',admin1:'Ohio',latitude:41.08,longitude:-81.52}]} : data()};
  });
  assert.equal(location.query,query); assert.equal(location.displayName,'Akron, Ohio');
  assert.match(urls[1],/latitude=41.08/);
 }
 await assert.rejects(resolveForecastLocation('missing comma'),/city/);
 await assert.rejects(resolveForecastLocation('Nowhere, OH',async()=>({ok:true,json:async()=>({})})),/not found/);
 await assert.rejects(resolveForecastLocation('44308',async url=>({ok:url.includes('geocoding'),json:async()=>({results:[{name:'Akron',admin1:'Ohio',latitude:41,longitude:-81}]})})),/Forecast unavailable/);
});

test('a location change discards an older in-flight forecast and fetches the new coordinates',async()=>{
 let release; const urls=[];
 const client=new ForecastClient({latitude:41,longitude:-81},{fetchData:async url=>{
  urls.push(url); if(urls.length===1) await new Promise(r=>release=r);
  return {ok:true,json:async()=>data()};
 }});
 const old=client.refresh();
 client.setLocation({latitude:40,longitude:-80,displayName:'New place'});
 release(); await old;
 await new Promise(r=>setImmediate(r));
 assert.equal(urls.length,2); assert.match(urls[1],/latitude=40/);
 assert.equal(client.location.displayName,'New place'); assert.ok(client.sample);
});

function dailyData() {
 const d = data();
 d.utc_offset_seconds = -14400;
 d.daily_units = {temperature_2m_max:'°C',temperature_2m_min:'°C',precipitation_probability_max:'%'};
 d.daily = {
  time:Array.from({length:10},(_,i)=>start/1000+i*86400),
  temperature_2m_max:[12,14,16,18,19,15,11,10,13,14],
  temperature_2m_min:[4,6,8,10,11,7,3,2,5,6],
  precipitation_probability_max:[0,10,20,30,40,50,60,70,80,null],
 };
 return d;
}

test('one request includes ten daily forecasts and hourly data',()=>{
 const url=new URL(forecastUrl({latitude:41,longitude:-81}));
 assert.equal(url.searchParams.get('forecast_days'),'10');
 assert.match(url.searchParams.get('daily'),/temperature_2m_max,temperature_2m_min,precipitation_probability_max/);
 assert.equal(url.searchParams.get('timeformat'),'unixtime');
});

test('daily selection follows local calendar dates across DST, midnight and month rollover',()=>{
 const sample=parseForecast(dailyData());
 assert.equal(sample.days[0].date,'2026-11-01');
 assert.equal(selectDays(sample,Date.parse('2026-11-02T04:30Z'))[0].date,'2026-11-01');
 const next=selectDays(sample,Date.parse('2026-11-02T05:00Z'));
 assert.equal(next[0].date,'2026-11-02'); assert.equal(next.at(-1).date,'2026-11-11');
 assert.equal(next.at(-1).high,null); // No invented eleventh forecast.
 const month=selectDays({...sample,days:[]},Date.parse('2026-12-30T12:00Z'));
 assert.equal(month.at(-1).date,'2027-01-08');
 const spring=selectDays({...sample,days:[]},Date.parse('2026-03-08T05:00Z'));
 assert.equal(spring[0].date,'2026-03-08'); assert.equal(spring[1].date,'2026-03-09');
 // Positive and half-hour UTC offsets use provider calendar dates as well.
 const d=dailyData(); d.utc_offset_seconds=19800; d.timezone='Asia/Kolkata';
 d.daily.time=d.daily.time.map((_,i)=>Date.parse('2026-11-01T00:00Z')/1000-19800+i*86400);
 assert.equal(parseForecast(d).days[0].date,'2026-11-01');
 assert.equal(selectDays(parseForecast(d),Date.parse('2026-11-01T19:00Z'))[0].date,'2026-11-02');
});

test('daily gaps, missing optional rain and malformed data do not corrupt the hourly forecast',()=>{
 const d=dailyData();
 d.daily.temperature_2m_max[3]=null; d.daily.temperature_2m_min[4]='bad';
 d.daily.precipitation_probability_max[5]=101;
 const sample=parseForecast(d), days=selectDays(sample,start);
 assert.equal(days[3].high,null); assert.equal(days[4].low,null); assert.equal(days[5].precipitation,null);
 const markup=dailyChartMarkup(days,'C');
 assert.equal((markup.match(/M[0-9]/g)||[]).length,4);
 assert.ok(!/NaN|undefined|forecast-grid/.test(markup));
 delete d.daily.precipitation_probability_max;
 assert.equal(parseForecast(d).days[0].precipitation,null);
 d.daily.temperature_2m_max[0]=-30; // Reversed high/low are unavailable.
 assert.equal(parseForecast(d).days[0].high,null);
 delete d.daily_units;
 assert.equal(parseForecast(d).days.length,0); assert.equal(selectHours(parseForecast(d),start).length,24);
 assert.equal(dailyChartMarkup(selectDays(parseForecast(d),start),'F'),'');
 d.daily={time:'broken'};
 assert.doesNotThrow(()=>parseForecast(d));
});

test('daily curves convert units and stay inside padded chart bounds including flat/extreme values',()=>{
 const days=selectDays(parseForecast(dailyData()),start);
 const f=dailyChartMarkup(days,'F');
 assert.match(f,/54°/); assert.match(f,/39°/);
 assert.equal((f.match(/<path /g)||[]).length,2);
 assert.equal((f.match(/<circle /g)||[]).length,20);
 for(const extreme of [-100,0,100]) {
  const markup=dailyChartMarkup(days.map(d=>({...d,high:extreme,low:extreme})),'F');
  const coordinates=[...markup.matchAll(/<text[^>]* y="([\d.]+)"/g)].map(m=>+m[1]);
  assert.ok(coordinates.every(y=>y>=24 && y<=270));
  assert.ok(!markup.includes('NaN'));
 }
});

function forecastDom() {
 const nodes=new Map();
 for(const id of ['forecast-area','forecast-status','forecast-graph','forecast-title','forecast-key']) {
  const handlers=new Map(), attributes=new Map();
  nodes.set(id,{textContent:'',innerHTML:'',handlers,attributes,
   setAttribute:(k,v)=>attributes.set(k,v),
   toggleAttribute:(k,on)=>on?attributes.set(k,''):attributes.delete(k),
   addEventListener:(k,fn)=>handlers.set(k,fn),removeEventListener:k=>handlers.delete(k),
   closest:()=>null,
  });
 }
 return {getElementById:id=>nodes.get(id)};
}

test('whole-area toggles immediately, preserves attribution, renders refreshes and cleans up',()=>{
 const root=forecastDom(), area=root.getElementById('forecast-area');
 let requests=0, tick, stopped=false;
 const client={sample:{...parseForecast(dailyData()),receivedAt:0},url:'test',location:{displayName:'Akron'},
  now:()=>0,setLocation:()=>{},start:()=>requests++,stop:()=>{stopped=true;}};
 const original=globalThis.setInterval;
 globalThis.setInterval=fn=>{tick=fn;return null;};
 let stop;
 try { stop=initializeForecast(root,client,{sample:{utcMilliseconds:start,receivedAt:0},now:()=>0},
  {read:()=>({temperatureUnit:'F'})}); } finally {globalThis.setInterval=original;}
 const title=root.getElementById('forecast-title'), graph=root.getElementById('forecast-graph');
 assert.match(title.textContent,/Next 24 hours/);
 const hourly=graph.innerHTML;
 area.handlers.get('click')({target:graph});
 assert.match(title.textContent,/Next 10 days/); assert.match(area.attributes.get('aria-label'),/show weather radar/);
 assert.match(graph.innerHTML,/forecast-low/); assert.equal(requests,1);
 client.sample.days[0].high=30; tick(); assert.match(graph.innerHTML,/86°/);
 area.handlers.get('click')({target:{closest:()=>({})}}); // Credit navigates normally.
 assert.match(title.textContent,/Next 10 days/);
 let prevented=false;
 area.handlers.get('keydown')({target:area,key:' ',preventDefault:()=>{prevented=true;}});
 assert.ok(prevented); assert.match(title.textContent,/Weather radar/);
 area.handlers.get('click')({target:area});
 assert.equal(graph.innerHTML,hourly); assert.equal(requests,1);
 area.handlers.get('click')({target:area}); client.sample.days=[]; tick();
 assert.match(root.getElementById('forecast-status').textContent,/Daily forecast unavailable/);
 assert.ok(graph.attributes.has('hidden'));
 area.handlers.get('click')({target:area}); // Daily -> radar -> hourly.
 area.handlers.get('click')({target:area}); assert.equal(graph.innerHTML,hourly);
 stop(); assert.ok(stopped); assert.equal(area.handlers.size,0);
});
