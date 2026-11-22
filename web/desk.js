/* A semantic view of the same C menu used by the native slate and XR wrist. */
function installDesk(runtime, doc, win) {
  const get = id => doc.getElementById(id);
  const menu=get("desk"), rows=get("rows"), controls=get("field-controls"), canvas=runtime.canvas;
  const str = p => runtime.UTF8ToString(p);
  let signature="", shown=false;
  function refresh() {
    if(runtime.stopped)return;
    const flags=runtime._orra_menu_flags(), count=runtime._orra_menu_count();
    const desk=!!(flags&1), touch=!!(flags&2), xr=!!(flags&4), narration=!!(flags&8);
    const tag=str(runtime._orra_language_tag());
    if(doc.documentElement.lang!==tag)doc.documentElement.lang=tag;
    menu.hidden=!count || xr;
    controls.hidden=desk || xr || !!count;
    controls.className=touch || narration ? "" : "assistive";
    menu.className=desk ? "" : (touch || narration ? "world-list" : "assistive");
    menu.style.fontSize=(100+25*(flags>>6))+"%";
    if(!controls.hidden) controls.querySelectorAll("button").forEach(b=>{
      const label=str(runtime._orra_field_label(Number(b.dataset.key)));
      if(b.textContent!==label)b.textContent=label;
    });
    if (!count || xr) {if(shown)canvas.focus();shown=false;signature="";return;}
    const title=str(runtime._orra_menu_heading()), detail=str(runtime._orra_menu_detail());
    if(get("detail").textContent!==detail)get("detail").textContent=detail;
    const labels=Array.from({length:count},(_,i)=>str(runtime._orra_menu_row(i)));
    const key=JSON.stringify([title,labels]);
    if(key!==signature) {
      const focus=doc.activeElement, index=Array.from(rows.children).indexOf(focus);
      const headingChanged=get("desk-title").textContent!==title;
      const scroll=menu.scrollTop;
      get("desk-title").textContent=title;
      rows.replaceChildren();
      labels.forEach((text,i)=>{
        const button=doc.createElement("button");button.type="button";button.textContent=text;
        button.onclick=()=>{runtime._orra_menu_pick(i);refresh();};
        rows.appendChild(button);
      });
      signature=key;
      if(desk && (!shown || headingChanged) || index>=0)
        rows.children[headingChanged?0:Math.max(0, Math.min(index,count-1))].focus({preventScroll:true});
      menu.scrollTop=headingChanged?0:scroll;
    }
    if(flags&16)canvas.focus();
    shown=true;
  }
  controls.querySelectorAll("button").forEach(b=>b.onclick=()=>{runtime._orra_menu_key(Number(b.dataset.key));refresh();});
  menu.addEventListener("keydown",e=>{
    if(runtime._orra_menu_flags()&16){canvas.focus();return;}
    if(!["ArrowDown","ArrowUp","Home","End","Escape","Tab","Enter"," "].includes(e.key))return;
    e.stopPropagation();
    if(e.key==="Escape"){e.preventDefault();runtime._orra_menu_key(41);refresh();return;}
    if(["Enter"," ","Tab"].includes(e.key))return;
    e.preventDefault();
    const items=Array.from(rows.children),i=items.indexOf(doc.activeElement);
    const next=e.key==="Home"?0:e.key==="End"?items.length-1:(i+items.length+(e.key==="ArrowDown"?1:-1))%items.length;
    items[next]?.focus();
  });
  win.addEventListener("pointerdown",e=>{runtime._orra_device(e.pointerType==="touch"?1:0);},true);
  win.addEventListener("keydown",()=>runtime._orra_device(0),true);
  win.addEventListener("blur",()=>{runtime._orra_interrupt();refresh();});
  doc.addEventListener("visibilitychange",()=>{if(doc.hidden){runtime._orra_interrupt();refresh();}});
  runtime.speak=text=>{if(win.speechSynthesis){win.speechSynthesis.cancel();const message=new win.SpeechSynthesisUtterance(text);message.lang=doc.documentElement.lang;win.speechSynthesis.speak(message);}};
  const interval=win.setInterval(refresh,120);
  win.addEventListener("pagehide",()=>win.clearInterval(interval),{once:true});
  refresh();
  return refresh;
}
if(typeof module!=="undefined" && module.exports)module.exports={installDesk};
