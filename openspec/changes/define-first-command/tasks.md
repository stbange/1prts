# First playable milestones

These implementation milestones take the Orra Basin campaign from a ground battle to the shipping lanes and then into orbit.

## 1. Boot the portable runtime

- [ ] 1.1 Create CMake targets for the C11/SDL3/OpenGL host, the headless simulation, Lua 5.4 and the Emscripten port.
- [ ] 1.2 Load a manifest, a scripted mission and a visual asset from the sibling folders on all targets. Exercise a non-default working directory and a website served at a subpath.
- [ ] 1.3 Add the unit runners, source inventories and per-language 95% line-coverage gates before adding gameplay systems.

## 2. Build the two ground forces

- [ ] 2.1 Author the two faction rosters, terrain permissions, building plans and production costs as external Lua content. Give each army original silhouettes, markings, crew voices and credits.
- [ ] 2.2 Build the Survey Hall, Dynamo House, Plate Mill, Watch House, Motor Hall and gate station as staffed, walkable spaces. Model both sections, the Mole, Lintel and Wake at metre scale.
- [ ] 2.3 Walk from the inspection gantry into a produced vehicle and issue group orders from aboard. Test occupied seats, moving frames, report ages, radio captions and grayscale recognition.

## 3. Deliver Gate Six

- [ ] 3.1 Ship surveyed pads, finite haul cycles, stock reservations, grid priority and staffed training and vehicle queues. Check cancellation, duplicate confirmations and a power interruption.
- [ ] 3.2 Script a rival that pays for its units, takes terrain-appropriate routes and sends its infantry into the station. Exercise the 60-second hold, interruption and either faction winning.
- [ ] 3.3 Replay infantry-first and armor-first openings at different render rates. Win from the yard and from aboard a produced vehicle. Verify the rival can win against an idle player.
- [ ] 3.4 Change the gate hold and a vehicle livery with external pack overrides. Repeat on native and web with the same respective binaries.

## 4. Exercise every target

- [ ] 4.1 Run rendered walk, board and order scenarios on Windows, Linux, macOS and desktop web. Capture state and a frame on failure.
- [ ] 4.2 Complete the mission in the automatic mobile touch layout. Exercise orientation change, simultaneous move/look and cancellation.
- [ ] 4.3 Complete the mission with emulated WebXR input. Exercise entry, stereo views, exit and flat fallback. Check scale, text and motion comfort on a headset.
- [ ] 4.4 Run strict OpenSpec validation, both opening replays, rendered tests and the independent unit coverage gates. Playtest factory feedback and the view from a moving tank.

## 5. Take the campaign to the water, air and orbit

- [ ] 5.1 Deliver Tide Window with the depot, relay, rival berth, both ferries and nested crawler boarding. Replay both landings, the unspent-alloy route, a late arrival and a missed tide.
- [ ] 5.2 Introduce the remaining ground roster and faction facilities. Play a Compact operation from its commander's eyes, move a working Slip Shed to a new mooring and exercise an Assembly priority job on reserve power.
- [ ] 5.3 Introduce the crewed patrol aircraft, cargo tiltrotor and shared Flight Shed crew. Exercise a sortie competing with a cargo arrival and the interior, moving-frame and remote command scenarios during flight.
- [ ] 5.4 Add a crewed spacecraft and orbital station. Walk the docking transfer and receive progress from an active surface job while aboard.
