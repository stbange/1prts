const test=require("node:test");
const assert=require("node:assert/strict");
const {installDesk}=require("../web/desk.js");
const {installProfile,summarizeRoute}=require("../web/profile.js");
function surface() {
  const parts={}, events={};
  class Element {
    constructor(){this.children=[];this.handlers={};this.style={};this.dataset={};this.textContent="";this.value="";this.scrollTop=0;}
    appendChild(x){this.children.push(x);}
    replaceChildren(){this.children=[];}
    focus(){doc.activeElement=this;}
    querySelectorAll(){return this.children;}
    addEventListener(k,f){this.handlers[k]=f;}
  }
  const doc={documentElement:{lang:"en"},getElementById(id){return parts[id]??=new Element();},createElement(){return new Element();},addEventListener:(k,f)=>events[k]=f};
  let time=50;
  const win={location:{search:""},performance:{now:()=>time},setTime:t=>time=t,addEventListener:(k,f)=>events[k]=f,
    setInterval(f){win.refresh=f;return 1;},clearInterval(){win.refresh=null;},requestAnimationFrame(f){win.frame=f;},
    speechSynthesis:{cancel(){},speak(x){win.spoken=x.text;}},SpeechSynthesisUtterance:class{constructor(s){this.text=s;}}};
  return {doc,win,events,parts,el:id=>doc.getElementById(id)};
}
test("semantic field desk keeps keyboard focus and device prompts synchronized",()=>{
  const {doc,win,events,el}=surface(), calls=[];
  const buttons=el("field-controls");
  buttons.appendChild(el("pause"));el("pause").dataset.key="19";
  let flags=1,labels=["Resume","Save"],title="Pause";
  const runtime={canvas:el("canvas"),UTF8ToString:s=>s,_orra_language_tag:()=>"en",_orra_menu_flags:()=>flags,_orra_menu_count:()=>labels.length,
    _orra_field_label:()=>"Pause",_orra_menu_row:i=>labels[i],_orra_menu_detail:()=>"Gate Six",_orra_menu_heading:()=>title,
    _orra_menu_pick:i=>calls.push(i),_orra_menu_key:k=>calls.push(k),_orra_interrupt:()=>calls.push("pause"),_orra_device:x=>calls.push(x)};
  const update=installDesk(runtime,doc,win);
  const rows=el("rows"), menu=el("desk");
  assert.equal(doc.activeElement,rows.children[0]);assert.equal(el("detail").textContent,"Gate Six");
  const event=k=>({key:k,preventDefault(){},stopPropagation(){}});
  for(const key of ["ArrowDown","Home","End","ArrowUp","Escape","Enter","Tab"," ","x"])menu.handlers.keydown(event(key));
  rows.children[1].onclick();assert.equal(calls.at(-1),1);
  labels=["Resume","Save B"];update();assert.equal(rows.children[1].textContent,"Save B");
  flags=17;menu.handlers.keydown(event("s"));assert.equal(doc.activeElement,runtime.canvas);
  flags=2;title="Orders";update();assert.equal(menu.className,"world-list");
  flags=0;update();assert.equal(menu.className,"assistive");
  labels=[];update();assert.ok(menu.hidden);assert.equal(el("pause").textContent,"Pause");
  el("pause").onclick();assert.equal(calls.at(-1),19);
  events.pointerdown({pointerType:"touch"});events.pointerdown({pointerType:"mouse"});events.keydown({});
  events.blur();doc.hidden=true;events.visibilitychange();assert.equal(calls.at(-1),"pause");
  runtime.speak("Crew assigned");assert.equal(win.spoken,"Crew assigned");events.pagehide();assert.equal(win.refresh,null);
});
test("local profiler attributes measured frames to a named route and preserves missing inputs",()=>{
  const {doc,win,events,el}=surface();
  const r={canvas:{width:900,height:540},HEAPF32:new Float32Array(256),UTF8ToString:s=>s,_orra_status:()=>"active|field|8|3|||0|gate-six"};
  installProfile(r,doc,win);assert.equal(win.frame,undefined);
  win.location.search="?profile=1";installProfile(r,doc,win);
  const button=el("measure"),output=el("measurement");button.onclick();assert.match(output.textContent,/reference device/i);
  el("device-name").value="Reference phone";el("route-name").value="Moving ferry";el("device-kind").value="phone";
  el("build").textContent="test";el("refresh-hz").value=60;
  button.onclick();r.profileFrame(false);win.setTime(60);events.pointerdown();win.setTime(65);r.profileFrame(false);win.setTime(85);r.profileFrame(false);
  button.onclick();const report=JSON.parse(output.textContent)[0];
  assert.equal(report.frames,2);assert.equal(report.p95FrameMs,20);assert.equal(report.p95InputToNextFrameMs,5);assert.ok(report.frameWithinBudget);
  el("device-kind").value="headset";button.onclick();r.profileInput();r.profileFrame(true);r.profileFrame(false);win.setTime(98);r.profileFrame(true);button.onclick();
  assert.equal(JSON.parse(output.textContent)[1].frames,1);
  assert.equal(summarizeRoute([],[],{target:"desktop",bootMs:9000,heapMiB:300}).frameWithinBudget,false);
});
