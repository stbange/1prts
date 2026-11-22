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
READY = "()=>document.getElementById('status').hidden && typeof Module!=='undefined' && typeof Module._orra_status==='function' && Module.UTF8ToString(Module._orra_status()).startsWith('active|')"
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
        page.evaluate("""([kind,hand])=>{const e=new Event(kind);e.inputSource={handedness:hand};testSession.dispatchEvent(e);
            if(kind==='squeezestart'){const up=new Event('squeezeend');up.inputSource=e.inputSource;testSession.dispatchEvent(up);}}""",[kind,hand])
    def touch(x,y):
        history.append(["tap",x,y]);page.touchscreen.tap(x,y)
    def button(name):
        target=page.get_by_role("button",name=name,exact=True)
        if layout=="touch": target.tap()
        else: target.click()
        advance(0)
    def start():
        button("Start Gate Six")
        button("Report for duty")
    def accept_switch():
        if status()[1]=="desk":
            if layout=="xr":
                xr_event("squeezestart","left");xr_event("selectstart","right");advance(0)
            else: button("Confirm")
    def toggle():
        if layout=="touch": button("Close" if status()[1]=="orders" else "Orders")
        elif layout=="xr": xr_event("selectstart","left")
        else: page.keyboard.press("Tab")
        advance(0)
    def order(index):
        history.append(["order",index])
        if layout=="touch": page.locator("#rows button").nth(index-1).tap()
        elif layout=="xr":
            for _ in range((index-1-int(status()[6])) % int(status()[18])): xr_event("squeezestart","left")
            xr_event("selectstart","right")
        else: page.keyboard.press(str(index))
        advance(0)
    try:
        page.goto(url+"?replayClock=1")
        page.wait_for_function(READY)
        assert not any(url.endswith("/station.mesh") for url in requests),requests
        print("boot",layout,pack,page.evaluate("()=>({ms:Math.round(performance.now()),heap:Module.HEAPF32.byteLength,assets:performance.getEntriesByType('resource').length})"),flush=True)
        assert status()[1]=="desk" and int(status()[3])==0
        advance(120);assert int(status()[3])==0
        start()
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
            button("Crews");page.locator("#rows button").first.tap();advance(0)
            draft=status();assert draft[1]=="assignment",draft
            page.set_viewport_size({"width":540,"height":900})
            page.wait_for_function("()=>document.querySelector('canvas').height>document.querySelector('canvas').width")
            assert status()[5]=="1" and status()[16:18]==draft[16:18] and status()[1]=="assignment"
            page.set_viewport_size({"width":900,"height":540})
            page.wait_for_function("()=>document.querySelector('canvas').width>document.querySelector('canvas').height")
            button("Close");button("Close")
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
                button("Continue saved operation")
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
        if pack:
            page.keyboard.press("m");advance(0);page.keyboard.press("8");advance(0);accept_switch()
            page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).split('|')[7]==='wire-quay'")
            page.keyboard.press("e");advance(0)
            assert int(status()[8])>=0 and status()[20]=="quay-courier",status()
            page.keyboard.press("F5");advance(0)
            page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('saved in this browser')")
            page.reload();page.wait_for_function(READY)
            button("Continue saved operation")
            assert status()[7]=="wire-quay" and int(status()[8])>=0 and any(p.endswith("courier.mesh") for p in requests)
        if layout=="xr":
            assert int(status()[8])>=0
            targets=[("gate-six","mole",-8,1,2),("tide-window","kestrel",0,1,0),
                     ("westbound","shunter",-1,2,-3),("white-mile","barrow",-1,2,-3),
                     ("night-mail","kite",0,1,-10),("high-water","lark",0,1,0),
                     ("compact-bank","hinge",-28,2,5)]
            for index,(sector,crew,x,y,z) in enumerate(targets):
                page.keyboard.press("m");advance(0);page.keyboard.press(str(index+1));advance(0);accept_switch()
                page.wait_for_function("id=>Module.UTF8ToString(Module._orra_status()).split('|')[7]===id",arg=sector)
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
            if status()[1]!="field": page.keyboard.press("Escape");advance(0)
            assert status()[1]=="field"
        if layout=="touch":
            view=page.locator("canvas").bounding_box()
            center=(view["x"]+view["width"]/2,view["y"]+view["height"]/2)
            touch(*center);advance(0)
            assert status()[0:2]==["victory","desk"]
            button("Cancel");assert status()[0]=="victory"
            touch(*center);advance(0);button("Confirm")
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

