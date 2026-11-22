/* Loose assets are fetched relative to this deployment, including subpaths. */
const saveFiles=[["command.sav","previous.sav"],["reserve.sav","reserve-prev.sav"],["checkpoint.sav","checkpoint-prev.sav"]];
function showLoadFailure(error) {
  const status=document.getElementById("status");
  status.textContent=String(error);
  status.hidden=false;
  const loading=document.getElementById("loading"), retry=document.getElementById("retry");
  if(loading)loading.hidden=false;
  if(retry){retry.hidden=false;retry.onclick=()=>location.reload();}
}
var Module = {
  onAbort: function(reason) {
    Module.stopped=true;
    const build=document.getElementById("build")?.textContent || "startup";
    const message="Build "+build+" / Web / "+(Module.currentMission || "startup")+" / "+String(reason).slice(0,220);
    const desk=document.getElementById("desk");
    if(desk)desk.hidden=true;
    try { localStorage.setItem("orra-failure.txt",message); } catch (_) {}
    showLoadFailure(message);
  },
  storePreference: function(name) {
    try {localStorage.setItem("orra-"+name,Module.FS.readFile("/orra-saves/"+name,{encoding:"utf8"}));} catch (_) {}
  },
  canvas: document.getElementById("canvas"),
  persistSlots: function () { Module._orra_saved(0); },
  printErr: function (text) {
    console.error(text);
    if (String(text).startsWith("Startup:")) {
      showLoadFailure(String(text)+" / A WebGL 2 browser or a native build is required.");
    }
  },
  preRun: [function () {
    const key = "orra-content";
    Module.addRunDependency(key);
    async function read(path, mode) {
      if (typeof path !== "string" || !/^[\w./-]+$/.test(path) || path.startsWith("/") ||
          path.includes("..") || path.includes("//")) throw Error("Invalid content path " + path);
      let response;
      try {response=await fetch(new URL(path, document.baseURI));}
      catch(error){throw Error(path+" / "+error.message);}
      if (!response.ok) throw Error(path + " returned " + response.status);
      return response[mode]();
    }
    (async function () {
      if (typeof localStorage!=="undefined") {
        Module.FS.mkdirTree("/orra-saves");
        try {for(const name of ["preferences.txt","failure.txt","recent.txt"]){
          const value=localStorage.getItem("orra-"+name);
          if(value!==null) Module.FS.writeFile("/orra-saves/"+name,value);
        }} catch (_) {}
      }
      if (typeof indexedDB !== "undefined") await prepareSaves(Module,indexedDB);
      const manifest = await read("content.json", "json");
      Module.FS.writeFile("content.json", JSON.stringify(manifest));
      const paths = new Set(manifest.files);
      for (const directory of manifest.packs || []) {
        const pack = await read(directory + "/pack.json", "json");
        Module.FS.mkdirTree(directory);
        Module.FS.writeFile(directory + "/pack.json", JSON.stringify(pack));
        for (const child of [pack.entry, ...pack.files]) paths.add(directory + "/" + child);
      }
      const inflight = new Map(), ready = new Set();
      let completed=0, bootTotal=0;
      async function fetchFile(path) {
        const bytes = await read(path, "arrayBuffer");
        const slash = path.lastIndexOf("/");
        if (slash >= 0) Module.FS.mkdirTree(path.slice(0, slash));
        Module.FS.writeFile(path, new Uint8Array(bytes));
        if(bootTotal) document.getElementById("status").textContent="Preparing command post / "+(++completed)+" of "+bootTotal+" files";
      }
      function load(path) {
        if (!inflight.has(path)) inflight.set(path,fetchFile(path).catch(error=>{
          inflight.delete(path);
          throw error;
        }));
        return inflight.get(path);
      }
      const sectors = new Map();
      if (paths.has("assets/sectors.txt")) {
        const index = await read("assets/sectors.txt","text");
        for (const row of index.trim().split(/\r?\n/)) {
          const [id,...names]=row.trim().split(/\s+/);
          sectors.set(id,names.map(name=>"assets/models/"+name+".mesh"));
        }
        Module.FS.mkdirTree("assets");
        Module.FS.writeFile("assets/sectors.txt",index);
        paths.delete("assets/sectors.txt");
      }
      const meshes=[...paths].filter(path=>path.startsWith("assets/models/"));
      async function sector(id) {
        await Promise.all((sectors.get(id)||meshes).map(load));
        ready.add(id);
      }
      Module.invalidateSector=id=>{
        if(ready.delete(id))for(const path of sectors.get(id)||meshes)inflight.delete(path);
      };
      Module.requestSector=(id,ticket)=>{
        if (ready.has(id)) return 1;
        sector(id).then(()=>Module._orra_sector_ready(ticket,1),error=>{
          Module.lastAssetError=String(error); Module._orra_sector_ready(ticket,0);
        });
        return 0;
      };
      const initial=new Set();
      if (sectors.size && Module.FS.analyzePath) for (const slot of saveFiles.flat()) {
        const path="/orra-saves/"+slot;
        if (Module.FS.analyzePath(path).exists) {
          const header=new TextDecoder().decode(Module.FS.readFile(path).subarray(0,96));
          const match=/^ORRA1 [\da-f]{8}\n([\w-]+) /.exec(header);
          if (match) initial.add(match[1]);
        }
      }
      bootTotal=new Set([...paths].filter(p=>!p.startsWith("assets/models/")).concat(["gate-six",...initial].flatMap(id=>sectors.get(id)||meshes))).size;
      await Promise.all([...paths].filter(path=>!path.startsWith("assets/models/")).map(load)
        .concat(sector("gate-six")));
      await Promise.allSettled([...initial].filter(id=>id!=="gate-six").map(sector));
      bootTotal=0;
      Module.removeRunDependency(key);
    })().catch(showLoadFailure);
  }],
  onRuntimeInitialized: function () {
    document.getElementById("status").hidden = true;
    const loading=document.getElementById("loading");
    if(loading) loading.hidden=true;
    if(typeof installDesk==="function") setTimeout(()=>{
      document.getElementById("build").textContent=Module.FS.readFile("build-id.txt",{encoding:"utf8"}).trim();
      installDesk(Module,document,window);
      if(typeof installProfile==="function")installProfile(Module,document,window);
    },0);
    Module.canvas.addEventListener?.("webglcontextlost",e=>{
      e.preventDefault();
      Module._orra_interrupt();
      Module.onAbort("Graphics context lost. Reload and continue from a saved watch.");
    },{once:true});
    Module.canvas.focus();
    configureXR(Module, navigator.xr, document.getElementById("vr"));
  }
};

