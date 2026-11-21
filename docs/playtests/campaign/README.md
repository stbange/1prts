# Orra Basin campaign playtest

The campaign suite plays all seven operations through the graphical game's input handlers. Gate Six also runs to victory in Chromium using desktop controls, touch alone and emulated WebXR controllers. These checks use the actual Lua rules, OpenGL or WebGL renderer and first-person camera.

## Campaign decisions

A highlighted order now points to an available next job. As crews take assignments, the slate can suggest work that another crew can do concurrently. The ferry slate also posts the Compact's public quay reservation. We ran twelve repeatable seeds per policy in each of the six added operations, for 216 episodes per version. The newcomer and dispatcher policies use the mission's public advice when they notice it. The planner uses a predetermined schedule and retries rejected orders.

| Operation | Newcomer wins before → after | Dispatcher wins before → after | Planner wins before → after |
| --- | ---: | ---: | ---: |
| Tide Window | 12 → 12 | 11 → 12 | 12 → 12 |
| Gate Six / Compact | 12 → 12 | 12 → 12 | 12 → 12 |
| Westbound | 7 → 8 | 9 → 11 | 12 → 12 |
| White Mile | 10 → 9 | 12 → 12 | 12 → 12 |
| Night Mail | 11 → 10 | 12 → 12 | 12 → 12 |
| High Water | 5 → 12 | 12 → 12 | 12 → 12 |
| **All six** | **57 → 63 / 72** | **68 → 71 / 72** | **72 → 72 / 72** |

Orbital dispatch gained the most from showing a surface job while the tender was busy. White Mile and Night Mail each lost one newcomer win in this sample. Each CSV row records the seed, policy, elapsed simulation time, result, accepted and rejected orders, and completed work. [Before](sectors-before.csv) · [After](sectors-after.csv)

## First-person feedback and rendering

The Gate Six evaluation used two disjoint blocks of 20 seeds, five commander policies, six early-order probes and a real-time framebuffer probe. A paired local Release run used the same 960 × 540 viewport and software OpenGL renderer for both versions. The revised renderer tests actual mesh bounds and draws the sky after opaque geometry so occupied compartments skip hidden sky work.

| Measurement | Before render change | After render change |
| --- | ---: | ---: |
| Input to visible radio feedback, 95th percentile | 23.1 ms | 20.7 ms |
| Render duration, 95th percentile | 23.4 ms | 22.5 ms |
| Delivered frames per second | 46.9 | 50.7 |
| Feel / flow / choice | 1.00 / 0.75 / 1.00 | 1.00 / 0.75 / 1.00 |
| Pacing / fairness / reward loop | 0.89 / 1.00 / 1.00 | 0.89 / 1.00 / 1.00 |
| Rubric score | 91.0 / 100 | 91.0 / 100 |

The software renderer's measured 95th-percentile frame took longer than the 16.7 ms budget for 60 Hz. The Gate Six newcomer policy won none of its 40 episodes. The dispatcher and planner proxies each won all 40. The score measures the rubric's timing and decision proxies. Human enjoyment and headset comfort still need direct observation. [Before](gate-before.json) · [After](gate-after.json) · [Earlier Gate Six evaluation](../gate-six/README.md)

## Reproduce

```sh
lua tests/campaign_personas.lua build/bin 12 > sectors.csv
python3 tools/fun_audit.py build output --episodes 20
ctest --test-dir build -C Release --output-on-failure
python3 tests/browser.py build-web/bin
```

`tests/presentation.py` compares both Gate Six openings and the ferry crossing at 30 and 90 Hz. Checkpoints include both factions, resources, queues, world positions, parent frames, claim progress, mission state and result tick. For every scheduled presentation frame to reach the GPU, set `ORRA_RENDER_EVERY_FRAME=1`. Native tests compare framebuffer captures after enabling the pack. The browser suite changes the loose manifest for a second launch with the same Wasm module, then wins the extended hold.

## Two observations still needed

- **First-time player.** Start Gate Six without a walkthrough. Find the Plate Mill order, describe what tells you the crew accepted and finished it, board the produced Lintel, and issue an order while it moves. Record where you needed help and which cues you noticed.
- **Physical headset.** Enter from the yard, compare your standing eye height with a doorway and crew member, read and point at the wrist slate, board a moving craft, snap-turn and exit to flat play. Record the headset and browser, seated or standing use, readability, discomfort and whether your position stayed consistent. Stop the session if it becomes uncomfortable.
