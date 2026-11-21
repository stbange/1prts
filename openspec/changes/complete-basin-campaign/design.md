# Delivery and acceptance plan

## Starting point

The baseline is `4255587` on `main`. The first-person C11/SDL3/OpenGL host, Lua 5.4 missions, moving parent frames, retro meshes and sprites, editable assets and seven operation replays are already useful foundations. The crucial distinction in the current roster is between a type placed in a scene and a crew that can do the job advertised in the field guide. The roster test checks placement. This plan adds a job test and a traversal test wherever the type has an interior. [Roster](../../../docs/roster.md) · [Existing roster test](../../../tests/campaign_rules.lua)

| Gap observed at the baseline | Starting code and data | Delivery |
| --- | --- | --- |
| Each side can own one produced section and one armored unit. Each side starts with eight private loads. | [Gate Six rules](../../../scripts/gate_six.lua), [C state](../../../src/game.h) | 1–2 |
| Field assignments often follow a straight line. Tally assigned to the gate reports “watch established.” The rival has a fixed production sequence. | [Crew jobs](../../../scripts/fieldwork.lua), [rival](../../../scripts/gate_six.lua) | 3, 6 |
| Numerous craft share the same bridge and service layout. Most facilities use a common shell. The depot footprint is 25 × 21 m. | [Fleet dimensions](../../../assets/fleet.txt), [asset authoring](../../../tools/campaign_art.py), [walking and boarding](../../../src/world.c) | 4 |
| The XR wrist slate works. Its world-pointing action recognizes one location in Gate Six. | [XR targeting](../../../src/xr.c), [browser input](../../../web/boot.js), [slate](../../../src/ui.c) | 5 |
| A mission change starts a fresh simulation. Packs add missions based on registered sectors. | [Mission switch](../../../src/app.c), [sector creation](../../../scripts/scenes.lua), [pack API](../../../docs/modding.md) | 6–7 |
| The newcomer policy lost 40/40 Gate Six runs. The measured software-renderer frame took 22.5 ms at the 95th percentile. Physical headset and first-time player observations are outstanding. | [Campaign playtest](../../../docs/playtests/campaign/README.md), [milestones 4.5 and 4.6](../define-first-command/tasks.md) | Early in 2 and 4, again in 8 |

## Order of delivery

| Section | Earliest useful start | Primary work | Demonstration that closes the milestone |
| --- | --- | --- | --- |
| 1. Identities and baseline | Current build | Engine, rules, tests | Two objects of the same type can be addressed independently while existing missions and the example pack replay correctly. |
| 2. Economy and production | 1 | Simulation, mission and UI | Ride the second Lintel, command both crews, and account for every load in a contested lot. |
| 3. Tactics and opponent | Start routing after 1. Integrate the rival with 2. | Gameplay and AI | An escorted hauler changes route, a disabled occupied hull is recovered, and either faction can win the gate. |
| 4. Inhabited world | Start the depot and lift after 1. Feed finished routes to 5 and 6 in batches. | World systems, art, audio | Enter and leave every crewed asset through its normal entrance and observe its defining work. |
| 5. Targeting | Prototype after 1. Integrate 3 and representative connectors from 4. | Input, UI, XR | The same order reaches the same crew from a mouse, a touch screen and a tracked controller. |
| 6. Roster and operations | Convert Gate Six after 2–3. Integrate each later sector as its rooms and commands arrive. | Mission and content | The seven operation rows below pass through ordinary player controls and an external pack can add a sector. |
| 7. Persistence | Prototype the snapshot after 1. Integrate changing state from 2–6. | Engine I/O, UI, mission rules | Quit on a moving ferry, resume on the same deck, finish and advance with the correct roster. |
| 8. Evaluation and release | Collect baselines now. Sign off on integrated builds. | Testing, art capture, player and device sessions | Downloadable packages, coverage reports, gameplay frames and both observed playtests match the release revision. |