async function prepareSaves(runtime, database) {
  const folder="/orra-saves", keys=saveFiles.flat();
  const fs=runtime.FS;
  fs.mkdirTree(folder);
  runtime.persistSlots=()=>runtime._orra_saved(0);
  function open() {
    return new Promise((resolve,reject)=>{
      const request=database.open("orra-basin-campaign",1);
      request.onupgradeneeded=()=>request.result.createObjectStore("slots");
      request.onsuccess=()=>resolve(request.result);
      request.onerror=()=>reject(request.error);
      request.onblocked=()=>reject(Error("Save database is open in another version."));
    });
  }
  try {
    const db=await open();
    const load=(selected=keys)=>new Promise((resolve,reject)=>{
      const tx=db.transaction("slots","readonly");
      for (const key of selected) {
        const req=tx.objectStore("slots").get(key);
        req.onsuccess=()=>{
          if (req.result) fs.writeFile(folder+"/"+key,req.result);
          else if (fs.analyzePath(folder+"/"+key).exists) fs.unlink(folder+"/"+key);
        };
      }
      tx.oncomplete=resolve;tx.onerror=tx.onabort=reject;
    });
    await load();
    runtime.persistSlots=(slot=0)=>{
      const selected=saveFiles[slot] || saveFiles[0];
      try {
        const tx=db.transaction("slots","readwrite");
        for (const key of selected) {
          const path=folder+"/"+key;
          if (fs.analyzePath(path).exists) tx.objectStore("slots").put(fs.readFile(path),key);
          else tx.objectStore("slots").delete(key);
        }
        tx.oncomplete=()=>runtime._orra_saved(1);
        tx.onabort=()=>load(selected).finally(()=>runtime._orra_saved(0));
      } catch (_) { runtime._orra_saved(0); }
    };
  } catch (_) { /* Flat play is available when browser storage is disabled. */ }
}
async function configureXR(runtime, xr, button) {
  if (!xr || !await xr.isSessionSupported("immersive-vr").catch(() => false)) return;
  button.hidden = false;
  button.onclick = async function () {
    let session, space, temp = 0, active = false;
    const held = new Map(), reconnected=new Set();
    let last = 0, snap = false, eyeHeight = 0, pointer = null, aim = null;
    function end() {
      if (active) runtime._orra_xr_mode(0);
      if (temp) runtime._free(temp);
      active = false; temp = 0; button.disabled = false; runtime.canvas.focus();
    }
    try {
      button.disabled = true;
      session = await xr.requestSession("immersive-vr", { optionalFeatures: ["local-floor"] });
      const gl = runtime.canvas.getContext("webgl2");
      await gl.makeXRCompatible();
      const layer = new XRWebGLLayer(session, gl);
      session.updateRenderState({ baseLayer: layer });
      try { space = await session.requestReferenceSpace("local-floor"); }
      catch (_) { space = await session.requestReferenceSpace("local"); eyeHeight = 1.72; }
      if (eyeHeight) space = space.getOffsetReferenceSpace(new XRRigidTransform({y: -eyeHeight}));
      temp = runtime._malloc(192);
      session.addEventListener("end", end, { once: true });
      const grips=new Set();
      function interrupt() {
        held.clear();grips.clear();
        for(const input of session.inputSources) reconnected.add(input);
        aim=pointer=null;last=0;snap=false;
        runtime._orra_interrupt();
      }
      session.addEventListener("visibilitychange",()=>{
        if(session.visibilityState!=="visible")interrupt();
      });
      session.addEventListener("inputsourceschange",e=>{
        for(const input of e.added || [])reconnected.add(input);
        if(e.removed?.length)interrupt();
      });
      session.addEventListener("selectstart", e => {
        runtime.profileInput?.();
        const actuator=e.inputSource.gamepad?.hapticActuators?.[0];
        if(actuator && (runtime._orra_menu_flags()&32)) actuator.pulse(.18,35).catch(()=>{});
        if (e.inputSource.handedness === "left") runtime._orra_xr_action(0);
        else if (runtime._orra_xr_binding?.(2)!==2 && runtime._orra_xr_binding) runtime._orra_xr_action(2);
        else if (!(pointer && runtime._orra_xr_pick(pointer[0],pointer[1])) &&
                 !(aim && runtime._orra_xr_mark(aim[12],aim[13],aim[14],-aim[8],-aim[9],-aim[10])))
          runtime._orra_xr_action(2);
      });
      session.addEventListener("squeezestart", e => {
        grips.add(e.inputSource.handedness);
        runtime._orra_xr_action(grips.size===2?11:e.inputSource.handedness==="left"?1:3);
      });
      session.addEventListener("squeezeend",e=>grips.delete(e.inputSource.handedness));
      runtime._orra_xr_mode(1); active = true;
      function frame(time, xrFrame) {
        session.requestAnimationFrame(frame);
        const pose = xrFrame.getViewerPose(space);
        if (!pose) { if(last)interrupt(); aim = pointer = null; runtime._orra_xr_wrist(0); return; }
        let move = 0, strafe = 0, turn = 0, wrist = null, ray = null;
        pointer = null;
        for (const input of session.inputSources) {
          if (xrFrame.getPose && input.handedness === "left" && input.gripSpace)
            wrist = xrFrame.getPose(input.gripSpace, space)?.transform.matrix;
          if (xrFrame.getPose && input.handedness === "right" && input.targetRaySpace)
            ray = xrFrame.getPose(input.targetRaySpace, space)?.transform.matrix;
          const pad = input.gamepad;
          if (!pad) continue;
          const axes = pad.axes.slice(-2);
          const left = input.handedness === "left";
          const bindings = left ? [4, 5, 9, 6] : [1, 8, 7, 10];
          const mask = (pad.buttons[4]?.pressed ? 1 : 0) | (pad.buttons[5]?.pressed ? 2 : 0) | (pad.buttons[3]?.pressed ? 4 : 0) | (pad.buttons[6]?.pressed ? 8 : 0);
          if(reconnected.delete(input))held.set(input,mask);
          for (let i = 0; i < bindings.length; i++)
            if ((mask & (1 << i)) && !((held.get(input) || 0) & (1 << i))) runtime._orra_xr_action(bindings[i]);
          held.set(input, mask);
          if (left) { strafe = axes[0] || 0; move = -(axes[1] || 0); }
          else {
            const horizontal = axes[0] || 0;
            if (!snap && Math.abs(horizontal) > .65) turn = Math.sign(horizontal) * Math.PI / 6;
            snap = Math.abs(horizontal) > .4;
          }
        }
        aim = ray;
        if (wrist) {
          const matrix = new Float32Array(wrist);
          for (let j = 0; j < 3; j++) matrix[12+j] += .19 * matrix[4+j] - .07 * matrix[8+j];
          runtime.HEAPF32.set(matrix, temp/4+32);
          runtime._orra_xr_wrist(temp+128);
          if (ray) pointer = slatePoint(matrix, ray);
        } else runtime._orra_xr_wrist(0);
        runtime._orra_xr_tick(last ? (time - last) / 1000 : 0, move, strafe, turn);
        last = time;
        gl.bindFramebuffer(gl.FRAMEBUFFER, layer.framebuffer);
        for (const view of pose.views) {
          const v = layer.getViewport(view), position = view.transform.position;
          runtime.HEAPF32.set(view.projectionMatrix, temp / 4);
          runtime.HEAPF32.set(view.transform.inverse.matrix, temp / 4 + 16);
          runtime._orra_xr_eye(temp, temp + 64, v.x, v.y, v.width, v.height, position.x, position.y, position.z);
        }
        runtime.profileFrame?.(true);
      }
      session.requestAnimationFrame(frame);
    } catch (error) {
      if (session) await session.end().catch(() => {});
      end();
      const status = document.getElementById("status"); status.textContent = String(error); status.hidden = false;
    }
  };
}

function slatePoint(slate, ray) {
  const dot = (a,b) => a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
  const normal=slate.slice(8,11), direction=ray.slice(8,11);
  const offset=[ray[12]-slate[12],ray[13]-slate[13],ray[14]-slate[14]];
  const angle=dot(direction,normal);
  if (Math.abs(angle)<.0001) return null;
  const distance=dot(offset,normal)/angle;
  if (distance<0) return null;
  const point=offset.map((v,i)=>v-distance*direction[i]);
  const x=.5+dot(point,slate.slice(0,3))/.52, y=.5-dot(point,slate.slice(4,7))/.44;
  return x>=0 && x<=1 && y>=0 && y<=1 ? [x,y] : null;
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { Module, configureXR, slatePoint, prepareSaves, showLoadFailure };
}
