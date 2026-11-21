const test = require('node:test');
const assert = require('node:assert/strict');
const status = { hidden: false, textContent: '' };
const vr = { hidden: true };
const gl = { FRAMEBUFFER: 1, async makeXRCompatible() {}, bindFramebuffer() {} };
const canvas = { focus() {}, getContext() { return gl; } };
global.document = { baseURI: 'https://example.test/builds/game/', getElementById(id) { return {status,vr,canvas}[id]; } };
const { Module: runtime, configureXR, slatePoint } = require('../web/boot.js');
const flush = () => new Promise(resolve => setImmediate(resolve));

test('load individual files under a prefix and report failed assets', async () => {
  let finished = false;
  const writes = [];
  runtime.FS = {mkdirTree(){},writeFile(path){writes.push(path);}};
  runtime.addRunDependency = () => {};
  runtime.removeRunDependency = () => {finished = true;};
  global.fetch = async url => {
    assert.match(url.pathname, /^\/builds\/game\//);
    return { ok:true, json:async()=>(url.pathname.endsWith('pack.json') ? {entry:'change.lua',files:['livery.mesh']} : {files:['scripts/game.lua','flag'],packs:['mods/repaint']}), arrayBuffer:async()=>new ArrayBuffer(4) };
  };
  runtime.preRun[0]();await flush();assert.ok(finished);assert.equal(writes.length,6);
  global.fetch=async()=>({ok:false,status:404});runtime.preRun[0]();await flush();
  assert.match(status.textContent,/content.json returned 404/);
  runtime.onRuntimeInitialized();assert.ok(status.hidden);
  runtime.printErr('Startup: missing test asset'); assert.equal(status.hidden,false);
});

test("stream sector meshes once and retry a failed crossing",async()=>{
  const seen=[], replies=[];
  runtime.FS={mkdirTree(){},writeFile(){},analyzePath:p=>({exists:p.endsWith("previous.sav")}),
    readFile:()=>new TextEncoder().encode("ORRA1 00000000\\ntide-window - ")};
  runtime._orra_sector_ready=(ticket,ok)=>replies.push([ticket,ok]);
  runtime.addRunDependency=()=>{};runtime.removeRunDependency=()=>{};
  let fail=true;
  global.fetch=async url=>{
    const path=url.pathname.split("/game/")[1];seen.push(path);
    return {ok:!(path.endsWith("station.mesh")&&fail),status:503,
      json:async()=>({files:["assets/sectors.txt",...["tank","port","station"].map(x=>"assets/models/"+x+".mesh")]}),
      text:async()=>"gate-six tank\\ntide-window port\\nhigh-water station",
      arrayBuffer:async()=>new ArrayBuffer(4)};
  };
  runtime.preRun[0]();await flush();
  assert.ok(seen.includes("assets/models/port.mesh")&&!seen.includes("assets/models/station.mesh"));
  assert.equal(runtime.requestSector("gate-six",1),1);
  assert.equal(runtime.requestSector("high-water",2),0);await flush();assert.deepEqual(replies.pop(),[2,0]);
  fail=false;runtime.requestSector("high-water",3);await flush();assert.deepEqual(replies.pop(),[3,1]);
  runtime.requestSector("mod-sector",4);await flush();assert.deepEqual(replies.pop(),[4,1]);
  assert.equal(seen.filter(p=>p.endsWith("tank.mesh")).length,1);
});

function setup(fallback=false) {
  const handlers={}, frames=[], calls=[], released=[];
  let viewport=0;
  const session = {
    inputSources:[],requestAnimationFrame(f){frames.push(f);},
    updateRenderState(v){assert.ok(v.baseLayer);},
    async requestReferenceSpace(id){
      if(fallback && id==='local-floor')throw Error('No floor');
      return {getOffsetReferenceSpace(){return {floor:true};}};
    },
    addEventListener(name,f){handlers[name]=f;},
    async end(){if(handlers.end)handlers.end();}
  };
  const adapter = {
    _orra_xr_wrist:p=>calls.push(["wrist",p]), _orra_xr_pick:(...p)=>calls.push(["pick",...p]), _orra_xr_mark:()=>0,
    canvas,HEAPF32:new Float32Array(80),_malloc:()=>16,_free:p=>released.push(p),
    _orra_xr_mode:v=>calls.push(['mode',v]),_orra_xr_tick:(...v)=>calls.push(['tick',...v]),
    _orra_xr_eye:(...v)=>calls.push(['eye',...v]),_orra_xr_action:v=>calls.push(['action',v])
  };
  global.XRWebGLLayer=class {getViewport(){return {x:viewport++*50,y:0,width:50,height:50};}};
  global.XRRigidTransform=class {};
  const xr={isSessionSupported:async()=>true,requestSession:async()=>session};
  return {handlers,frames,calls,released,session,adapter,xr};
}

test('unsupported and declined sessions leave flat play available', async () => {
  const button={hidden:true};await configureXR(runtime,null,button);
  await configureXR(runtime,{isSessionSupported:async()=>{throw Error('unavailable');}},button);assert.ok(button.hidden);
  const t=setup();t.xr.requestSession=async()=>{throw Error('declined');};
  await configureXR(t.adapter,t.xr,button);await button.onclick();
  assert.match(status.textContent,/declined/);assert.equal(button.disabled,false);
  t.xr.requestSession=async()=>t.session;t.session.requestReferenceSpace=async()=>{throw Error('no reference space');};
  await configureXR(t.adapter,t.xr,button);await button.onclick();assert.match(status.textContent,/no reference space/);
});

test('stereo views, locomotion, input edges and exit preserve one simulation',async()=>{
  for(const fallback of [true,false]){
    const t=setup(fallback),button={hidden:true};await configureXR(t.adapter,t.xr,button);await button.onclick();
    assert.deepEqual(t.calls[0],['mode',1]);assert.ok(t.frames.length);
    const left={handedness:'left',gamepad:{axes:[.25,-.6],buttons:[]}};
    const right={handedness:'right',gamepad:{axes:[.9,0],buttons:[{},{},{},{},{pressed:true}]}};
    t.session.inputSources=[{},left,right];
    for(const inputSource of [left,right]){
      t.handlers.selectstart({inputSource});t.handlers.squeezestart({inputSource});
    }
    const matrix=new Float32Array(16);
    const view={projectionMatrix:matrix,transform:{inverse:{matrix},position:{x:0,y:1.7,z:0}}};
    const valid={getViewerPose:()=>({views:[view,view]})};
    t.frames.shift()(10,{getViewerPose:()=>null});
    t.frames.shift()(100,valid);t.frames.shift()(120,valid);
    const ticks=t.calls.filter(c=>c[0]==='tick'), eyes=t.calls.filter(c=>c[0]==='eye');
    assert.equal(eyes.length,4);assert.equal(ticks.length,2);assert.equal(ticks[0][2],.6);
    assert.ok(ticks[0][4]>0);assert.equal(ticks[1][4],0);assert.equal(ticks[1][1],.02);
    assert.deepEqual(t.calls.filter(c=>c[0]==='action').map(c=>c[1]),[0,1,2,3,1]);
    const hand=new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,-.5,1]);
    const ray=new Float32Array(hand);ray[13]=.19;ray[14]=0;
    left.gripSpace="left";right.targetRaySpace="right";
    t.frames.shift()(140,{...valid,getPose:which=>({transform:{matrix:which==="left"?hand:ray}})});
    t.handlers.selectstart({inputSource:right});assert.ok(t.calls.some(c=>c[0]==="pick"));
    await t.session.end();assert.deepEqual(t.calls.at(-1),['mode',0]);assert.deepEqual(t.released,[16]);
  }
});

