"""Run rendered Chromium desktop, touch and emulated WebXR smoke tests on a built web directory."""
import argparse
import json
import os
import math
import functools
import http.server
import threading
from pathlib import Path
from playwright.sync_api import sync_playwright

class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass

STATUS = "()=>Module.UTF8ToString(Module._orra_status()).split('|')"
READY = "()=>typeof Module._orra_status==='function' && Module.UTF8ToString(Module._orra_status()).startsWith('active|')"
XR_DEVICE = """
Object.defineProperty(navigator, 'xr', {value: {
  isSessionSupported:async()=>true,
  requestSession:async()=>{
    const s=window.testSession=new EventTarget();
    s.inputSources=[{handedness:"left",gamepad:{axes:[0,0],buttons:[]}},
                    {handedness:"right",targetRaySpace:"hand",gamepad:{axes:[0,0],buttons:[]}}];s.updateRenderState=()=>{};
    s.requestReferenceSpace=async()=>({});
    s.requestAnimationFrame=callback=>requestAnimationFrame(t=>{if(!s.ended)callback(t,{
      getPose:which=>which==="hand" && window.testRay ? {transform:{matrix:new Float32Array(testRay)}} : null,
      getViewerPose:()=>({views:[-.032,.032].map(x=>({
        projectionMatrix:new Float32Array([1,0,0,0,0,1,0,0,0,0,-1,-1,0,0,-.2,0]),
        transform:{inverse:{matrix:new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,-x,-1.72,0,1])},position:{x,y:1.72,z:0}}
      }))})})});
    s.end=async()=>{s.ended=true;s.dispatchEvent(new Event('end'));};return s;
  }
}});
WebGL2RenderingContext.prototype.makeXRCompatible=async()=>{};
const clear=WebGL2RenderingContext.prototype.clear;
window.xrDepthRegions=new Set();
WebGL2RenderingContext.prototype.clear=function(mask){
  const v=this.getParameter(this.VIEWPORT), box=this.getParameter(this.SCISSOR_BOX);
  if(window.testSession && !testSession.ended && (mask & this.DEPTH_BUFFER_BIT) &&
     v[2]*2===this.drawingBufferWidth && this.isEnabled(this.SCISSOR_TEST) &&
     v.every((value,i)=>value===box[i])) xrDepthRegions.add(v[0]);
  return clear.call(this,mask);
};
window.XRWebGLLayer=class{
  constructor(s,g){this.framebuffer=null;this.gl=g;}
  getViewport(v){const w=this.gl.drawingBufferWidth/2;window.xrEyes=(window.xrEyes||0)+1;
    return{x:v.transform.position.x<0?0:w,y:0,width:w,height:this.gl.drawingBufferHeight};}
};
"""

