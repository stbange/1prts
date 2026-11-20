# Design for the first playable build

## Context

The hard case is a person standing in a crawler on a ferry while three crews do different jobs. The ferry must carry the crawler, the crawler must carry its occupants, and a command issued from inside either one must still reach the depot. This case connects the camera, simulation, content format and test design. [Player-facing concept](../../../docs/concept.md)

## Port mission timing

Time belongs to the simulation. The proposed tuning below makes both investments viable and leaves time to recover from a late sailing. All durations live in the mission's Lua content.

| Activity | Initial duration |
| --- | --- |
| Commission the hoist, then lift both cores into the ferry's deck cradles | 60 s + 35 s |
| Fit the extended ramp while the crawler shuttles two cores into the cradles | 100 s for the fit, concurrent with two 80 s trips |
| Embark the crawler and clear the home berth | 25 s |
| Cross to either island landing | 135 s |
| Discharge cargo and crawler and free the landing | 30 s |
| Haul from the public quay and install both cores | 150 s total |
| Haul from the gravel slip and install both cores | 225 s total |

With immediate orders the hoist route reaches the quay at 04:15 and frees it at 04:45. The ramp route reaches the slip at 05:20, while the rival is at the quay, and energizes the relay at 09:35. A quay arrival must fit the whole discharge into a free berth window. For example, a 04:45 arrival with the standard ramp waits until 08:00 and can still finish at 11:00. Jobs consume stock at the start of work. A canceled queued job releases its reservation. A repeated confirmation must never spend the same stock twice.

## Decisions

### C owns the world and Lua defines the game

Use C11, SDL3 and OpenGL in a CMake build. SDL3 supplies windows, input and audio devices. The C core owns entity identity, fixed ticks, transforms, collision, traversal, path queries, order queues, resource reservations, rendering, audio playback and asset I/O. A renderer can be absent when running a simulation test.

Ship a Lua 5.4 interpreter with the runtime. Plain-text Lua files define unit types and crew roles, legal jobs and costs, build plans, rival decisions, mission events and win/loss conditions. Expose a versioned API for registering content, submitting normal orders, reading world facts available to the caller and subscribing to events. Scripts refer to stable IDs. C checks inputs and applies changes at tick boundaries. Use seeded random numbers and the simulation clock for game rules.

For example, the ferry pilot's Lua job chooses a destination. The C navigation system moves the ferry and carries the people on its deck. On arrival C raises a berth event. The mission decides whether the cargo satisfies an objective. The same event and order path is available to an additional mission pack.

### Ship loose content on every target

The native distribution uses this directory layout on Windows, Linux and macOS.

```text
1stprts[.exe]
content.json
assets/
  models/       # glTF hulls, rooms and collision meshes
  textures/     # PNG palettes, signs and directional sprite sheets
  sounds/       # ambient loops, machinery and radio calls
  shaders/
  CREDITS.md
scripts/
  units/
  jobs/
  missions/tide_window.lua
mods/
  long-tide/    # example optional pack
```

Resolve the native content root from the executable location even when launched from another working directory. Use the same portable directory for the initial macOS build. The browser distribution places `index.html`, the JavaScript loader and the Wasm module beside the same manifest and folders. Resolve asset URLs against that distribution directory, including when hosted below a URL prefix. Fetch the manifest and boot files, then stream sector assets as needed. Original media, shaders and scripts stay individually replaceable. The executable and Wasm module contain the runtime.

The manifest declares a content API version, stable IDs, paths, entry scripts and the ordered list of enabled packs. Load base content, then the listed packs in order. A pack declares each base ID it overrides. Reject an incompatible API version, an undeclared duplicate ID or a missing required file with the pack ID and offending version, ID or path before the mission starts. Re-read the manifest and revalidate changed files on the next launch or page reload.

For a first mod, copy the base mission script into `mods/long-tide/`, change its tide deadline from 720 seconds to 900 and register an override for the base mission in the pack manifest. Enable the pack in `content.json` and relaunch. The briefing clock and the actual loss tick must both move to 15:00 using the same native binary or browser module. A livery pack can likewise substitute one PNG and supply its own credit entry. Project-authored Lua and shaders use MIT. Original art and sound use CC BY 4.0.

### Keep physics local to the deck

Run the authoritative simulation at 30 ticks per second, independently of rendering. Identify a location by sector, parent frame and local pose. Terrain, hulls, vehicles and station modules can supply frames. Moving a ferry updates its hull frame once. Actors aboard retain their local pose while physics resolves their own walking. At a connected hatch, ramp or collar, convert the pose and relative velocity once into the destination frame. Boarding a nested crawler attaches the commander to the crawler's frame.