test('tracked controller ray intersects a physical slate',()=>{
  const slate=new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,-.5,1]);
  const ray=new Float32Array(slate);ray[14]=0;
  assert.deepEqual(slatePoint(slate,ray),[.5,.5]);
  ray[12]=1;assert.equal(slatePoint(slate,ray),null);ray[14]=-1;assert.equal(slatePoint(slate,ray),null);
  ray[10]=0;assert.equal(slatePoint(slate,ray),null);
});

test("browser save transaction restores both generations and rolls back on quota failure",async()=>{
  const {prepareSaves}=require("../web/boot.js");
  const disk=new Map([["command.sav",new Uint8Array([1])]]);
  const files=new Map([["/orra-saves/previous.sav",new Uint8Array([9])]]);
  const callbacks=[];
  const host={FS:{mkdirTree(){},analyzePath:path=>({exists:files.has(path)}),unlink:path=>files.delete(path),
    writeFile:(path,value)=>files.set(path,value),readFile:path=>files.get(path)},_orra_saved:v=>callbacks.push(v)};
  let blocked=false,quota=false;
  const db={createObjectStore(){},transaction(store,mode){
    const jobs=[];
    const tx={objectStore(){return {
      get(key){const req={};jobs.push(()=>{req.result=disk.get(key);req.onsuccess();});return req;},
      put(value,key){jobs.push(()=>disk.set(key,value));}
    };}};
    setImmediate(()=>{if (quota && mode==="readwrite") tx.onabort();else {jobs.forEach(fn=>fn());tx.oncomplete();}});
    return tx;
  }};
  const factory={open(){
    const request={result:db};
    setImmediate(()=>{if(blocked)request.onblocked();else{request.onupgradeneeded();request.onsuccess();}});
    return request;
  }};
  await prepareSaves(host,factory);
  assert.deepEqual(files.get("/orra-saves/command.sav"),new Uint8Array([1]));
  assert.ok(!files.has("/orra-saves/previous.sav"));
  files.set("/orra-saves/command.sav",new Uint8Array([2]));files.set("/orra-saves/previous.sav",new Uint8Array([1]));
  host.persistSlots();await flush();assert.equal(callbacks.pop(),1);
  quota=true;files.set("/orra-saves/command.sav",new Uint8Array([3]));
  host.persistSlots();await flush();await flush();await flush();
  assert.equal(callbacks.pop(),0);assert.equal(files.get("/orra-saves/command.sav")[0],2);
  blocked=true;await prepareSaves(host,factory);host.persistSlots();assert.equal(callbacks.pop(),0);
});
