# First playable milestones

These are implementation tasks for the proposed game. The initial concept commit supplies the design and acceptance scenarios.

## 1. Boot the portable runtime

- [ ] 1.1 Create CMake targets for the C11/SDL3/OpenGL host, the headless simulation, Lua 5.4 and the Emscripten port.
- [ ] 1.2 Load a manifest, a scripted mission and a visual asset from the sibling folders on all targets. Exercise a non-default working directory and a website served at a subpath.
- [ ] 1.3 Add the unit runners, source inventories and per-language 95% line-coverage gates before adding gameplay systems.

## 2. Build the dock-to-bridge walk

- [ ] 2.1 Give the depot, relay hall, crawler and both ferries metre-scale portals, traversable rooms and working crew. Reuse the ferry hull for the rival.
- [ ] 2.2 Walk from the dock to the crawler to the ferry bridge, sail and disembark. Exercise nested transforms, blocked hatches and sector reloads.
- [ ] 2.3 Add low-poly geometry, directional sprites, sound cues, captions and legible in-world signs. Record original art credits with the shipped pack.

## 3. Deliver Tide Window

- [ ] 3.1 Bind slate, radio and command actions to crew queues. Implement report age, acknowledgments, cancellation and blocked-job reasons.
- [ ] 3.2 Script the resource reservations, both investments, rival berth reservation, hauls, installations and exact deadline.
- [ ] 3.3 Run seeded games for the hoist route, the slip route, a wait at the quay and a missed tide. Complete an operation from shore and another while riding. Check results at different render cadences.
- [ ] 3.4 Change the deadline and livery with external pack overrides. Repeat on native and web with the same respective binaries.

## 4. Exercise every target

- [ ] 4.1 Run rendered walk, board and order scenarios on Windows, Linux, macOS and desktop web. Capture state and a frame on failure.
- [ ] 4.2 Complete the mission in the automatic mobile touch layout. Exercise orientation change, simultaneous move/look and cancellation.
- [ ] 4.3 Complete the mission with emulated WebXR input. Exercise entry, stereo views, exit and flat fallback. Check scale, text and motion comfort on a headset.
- [ ] 4.4 Run strict OpenSpec validation, both route replays, rendered tests and the independent unit coverage gates. Playtest the command and wayfinding questions in the concept.

## 5. Extend beyond the first playable

- [ ] 5.1 Introduce a crewed aircraft and an airfield delivery. Pass the same interior, moving-frame and remote command scenarios during flight.
- [ ] 5.2 Add a crewed spacecraft and orbital station. Walk the docking transfer and receive progress from an active surface job while aboard.
