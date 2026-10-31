import test from 'node:test';
import assert from 'node:assert/strict';
import { ForecastClient, parseForecast, selectHours, chartMarkup, forecastUrl } from '../forecast.js';
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
