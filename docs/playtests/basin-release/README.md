# Basin campaign verification

At the start of this change, all 17 tests at `daea77b` passed. That run included seven operations, both Gate Six opening builds, the idle commander's defeat, the standard and upgraded ferry landings, the Long Watch art and rule overrides, and the 30 and 90 Hz checkpoints. Independent unit coverage measured 1,942 of 2,019 native C lines, 1,596 of 1,622 shipped Lua lines and 100% of the handwritten browser JavaScript lines.

The second-tank replay walks from the initial position to the factory's second Lintel and boards by the rear hatch. The yard tests reserve four alloy out of six, hold a third rollout until the exit clears, and put two haulers on the same last load. The presentation test compares the complete Lua state reached through the graphical controls at 30 and 90 Hz with the headless simulator. [Rendered input](../../../tests/second-tank.route) · [Yard assertions](../../../tests/yard_rules.lua) · [Host comparison](../../../tests/presentation.py)

A roster walk starts at the entrance of each craft or facility, traverses its aisle to the fitted work area and returns outside. The Crown walk starts on the docked tender and returns to it. The campaign state tests dispatch the advertised field shift for every catalog ID and check the receiving workplace's receipt. The tests also restore the commander on a moving crawler and recover the previous save after an interrupted write. [Interior walks](../../../tests/rooms.c) · [Field shifts and transfers](../../../tests/campaign_state.lua) · [Save and lift tests](../../../tests/dispatch_state.c)

## Gate Six onboarding

A simulated first-time reader glances at the command slate every 1.5 to 6.2 seconds and sometimes follows an order at random. With the same policy and the same 40 seeds, highlighting an affordable next order and showing the bay's time to handover raised wins from 0 to 27. Thirteen runs still had an active mission at the four-minute observation limit. Median first mill acceptance fell from 313 to 77 simulation ticks among runs that ordered a mill. The CSVs give individual results and the first rejection reason. The original noise, timed dispatcher and planner policies are measured separately. [Reader policy](../../../tests/read_slate.lua) · [Before](reader-before.csv) · [After](reader-after.csv)