def session_acceptance(browser,url):
    from playwright.sync_api import expect
    ctx=browser.new_context(viewport={"width":360,"height":640},has_touch=True,is_mobile=True)
    page=ctx.new_page()
    errors=[]
    page.on("pageerror",lambda e: errors.append(str(e)))
    get=lambda name:page.get_by_role("button",name=name,exact=True)
    def tap(name): get(name).tap()
    def ticks(n): page.evaluate("ticks=>Module._orra_clock(ticks)",n)
    def state(): return page.evaluate(STATUS)
    def heading(name): expect(page.locator("#desk-title")).to_have_text(name)
    try:
        page.goto(url+"?replayClock=1");page.wait_for_function(READY)
        heading("Orra Basin");ticks(60);assert state()[3]=="0"
        for target in page.locator("#rows button").all():
            b=target.bounding_box();assert b and b["height"]>=44 and b["width"]>=44,b
        tap("Start Gate Six");heading("Watch briefing");tap("Report for duty")
        for width,height in ((360,640),(800,450),(360,640)):
            page.set_viewport_size({"width":width,"height":height})
            page.wait_for_function("""()=>{const view=document.querySelector("canvas").getBoundingClientRect();
                const dock=document.getElementById("field-controls").getBoundingClientRect();
                return view.height>150 && dock.height>=44 && view.bottom<=dock.top+1 &&
                       dock.bottom<=innerHeight+1 && dock.width<=innerWidth;}""")
        tap("Orders");tap("Commission Plate Mill");tap("Close");ticks(45)
        original=state()
        tap("Operations");tap("Tide Window");heading("Confirm action");ticks(400)
        assert state()[3]==original[3];tap("Cancel")
        assert state()[7]=="gate-six" and state()[2]=="4"
        tap("Close");tap("Pause");heading("Watch paused")
        tap("Save operation");tap("Watch B / empty")
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).includes('Operation saved in this browser')")
        tap("Pause");tap("Rehearse Gate Six");heading("Field help");tap("Report for duty")
        assert state()[2]=="8";ticks(90);tap("Pause");tap("End practice and return")
        restored=state();assert restored[2:4]==original[2:4] and restored[7:13]==original[7:13]
        tap("Pause");tap("Settings");tap("Interface size: 100%");tap("Interface size: 125%")
        tap("Music volume: 100%");tap("Apply changes");tap("Resume watch")
        tap("Pause");tap("Settings");expect(get("Interface size: 150%")).to_be_visible()
        tap("Preview defaults");tap("Cancel");tap("Resume watch")
        page.reload();page.wait_for_function(READY);tap("Continue saved operation")
        assert state()[2:4]==original[2:4];tap("Pause");tap("Settings")
        expect(get("Music volume: 0%")).to_be_visible();tap("Cancel");tap("Resume watch")
        failed={"active":True}
        def block(route):
            if failed["active"]:route.fulfill(status=503,body="offline")
            else:route.continue_()
        page.route("**/station.mesh",block)
        ticks(10);tap("Operations");tap("High Water");tap("Confirm");heading("Operation unavailable")
        expect(page.locator("#detail")).to_contain_text("station.mesh")
        assert state()[7]=="gate-six";failed["active"]=False;tap("Retry loading")
        page.wait_for_function("()=>Module.UTF8ToString(Module._orra_status()).split('|')[7]==='high-water'")
        page.evaluate("()=>window.dispatchEvent(new Event('blur'))")
        heading("Watch paused");freeze=state()[3];ticks(60);assert state()[3]==freeze
        tap("Credits and release");tap("SDL license");heading("Release document")
        expect(page.locator("#detail")).not_to_be_empty()
        tap("Back");tap("Back");tap("Settings");tap("Control bindings")
        tap("Walk forward: W");page.keyboard.press("q")
        expect(get("Walk forward: Q")).to_be_visible()
        tap("Back");tap("Language: English");tap("Apply changes")
        expect(page.locator("html")).to_have_attribute("lang","ru")
        tap("Настройки");expect(get("Язык: Русский")).to_be_visible()
        tap("Отмена");tap("Вернуться в игру");expect(get("Приказы")).to_be_visible()
        tap("Приказы");expect(get("Commission Plate Mill")).to_have_count(0)
        assert state()[7]=="high-water"
        assert not errors,errors
        print("Verified protected transitions, independent slot, practice return, saved preferences, touch sizes, recovery and background pause.",flush=True)
    finally:
        ctx.close()

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
            session_acceptance(browser,url)
            base=exercise(browser,url,"desktop")
            changed=exercise(browser,url,"desktop",pack=True)
            assert base!=changed,"Expected the pack livery to change the captured frame"
            exercise(browser,url,"touch");exercise(browser,url,"xr")
        finally:
            browser.close();server.shutdown()
    print("Desktop boarding, loose livery and hold overrides, touch and stereo XR victories passed.")