def exercise(browser, url, layout, pack=False):
    context=browser.new_context(viewport={"width":900,"height":540},has_touch=layout=="touch",is_mobile=layout=="touch")
    if layout=="xr": context.add_init_script(XR_DEVICE)
    page=context.new_page()
    errors,requests,history=[],[],[]
    page.on("pageerror",lambda error: errors.append(str(error)))
    page.on("response",lambda response: requests.append(response.url))
    if pack:
        def enable(route):
            source=route.fetch()
            manifest=source.json()
            manifest["packs"]=["mods/long-watch","mods/wire-quay"]
            route.fulfill(response=source,json=manifest)
        page.route("**/content.json",enable)
    def status(): return page.evaluate(STATUS)
    def advance(ticks):
        page.evaluate("n=>Module._orra_clock(n)",ticks)
        history.append(["advance",ticks,status()])
    def xr_event(kind, hand):
        history.append(["controller",kind,hand])
        page.evaluate("""([kind,hand])=>{const e=new Event(kind);e.inputSource={handedness:hand};testSession.dispatchEvent(e)}""",[kind,hand])
    def touch(x,y):
        history.append(["tap",x,y]);page.touchscreen.tap(x,y)
    def toggle():
        if layout=="touch": touch(752,492)
        elif layout=="xr": xr_event("selectstart","left")
        else: page.keyboard.press("Tab")
        advance(0)
    def order(index):
        history.append(["order",index])
        if layout=="touch": touch(715,134+24*(index-1))
        elif layout=="xr":
            for _ in range((index-1-int(status()[6])) % int(status()[18])): xr_event("squeezestart","left")
            xr_event("selectstart","right")
        else: page.keyboard.press(str(index))
        advance(0)
    try:
        page.goto(url+"?replayClock=1")
        page.wait_for_function(READY)
        assert len(page.locator("canvas").screenshot())>12000
        assert int(status()[15])==(2700 if pack else 1800)
        if layout=="touch":
            assert status()[5]=="1"
            cdp=context.new_cdp_session(page)
            def fingers(kind, points):
                cdp.send("Input.dispatchTouchEvent",{"type":kind,"touchPoints":[{"x":x,"y":y,"id":i} for i,x,y in points]})
            def wait_for_fingers(pressed, previous_yaw=None):
                # Chromium may acknowledge the command before delivering it to the canvas.
                page.wait_for_function("""([pressed, previousYaw]) => {
                    Module._orra_clock(0);
                    const fields = Module.UTF8ToString(Module._orra_status()).split('|');
                    return fields[13] === pressed && fields[14] === pressed &&
                        (previousYaw === null || fields[12] !== previousYaw);
                }""", arg=["1" if pressed else "0", previous_yaw], timeout=5000)
            origin=status()
            fingers("touchStart",[(1,120,300),(2,630,300)])
            fingers("touchMove",[(1,120,246),(2,666,294)])
            wait_for_fingers(True, origin[12])
            advance(12)
            moved=status()
            assert moved[13:15]==["1","1"] and moved[11]!=origin[11] and moved[12]!=origin[12], moved
            fingers("touchCancel",[])
            wait_for_fingers(False)
            advance(1)
            assert status()[13:15]==["0","0"],status()
            touch(204,43);advance(0);touch(715,134);advance(0)
            draft=status();assert draft[1]=="assignment",draft
            page.set_viewport_size({"width":540,"height":900})
            page.wait_for_function("()=>document.querySelector('canvas').height>document.querySelector('canvas').width")
            assert status()[5]=="1" and status()[16:18]==draft[16:18] and status()[1]=="assignment"
            page.set_viewport_size({"width":900,"height":540})
            page.wait_for_function("()=>document.querySelector('canvas').width>document.querySelector('canvas').height")
            touch(204,43);advance(0);touch(204,43);advance(0)
            assert status()[1]=="field"
        if layout=="xr":
            page.locator("#vr").click()
            page.wait_for_function("()=>window.xrEyes>=4 && xrDepthRegions.size===2")
        toggle()
        assert status()[1]=="orders",status()
        order(2);order(1);advance(300)
        order(4);advance(310);order(6);advance(310);order(3);advance(95);order(5);advance(92)
        toggle()
        frame=None
        if layout=="desktop":
            def walk(key,ticks):
                page.keyboard.down(key);advance(ticks);page.keyboard.up(key);advance(0)
            walk("d",28);walk("w",57)
            page.keyboard.press("e");advance(0)
            assert int(status()[8])>=0,status()
            frame=page.locator("canvas").screenshot()
            if not pack:
                saved=status()
                page.keyboard.press("F5");advance(0)
                page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('saved in this browser')")
                page.reload();page.wait_for_function(READY)
                page.keyboard.press("F9");advance(0)
                restored=status()
                assert (restored[3],restored[7],restored[8])==(saved[3],saved[7],saved[8]),restored
        if layout=="xr":
            def stick(x,y,ticks):
                eyes=page.evaluate("window.xrEyes")
                page.evaluate("xy=>testSession.inputSources[0].gamepad.axes=xy",[x,-y])
                page.wait_for_function("old=>window.xrEyes>old+2",arg=eyes)
                advance(ticks)
            stick(1,0,28);stick(0,1,57);stick(0,0,0)
            xr_event("squeezestart","right");advance(0)
            assert int(status()[8])>=0,status()
        toggle()
        order(8);order(7);advance(2450)
        if pack:
            assert status()[0]=="active" and any(s.endswith("/oxide.mesh") for s in requests),status()
            advance(3500)
        assert status()[0]=="victory",status()
        if layout=="xr":
            assert int(status()[8])>=0
            targets=[("gate-six","mole",-8,1,2),("tide-window","kestrel",0,3,0),
                     ("westbound","shunter",0,2,-3),("white-mile","barrow",0,2,-3),
                     ("night-mail","kite",0,3,-10),("high-water","lark",0,3,0),
                     ("compact-bank","hinge",-27,2,5)]
            for index,(sector,crew,x,y,z) in enumerate(targets):
                page.keyboard.press("m");advance(0);page.keyboard.press(str(index+1));advance(0)
                state=status();assert state[7]==sector,state
                cx,cz,yaw=float(state[10]),float(state[11]),float(state[12])
                cy=1.72
                if sector=="compact-bank":cx,cy,cz=-25,2.56,-22
                vx,vy,vz=x-cx,y-cy,z-cz
                norm=math.sqrt(vx*vx+vy*vy+vz*vz)
                vx,vy,vz=(vx*math.cos(yaw)+vz*math.sin(yaw))/norm,vy/norm,(-vx*math.sin(yaw)+vz*math.cos(yaw))/norm
                ray=[1,0,0,0,0,1,0,0,-vx,-vy,-vz,0,0,1.72,0,1]
                old=page.evaluate("xrEyes")
                page.evaluate("ray=>window.testRay=ray",ray)
                page.wait_for_function("old=>xrEyes>old+2",arg=old)
                xr_event("selectstart","right");advance(0)
                assert status()[1]=="assignment" and status()[20]==crew,(sector,status())
                page.keyboard.press("Escape");page.keyboard.press("Escape");advance(0)
            page.evaluate("()=>testSession.end()")
            page.keyboard.press("Escape");advance(0)
            assert status()[1]=="field"
        if layout=="touch":
            touch(450,270);advance(0)
            assert status()[0]=="active"
        assert not errors, errors
        return frame
    except Exception:
        out=Path(os.getenv("ORRA_FAILURE_DIR","browser-failures"))
        out.mkdir(exist_ok=True,parents=True)
        (out/(layout+"-input.json")).write_text(json.dumps(history,indent=2))
        page.screenshot(path=out/(layout+"-failure.png"),timeout=10000)
        (out/(layout+"-state.txt")).write_text(str(page.evaluate("()=>typeof Module._orra_status==='function' ? Module.UTF8ToString(Module._orra_status()) : document.body.innerText"))+"\n"+str(errors))
        raise
    finally:
        context.close()

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("web",type=Path);parser.add_argument("--chromium")
    args=parser.parse_args()
    handler=functools.partial(Quiet,directory=str(args.web.resolve().parent))
    with http.server.ThreadingHTTPServer(("127.0.0.1",0),handler) as server,sync_playwright() as pw:
        threading.Thread(target=server.serve_forever,daemon=True).start()
        url=f"http://127.0.0.1:{server.server_port}/{args.web.resolve().name}/"
        browser=pw.chromium.launch(executable_path=args.chromium,headless=True,
            args=["--use-gl=angle","--use-angle=swiftshader","--enable-unsafe-swiftshader","--disable-dev-shm-usage"])
        try:
            base=exercise(browser,url,"desktop")
            changed=exercise(browser,url,"desktop",pack=True)
            assert base!=changed,"Expected the pack livery to change the captured frame"
            exercise(browser,url,"touch");exercise(browser,url,"xr")
        finally:
            browser.close();server.shutdown()
    print("Desktop boarding, loose livery and hold overrides, touch and stereo XR victories passed.")