The first integration build is sections 1 and 2 at Gate Six. The second is the depot-to-ferry crossing with shared targeting and a saved boarding state. Completing the campaign then becomes a sequence of playable sectors. For estimating content work, time a small hull, the depot and the Crown lift through modeling, collision, crew placement and replay capture. Use those measured times and the remaining assets by family to forecast the rest.

## Simulation and content decisions

**First-person strategy.** Use the 1995 *Command & Conquer* sequence of hauler arrival, resource payout, factory rollout and a contested objective as the pacing reference. Let the commander witness each event or hear it reported by a named crew. Construction, group selection, tactical charts, saving and debriefing all stay at the commander's eyes. Base the factions, silhouettes, symbols, maps, voices and music on Orra Basin's original fiction.

**Object identity and state.** Keep type IDs such as `lintel` in the catalog. Allocate a stable instance ID to each produced hull, worker, group, lot and facility. A parent frame, target, seat assignment or reservation references an instance. Store a deterministic spawn counter with the mission. The C host owns transforms, collision and path queries, renderer and storage I/O. Lua remains authoritative for recipes, prices, diplomacy, crew qualifications, rival choices and objectives. Adapt the existing scene and snapshot interfaces incrementally. Keep one authority for each piece of state as it migrates. [C/Lua bridge](../../../src/sim.c) · [Runtime and pack loader](../../../scripts/runtime.lua)

**Economy and orders.** Keep eight starting alloy and enough home plate for either teaching opening. Place the additional contested load where both factions have a legal route. Track a load from source reservation through pickup, transport, delivery and processing. Tag each accepted player command with an ID. A retry, stock conflict, blocked rollout or restored save can then resolve one payment and one spawn. Sort same-tick claims by a recorded, deterministic rule. Keep the physical pad, factory queue, slate and radio synchronized. Assign power per facility and expose the reason a job is waiting.

**Movement and information.** Author connected routes and costs in sector data. Use shared world queries for legal terrain, clearance, occupied passages, transport handoffs and docking. Replan when a span opens or a berth becomes unavailable. Represent a visible target by instance and a reported target by last-known location, observer and tick. An escort follows the assigned entity. A support order acts on the reported area. The opponent receives the same scoped information, job results and path refusals that a player receives. Terrain and encounter outcomes use original, abstract game rules.

