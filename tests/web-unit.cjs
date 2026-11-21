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
