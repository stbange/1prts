/* Loose assets are fetched relative to this deployment, including subpaths. */
var Module = {
  canvas: document.getElementById("canvas"),
  printErr: function (text) {
    console.error(text);
    if (String(text).startsWith("Startup:")) {
      const status = document.getElementById("status");
      status.textContent = String(text); status.hidden = false;
    }
  },
  preRun: [function () {
    const key = "orra-content";
    Module.addRunDependency(key);
    async function read(path, mode) {
      const response = await fetch(new URL(path, document.baseURI));
      if (!response.ok) throw Error(path + " returned " + response.status);
      return response[mode]();
    }
    (async function () {
      const manifest = await read("content.json", "json");
      await Promise.all(manifest.files.map(async function (path) {
        const bytes = await read(path, "arrayBuffer");
        const slash = path.lastIndexOf("/");
        if (slash >= 0) Module.FS.mkdirTree(path.slice(0, slash));
        Module.FS.writeFile(path, new Uint8Array(bytes));
      }));
      Module.removeRunDependency(key);
    })().catch(function (error) { document.getElementById("status").textContent = String(error); });
  }],
  onRuntimeInitialized: function () {
    document.getElementById("status").hidden = true;
    Module.canvas.focus();
    configureXR(Module, navigator.xr, document.getElementById("vr"));
  }
};

async function configureXR(runtime, xr, button) {
  if (!xr || !await xr.isSessionSupported("immersive-vr").catch(() => false)) return;
  button.hidden = false;
  button.onclick = async function () {
    let session, space, temp = 0, active = false;
    const held = new Map();
    let last = 0, snap = false, eyeHeight = 0;
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
      temp = runtime._malloc(128);
      session.addEventListener("end", end, { once: true });
      session.addEventListener("selectstart", e => runtime._orra_xr_action(e.inputSource.handedness === "left" ? 0 : 2));
      session.addEventListener("squeezestart", e => runtime._orra_xr_action(e.inputSource.handedness === "left" ? 1 : 3));
      runtime._orra_xr_mode(1); active = true;
      function frame(time, xrFrame) {
        session.requestAnimationFrame(frame);
        const pose = xrFrame.getViewerPose(space);
        if (!pose) return;
        let move = 0, strafe = 0, turn = 0;
        for (const input of session.inputSources) {
          const pad = input.gamepad;
          if (!pad) continue;
          const axes = pad.axes.slice(-2);
          if (input.handedness === "left") { strafe = axes[0] || 0; move = -(axes[1] || 0); }
          else {
            const horizontal = axes[0] || 0;
            if (!snap && Math.abs(horizontal) > .65) turn = Math.sign(horizontal) * Math.PI / 6;
            snap = Math.abs(horizontal) > .4;
            const pressed = !!(pad.buttons[4] && pad.buttons[4].pressed);
            if (pressed && !held.get(input)) runtime._orra_xr_action(1);
            held.set(input, pressed);
          }
        }
        runtime._orra_xr_tick(last ? (time - last) / 1000 : 0, move, strafe, turn);
        last = time;
        gl.bindFramebuffer(gl.FRAMEBUFFER, layer.framebuffer);
        for (const view of pose.views) {
          const v = layer.getViewport(view), position = view.transform.position;
          runtime.HEAPF32.set(view.projectionMatrix, temp / 4);
          runtime.HEAPF32.set(view.transform.inverse.matrix, temp / 4 + 16);
          runtime._orra_xr_eye(temp, temp + 64, v.x, v.y, v.width, v.height, position.x, position.y, position.z);
        }
      }
      session.requestAnimationFrame(frame);
    } catch (error) {
      if (session) await session.end().catch(() => {});
      end();
      const status = document.getElementById("status"); status.textContent = String(error); status.hidden = false;
    }
  };
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { Module, configureXR };
}