First-time human observations and physical-headset observations remain open. The device test covers perceived scale, motion during a crossing and lift ride, wrist readability and return to flat play. Human factory feedback includes what the player understood about acceptance, delay and rollout, and whether they could issue an order aboard their tank. [Observation protocol](../campaign/README.md#two-observations-still-needed)

## Release candidate

The release candidate completed 21 native CTest entries. The identical schedules at 30 and 90 Hz produce matching simulation checkpoints. Two additional comparisons exit the process during an active Gate Six claim and from inside the Mole aboard the moving Kestrel with the Shore Depot on shift. Resuming and advancing 183 ticks reproduces the simulation digest, world projection and commander pose in both cases. Chromium exercised desktop input, automatic touch controls, two-eye XR rendering, all seven sector targets, page reload, and boarding the Wire Courier with both packs enabled. Strict OpenSpec validated all five items. [Tested source](https://github.com/stbange/1prts/tree/984d4ec83aafbfa8bd50b8b5a26e25108b085705) · [Native test results](native-tests.txt) · [Browser results](chromium.txt) · [OpenSpec results](openspec.txt)

The field exercise records the crew, operation, job and resulting acknowledgment for all 69 field-guide types. Each of the 61 craft and structures also has an entrance-to-workplace-to-exit exercise. The silhouette test renders the Lintel and Wake as two solid masks from the same raised view. Their shapes differ on 62.5% of their combined silhouette pixels. [Roster evidence](roster.tsv) · [Hull and audio checks](../../../tests/asset_readability.py) · [Screenshots](../../screenshots/README.md)

| Independent unit inventory | Covered lines | Coverage |
| --- | ---: | ---: |
| C including otherwise unmeasured platform adapter lines | 2,666 / 2,788 | 95.62% |
| Compiled native C configuration | 2,666 / 2,749 | 96.98% |
| Shipped Lua, including loose packs | 3,391 / 3,526 | 96.17% |
| Handwritten browser JavaScript | All lines | 100% |

The C lower bound counts the other platforms' adapter lines as uncovered. The unit runs use fresh coverage files. The gameplay replays, renderer benchmarks and browser victories run separately. [Per-source counts and missed lines](unit-coverage.json) · [JavaScript unit report](web-unit-tests.txt)

## Release playtest

On the release candidate, the slate-reader proxy won 15 of the first 20 seeds and 17 of the 20 held-out seeds. Eight sessions remained active at the four-minute limit. Median time to the first accepted Plate Mill order was 70 ticks, or 2.33 seconds, among sessions that placed one. Rejected orders across the 40 sessions fell from 1,170 on the original run to 744 immediately after the cue change and 728 on the release candidate. The second-tank replay boards the new Lintel, dispatches the first tank to guard and its own driver to move to Gate Six, then verifies both live orders. The commander's local deck coordinates stay fixed as the tank moves over 22 metres. [Individual release sessions](reader-release.csv) · [Boarding inputs](../../../tests/second-tank.route) · [Two-driver checkpoints](two-drivers.txt)

The Game Fun Evaluation also ran five independent policies over both 20-seed blocks. Its composite score moved from 91.0 to 89.8 out of 100. Software-rendered p95 draw time improved from 30.59 to 26.20 ms. Input-to-visible-feedback p95 rose from 29.36 to 40.55 ms, and defeat-to-retry measured 25.8 ms. The reports include the policy definitions, raw timings and results for each scored dimension. [Before](yard-before.json) · [After](yard-after.json) · [Before trials](yard-before-trials.csv.gz) · [After trials](yard-after-trials.csv.gz) · [Before frame samples](yard-before-frames.csv) · [After frame samples](yard-after-frames.csv)

The four profiles below used the uninstrumented Linux release, a 960 × 540 framebuffer, software OpenGL on an AMD EPYC 9V74 CPU and 100 measured frames after five warmups. Each frame advances the simulation, draws, and waits for the GPU. The stereo run draws two 480 × 540 eyes. The process peaked at 126.7 MiB of resident memory. The busy yard and stereo lift exceeded the 16.67 ms frame budget at the 95th percentile.

| Scene | Median frame | p95 frame | Frames over 16.67 ms |
| --- | ---: | ---: | ---: |
| Yard with the second tank | 22.79 ms | 24.37 ms | 100 / 100 |
| Moving ferry | 11.60 ms | 13.44 ms | 3 / 100 |
| Crown traffic lift | 9.59 ms | 11.43 ms | 0 / 100 |
| Two-eye Crown lift | 15.64 ms | 23.47 ms | 25 / 100 |

The browser now fetches models when an operation needs them. The renderer prepares nearby compartment fittings as the commander approaches. The four Chromium starts observed between 595 and 1,235 ms to the first readiness check and a WebAssembly heap of 17,104,896 bytes. The software profiles measure frame-budget overruns. Physical display drops, motion comfort and phone input latency still need a device session. [Sector frame samples](sector-frames.csv) · [Chromium startup measurements](chromium.txt) · [Game Fun Evaluation probe](../../../tests/fun_probe.c)

Across 216 additional campaign trials, the planner won all 72 sessions. The newcomer won 45 of 72 and the timed dispatcher won 51 of 72. Those losses include missed deadlines and occupied work slots. [Campaign trials](campaign-trials.csv) · [Scenario policies](../../../tests/campaign_personas.lua)

To repeat the sector profiles, run the release build's `fun_probe build/bin output sectors` with a 960 × 540 software OpenGL surface. The last column reports milliseconds per simulation step and completed draw. Generate the unit inventory with `python3 tools/coverage.py build-coverage --report coverage.json`. [Coverage tool](../../../tools/coverage.py)

## Verified packages

Linux x86-64, Windows x86-64, macOS arm64 and the Emscripten build all passed CI on the tested revision. Each downloadable distribution includes editable models, sounds, shaders, Lua rules, the two example packs, attribution and both licenses. The local distribution archives also include controls, save instructions and the modding guide. [Build and test jobs](https://github.com/stbange/1prts/actions/runs/44960266380) · [Distribution checksums](SHA256SUMS)

- [Linux build](https://github.com/stbange/1prts/actions/runs/44960266380/artifacts/14728893977)
- [Web build](https://github.com/stbange/1prts/actions/runs/44960266380/artifacts/14729008164)
- [Windows build](https://github.com/stbange/1prts/actions/runs/44960266380/artifacts/14729292398)
- [macOS build](https://github.com/stbange/1prts/actions/runs/44960266380/artifacts/14729406309)

A fresh extraction of the Linux distribution replayed all seven operations, the independent tank orders, process resume, and the loose-content pack. A fresh extraction of the published web build passed the same desktop, touch and stereo XR suite under a subpath. [Linux archive replay](packaged-linux.txt) · [Web archive replay](packaged-web.txt)
