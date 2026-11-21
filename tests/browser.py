"""Run rendered Chromium desktop, touch and emulated WebXR smoke tests on a built web directory."""
import argparse
import json
import os
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
    s.inputSources=[];s.updateRenderState=()=>{};
    s.requestReferenceSpace=async()=>({});
    s.requestAnimationFrame=callback=>requestAnimationFrame(t=>{if(!s.ended)callback(t,{
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
    errors,requests=[],[]
    page.on("pageerror",lambda error: errors.append(str(error)))
    page.on("response",lambda response: requests.append(response.url))
    if pack:
        def enable(route):
            source=route.fetch()
            manifest=source.json()
            manifest["packs"]=["mods/long-watch"]
            route.fulfill(response=source,json=manifest)
        page.route("**/content.json",enable)
    def status(): return page.evaluate(STATUS)
    def advance(ticks): page.evaluate("n=>Module._orra_clock(n)",ticks)
    def xr_event(kind, hand):
        page.evaluate("""([kind,hand])=>{const e=new Event(kind);e.inputSource={handedness:hand};testSession.dispatchEvent(e)}""",[kind,hand])
    def touch(x,y): page.touchscreen.tap(x,y)
    def toggle():
        if layout=="touch": touch(752,492)
        elif layout=="xr": xr_event("selectstart","left")
        else: page.keyboard.press("Tab")
        advance(0)
    def order(index):
        if layout=="touch": touch(715,134+24*(index-1))
        elif layout=="xr":
            for _ in range((index-1-int(status()[6])) % 14): xr_event("squeezestart","left")
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
            origin=status()
            fingers("touchStart",[(1,120,300),(2,630,300)])
            fingers("touchMove",[(1,120,246),(2,666,294)])
            advance(12)
            moved=status()
            assert moved[13:15]==["1","1"] and moved[11]!=origin[11] and moved[12]!=origin[12], moved
            fingers("touchCancel",[])
            advance(1)
            assert status()[13:15]==["0","0"],status()
            page.set_viewport_size({"width":540,"height":900})
            page.wait_for_function("()=>document.querySelector('canvas').height>document.querySelector('canvas').width")
            assert status()[5]=="1"
            page.set_viewport_size({"width":900,"height":540})
            page.wait_for_function("()=>document.querySelector('canvas').width>document.querySelector('canvas').height")
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
        toggle()
        order(8);order(7);advance(2450)
        if pack:
            assert status()[0]=="active" and any(s.endswith("/oxide.mesh") for s in requests),status()
            advance(3500)
        assert status()[0]=="victory",status()
        if layout=="xr":
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
