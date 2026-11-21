"""Run rendered Chromium desktop, touch and emulated WebXR smoke tests on a built web directory."""
import argparse
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
def exercise(browser, url, mobile=False):
    context = browser.new_context(viewport={"width": 900, "height": 540}, has_touch=mobile, is_mobile=mobile)
    if not mobile:
        context.add_init_script(XR_DEVICE)
    page = context.new_page()
    errors = []
    page.on("pageerror", lambda err: errors.append(str(err)))
    page.goto(url)
    page.wait_for_function(READY)
    assert len(page.locator("canvas").screenshot())>12000, "Expected a detailed rendered world frame"
    if mobile:
        page.wait_for_function("()=>" + STATUS[4:] + "[5]==='1'")
        # Bottom right control strip, then the second command on the opened slate.
        page.touchscreen.tap(900-148,540-48)
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('|orders|')")
        page.touchscreen.tap(900-180,154)
        page.set_viewport_size({"width": 540, "height": 900})
    else:
        page.keyboard.press("Tab");page.keyboard.press("2")
    page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('|orders|4|')")
    assert page.evaluate(STATUS)[4].startswith('Plate Mill'), page.evaluate(STATUS)
    if not mobile:
        page.keyboard.press("Tab");page.keyboard.press("F1")
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('|guide|')")
        page.keyboard.press("Escape");page.locator("#vr").click()
        page.wait_for_function("()=>window.xrEyes>=4 && xrDepthRegions.size===2")
        page.evaluate("""()=>{const event=new Event('selectstart');event.inputSource={handedness:'left'};testSession.dispatchEvent(event);}""")
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('|orders|')")
        page.evaluate("()=>testSession.end()")
        page.keyboard.press("Escape")
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('|field|4|')")
    assert not errors,errors
    context.close()

if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("web",type=Path);parser.add_argument("--chromium")
    args=parser.parse_args()
    handler=functools.partial(Quiet,directory=str(args.web.resolve().parent))
    with http.server.ThreadingHTTPServer(("127.0.0.1",0),handler) as server, sync_playwright() as pw:
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        url=f"http://127.0.0.1:{server.server_port}/{args.web.resolve().name}/"
        browser=pw.chromium.launch(executable_path=args.chromium,headless=True,
            args=["--use-gl=angle","--use-angle=swiftshader","--enable-unsafe-swiftshader","--disable-dev-shm-usage"])
        try:
            exercise(browser,url);exercise(browser,url,True)
        finally:
            browser.close();server.shutdown()
    print("Desktop WebGL, touch layout and emulated stereo XR passed.")