**Rooms and equipment.** Use the same metre-scale anchors for the visual entrance, collision opening, nav links and interaction ray. Give desks, machines, crew seats, lift platforms and doors collision where their geometry occupies space. Conditional connectors check berth alignment, door state and lift stop before transfer. Convert the commander's pose and relative motion across frames at the crossing. Use a 1.72 m standing eye height and the concept's roughly 2.1 m personnel doorway as on-foot references. Qualify cargo openings with the actual loaded hull. Ship rooms, collision, crew stations, palette, signs, sound and scripts beside the executable. Voxel dressing remains optional. [Scale references](../../../docs/concept.md#a-world-built-at-crew-scale)

**Targeting.** A field order carries the crew or group, verb, typed destination and command ID. Show a first-person highlight and caption on the selected target. A stale contact shows its age on the chart. On confirm, validate the target again at the next simulation tick and acknowledge success or the reason for refusal. The slate, mouse, touch and controller ray use that same path. UI hits take precedence over the scene behind the slate. Canceling clears only the draft. [Existing field command requirements](../define-first-command/specs/field-command/spec.md)

**Persistence and campaign rules.** The proposed route is Gate Six → Tide Window → Westbound → White Mile → Night Mail → High Water. Gate Six / Compact is an independently playable side operation with its own faction record. Keep free operation selection for practice. Winning writes a result once and offers Continue. Retry restores the operation entry state. A campaign transfer carries the eligible instance, owner, crew, condition and installed upgrades. A compatible incoming unit fills an authored slot. A fresh or depleted roster gets the mission's stated minimum.

For the first transfer, let the surviving Assembly Mole and its crew fill Tide Window's crawler slot. Depot upgrades belong to the depot, ferry upgrades to the ferry. The proposed equipment handover uses existing infrastructure. Night Mail already has a Shore Depot. Give its optional ground-loading job a supply link to Tide Window's depot so a previously commissioned Quay Hoist speeds that handoff. The Flight Shed still has to receive and dispatch all three bags. In the linked-operation pack fixture, deploy the same Kestrel with its Extended Bow Ramp to a compatible gravel landing. Test that the Compact side operation cannot draw Assembly stock and that replaying a victory cannot duplicate a hull or upgrade.

Save only declarative, versioned data. Record the campaign and sector IDs, content and enabled pack identities/versions, RNG state, simulation tick and commander frame alongside the mission data. Keep a previous committed save while writing the next one, including across browser storage transactions. Stage restoration before switching the active world. Specify migration fixtures for each published save schema and explain an incompatible pack or unavailable storage on the slate. A clean install starts with an empty campaign ledger.

## Roster acceptance

Maintain one row of evidence per shipped ID. A unit row identifies its sector, owner, order, tangible effect and refusal condition. A craft or structure row also records entrance, occupied operating station, second functional area, exit, moving parent where relevant, and a replay or frame. Reuse building components and hull fittings where they serve the same purpose. Lay out each route around the work a player can observe. The 69 catalog entries below comprise 31 craft, eight personnel types and 30 structures. [Catalog](../../../scripts/catalog.lua) · [Field guide](../../../docs/field-guide.md)

| Unit batch | Types to qualify | Representative proof |
| --- | --- | --- |
| Shared crews and transport | Fitting Crew, Mole, Kestrel, Porter, Sump, Kite, Lark, Tag Crew, Dray | Mark a lot, haul its stock, deliver operators and a building section, drain an entrance, and transfer crew or cargo by ferry, tiltrotor and tender. |
| Weir Assembly | Gate Section, Lintel, Plumb, Span, Tally | Occupy the room, contest the yard, refresh a survey report, open a crossing while the Span is committed, support a reported yard. |
| Sounding Compact | Channel Section, Wake, Reed, Hinge, Crossbill | Embark a section, sound a channel, tow a disabled hull, and return a patrolling crew for service. |
| Terrace Railway | Points Crew, Shunter, Anvil, Sleeper, Whistle | Discover a blockage, lay track, set a switch, hold a siding and bring the wagon through. |
| Salt Survey | Chain Crew, Glint, Barrow, Prism, Shoe | Mark control points, establish a bearing, traverse soft ground and deliver an identified sample. |
| Lantern Pilots | Ground Watch, Tern, Gimbal, Gantry, Rook | Secure an office, set an approach beacon, fly a delivery, recover a disabled aircraft and return its observer. |
| Meridian Dock Trust | Collar Watch, Cleat, Spindle, Rivet, Loom | Staff a docking office, tow a module, transfer cargo, repair a joint and transport the incoming shift. |

| Facility batch | Types to qualify | Representative proof |
| --- | --- | --- |
| Ground yard and workers | Survey Hall, Dynamo House, Plate Mill, Watch House, Motor Hall, Yard Post, Plate Store, Shift Infirmary, Crew Quarters, Signal Loft | Stand at the relevant desk or machine and trace a shipment, crew change, defense report or power interruption back to the responsible station. |
| Shore and trade | Shore Depot, Island Relay, Pump Room, Slipway, Tug Mooring, Transit Office | Walk the entire 65 m depot service route, see a loaded vessel enter service, queue a berth and observe relief of a flooded entrance. |
| Rail, air and orbit | Freight Siding, Flight Shed, Beacon House, Pressure Lock, Transfer Spaceport | Follow physical freight, apron and lock handoffs. Observe a delayed arrival and its crew's next available slot. |
| Faction facilities | Accumulator Gallery, Slip Shed, Roundhouse, Assay Office, Approach Tower, Pressure Cradle | Demonstrate reserve power, tow and recommission a shed, refit a rail vehicle, assay a lot, schedule arrivals and service an occupied module. |
| Objectives | Gate Six Station, West Exchange, Crown Station | Take the actual switch room, reroute winter freight and ride the traffic-control lift. Test a rival occupant or interrupted handover where the mission permits it. |

## Operation acceptance

| Operation | Choice the commander can make | Observable replay outcome |
| --- | --- | --- |
| Gate Six | Build infantry first or armor first. Commit a second crew to escort a Mole or contest the yard. | Both openings can win. Interrupt a claim at 1,799 ticks and demand a fresh 1,800. An unopposed rival can win with paid units. |
| Tide Window | Buy the Quay Hoist, fit the Extended Bow Ramp or keep all eight alloy and wait for the public quay. | Trace both cores and the loaded crawler across the selected landing. The standard route can finish at 11:00. An installation first finishing at 12:00 loses under base rules. |
| Gate Six / Compact | Take an early repair or tow the crewed Slip Shed to the new bank. Choose when to dispatch the Wake and its section. | Begin aboard the shed, suspend service under tow, land the same passengers, and resume repairs at the new berth. A missed handover can cost the deadline. |
| Westbound | Buy the Points Heater or sequence repair and reserve power around the outage. | The switch and track must admit the Shunter and freight wagon. A held siding delays the delivery. |
| White Mile | Fit Wide Shoes for a loaded shortcut or use the firm approach. Decide when to drain the camp and identify the sample. | The loaded route changes with ground permissions. The returned sample and its assay complete the objective. |
| Night Mail | Install the Night Board or sequence courier, cargo and patrol work around a shared apron. | All three bags have unique deliveries. A late patrol occupies a real crew or slot and can cost the final bag before dawn. In campaign play, compare the optional ground handoff with and without the upstream depot's hoist. |
| High Water | Fit the Second Collar for overlapping work or sequence the tug and tender at one collar. | Dock, walk into Crown Station, ride the lift and receive a live surface report. A closed joint or an occupied collar holds the handoff. |

## Evidence for release

Keep coverage evidence separate from rendered evidence. The baseline recorded 17 passing native tests and independent unit line coverage of 96.19% for the compiled native C configuration, 98.40% for shipped Lua and 100% for handwritten browser JavaScript. Extend source inventories as modules and platform adapters are introduced. Merge coverage by source line across applicable native and Emscripten configurations so platform-only code is accounted for. [Coverage runner](../../../tools/coverage.py) · [CI workflow](../../../.github/workflows/game.yml)

On each native target replay the seven operations. In Chromium exercise desktop, touch-only and emulated XR entry, world targeting, cancel and fallback. Compare common actions across inputs and compare deterministic state at 30 and 90 Hz. Exercise a packaged save and a loose content pack from a different native working directory and a web subpath. Capture actual first-person frames, action logs, stable IDs, seed, tick and content revision on failure. Recheck original CC BY 4.0 attribution for newly added media, and MIT notices for code and scripts.

Use the existing entry points in a configured native build, a separate coverage build with `ORRA_COVERAGE=ON`, and an Emscripten build. Set `ORRA_CAPTURE_DIR` to an existing directory for release screenshots. When a deliberate rule change affects an existing replay, record the prior expectation and the gameplay reason for the new one. Compare the same revised rules across devices and render rates.

```sh
ctest --test-dir build -C Release --output-on-failure
python3 tools/coverage.py build-coverage
python3 tests/browser.py build-web/bin
openspec validate --all --strict
```

For performance, repeat the baseline 960 × 540 software-OpenGL probe and report p95 frame time against the 16.7 ms budget for 60 Hz. Publish phone and headset results with the real device and refresh rate. In XR also measure missed display frames during a busy yard, crossing and lift ride. Keep stereo viewpoints, pose, job progress, collision and readable work cues intact when reducing rendering detail. [Existing measurement and protocol](../../../docs/playtests/campaign/README.md)

For onboarding, begin with the 0/40 Gate Six newcomer result and the two viable expert openings. Record the first failed step, change the world cue or dispatch feedback responsible, and rerun both seed blocks. A first-time human should be able to explain a factory delay and direct the produced tank from aboard. A headset wearer should report scale, wrist-slate readability and comfort in their own words. Record unresolved findings and repeat the affected route after a fix. Check off first-playable items 4.5 and 4.6 only against those observations. [Original sign-offs](../define-first-command/tasks.md#4-exercise-every-target)