Give each asset an exterior, a walkable interior graph, compatible entrance connectors, collision surfaces, crew stations and a scale in metres. Prefetch the next compartment near a portal. Keep crew identity, assigned station, cargo and job progress authoritative when a compartment is unloaded. Nearby stations show workers doing the recorded jobs. Distant crews can advance their schedules without animating every hand and foot.

### Render at human scale

Use the common feature set of OpenGL 3.3 core on Windows/Linux, a forward-compatible OpenGL 4.1 core context on macOS, and WebGL 2 through Emscripten's OpenGL ES 3 path. Maintain desktop and ES shader variants as external assets. Sector culling, hull and interior levels of detail, instanced props and distant sprites limit work in a large port. The retro palette and deliberate texture resolution apply to the world. Render slate text at a legible output resolution. In stereo, both eyes see the same positioned sprite and the same slate surface.

### Bind the same actions to each device

| Target | Movement and looking | Giving an order |
| --- | --- | --- |
| Windows, Linux, macOS and desktop web | WASD and mouse look. E operates a door or station. | Tab raises the slate. Select a call sign and job, then point at a target or select a chart marker. Confirm or cancel. |
| Mobile web | Automatically show a left virtual stick, right drag-look area and contextual interact button when a touch device has a coarse primary pointer. Touch input on a hybrid can activate the layout too. | Tap the slate button, recipient, job and target. Give confirm and cancel separate touch targets. Preserve a pending order across rotation or a change of input device. |
| WebXR | Enter an available immersive VR session by user gesture in a secure context. Head tracking sets the view. Thumbstick movement and snap turning support seated or standing play. | Raise a wrist slate, point a controller ray and confirm with the trigger. A controller binding also operates doors and cancels orders. |

The Emscripten bridge requests an XR-compatible WebGL 2 context and uses the XR session's frame callback, per-eye views, projection matrices and input poses. While presenting in XR the session owns rendering and the simulation advances once per tick for both eyes. On exit restore the flat input bindings at the same commander and world state. When immersive VR is unavailable the normal browser game remains playable. Touch and headset users must be able to finish the same mission with their own controls.

## Verification design

| Layer | Method | Evidence required for the playable milestone |
| --- | --- | --- |
| Unit | Drive C via CTest with fake clocks, devices and files. Exercise Lua content with a seeded test host. Unit-test handwritten browser glue with simulated browser APIs. | At least 95% executable-line coverage independently for first-party runtime C, shipped Lua and handwritten browser glue. Merge C source lines across platform configurations with LLVM coverage, use LuaCov for Lua and JS line coverage for the glue. Include never-loaded production files in each denominator. Exclude generated code, third-party code, tests and fixtures. |
| Game simulation | Record ordinary player actions by tick. Replay the same seed and actions against the shipping engine and Lua mission with rendering disabled. | Both routes win, a late crew loses, berth conflicts and canceled reservations behave correctly. Repeated runs at different render cadences agree on cargo, orders, crew, funds, objective and result tick. |
| Rendered game | Drive SDL input on each native OS and browser input on desktop web, a touch-only mobile profile and an emulated WebXR device. | Walk aboard, give an order and observe its effect through the player interface. Complete a touch-only run. In XR check both eyes, operate the slate, complete the run and return to flat play. Keep event logs and a screenshot on failure. |

Measure coverage from unit tests alone. Game tests supply independent evidence that the parts work together. Check stereo scale, text legibility and comfort on a real headset as well as exercising the emulated XR lifecycle in automation. Map test cases to the named scenarios under [specs](specs/).

## First playtest risks

- **Orders may take too much attention.** Test whether a new player can find the delayed crew and its reason from the radio log while moving. Keep recipient, job and target visible through confirmation.
- **Big interiors may turn trips into waiting.** Time the apron-to-bridge walk and ask what the player learned on the way. Put the engine-room window, approach sightline and most-used crew stations on useful routes.
- **Moving decks and stereo amplify small errors.** Exercise a stationary observer, a walking observer and an observer inside the crawler through the same ferry turn. Compare poses in metres and look for drift at the ramp seam.
- **A whole port can overwhelm a phone or headset.** Profile the walk from the open quay into the ferry on both early target devices. Keep collision, crew jobs and command text functioning as distant detail is reduced.
