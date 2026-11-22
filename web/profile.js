/* Optional, local-only hardware qualification at ?profile=1. */
function summarizeRoute(samples, latency, meta) {
  function p95(values) {
    if(!values.length) return null;
    const sorted=values.slice().sort((a,b)=>a-b);
    return Math.round(sorted[Math.ceil(sorted.length*.95)-1]*100)/100;
  }
  const budget=meta.target==="phone"?1000/30:meta.target==="headset"?1000/meta.hz:1000/60;
  const frame=p95(samples), input=p95(latency);
  const inputBudget=meta.target==="headset"?2*budget:meta.target==="phone"?100:50;
  return {...meta,frames:samples.length,p95FrameMs:frame,p95InputToNextFrameMs:input,
    inputBudgetMs:inputBudget,inputWithinBudget:input!==null && input<=inputBudget,
    estimatedMissedIntervals:samples.reduce((n,ms)=>n+Math.max(0,Math.round(ms/budget)-1),0),
    measuredOverBudget:samples.filter(v=>v>budget).length,frameBudgetMs:budget,
    memoryBudgetMiB:256,heapWithinBudget:meta.heapMiB<=256,
    frameWithinBudget:frame!==null && frame<=budget, coldStartBudgetMs:5000,
    coldStartWithinBudget:meta.bootMs<=5000};
}
function installProfile(runtime,doc,win) {
  if(!new URLSearchParams(win.location.search).has("profile"))return;
  const box=doc.getElementById("profile"), button=doc.getElementById("measure"), output=doc.getElementById("measurement");
  box.hidden=false;
  const reports=[];
  let bootMs=null;
  let frames=[],inputs=[],prev=0,lastInput=null,live=false,peak=0,kind="";
  const now=()=>win.performance.now();
  const input=()=>{if(live)lastInput=now();};
  win.addEventListener("pointerdown",input,true); win.addEventListener("keydown",input,true);
  runtime.profileInput=input;
  runtime.profileFrame=(immersive=false)=>{
    if(bootMs===null)bootMs=Math.round(now());
    if(!live || (kind==="headset")!==immersive)return;
    const t=now();
    if(prev)frames.push(t-prev);
    if(lastInput!==null){inputs.push(t-lastInput);lastInput=null;}
    prev=t;peak=Math.max(peak,runtime.HEAPF32.byteLength/1048576);
  };
  button.onclick=()=>{
    if(!live){
      kind=doc.getElementById("device-kind").value;
      if(!doc.getElementById("device-name").value.trim()){output.textContent="Enter the reference device and browser.";return;}
      frames=[];inputs=[];prev=0;lastInput=null;peak=0;live=true;
      button.textContent="Finish route";output.textContent="Measuring. Use the normal game controls.";
    }else{
      live=false;
      const state=runtime.UTF8ToString(runtime._orra_status()).split("|");
      const report=summarizeRoute(frames,inputs,{target:kind,device:doc.getElementById("device-name").value,
        route:doc.getElementById("route-name").value,mission:state[7],hz:Number(doc.getElementById("refresh-hz").value)||60,
        viewport:[runtime.canvas.width,runtime.canvas.height],heapMiB:peak,bootMs,build:doc.getElementById("build").textContent,
        observations:doc.getElementById("observations").value});
      reports.push(report);output.textContent=JSON.stringify(reports,null,2);button.textContent="Measure route";
    }
  };
}
if(typeof module!=="undefined" && module.exports)module.exports={summarizeRoute,installProfile};
