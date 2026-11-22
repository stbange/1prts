# Commander session release checks

The mobile browser test commissions a Plate Mill for four alloy, cancels a mission change and waits while the watch is paused. It saves to Watch B, enters rehearsal, advances practice by 90 ticks and returns to the previous mission tick, camera and budget. It then reloads the site and continues from persistent storage. An injected HTTP 503 for the Crown Station model exercises recovery without losing Gate Six. The same test changes a keyboard binding, switches to Cyrillic controls, checks English fallback, and reads a bundled license from the desk. [Browser acceptance routes](../../../tests/browser.py)

The C unit suite covers a manual save arriving during a checkpoint, independent slot recovery, damaged saves, cancellation, held input on focus loss, HUD and wrist actions, persisted preferences, text substitutions, missing glyphs, call history and radio attenuation. The rendered routes exercise all seven operations, and the browser suite takes Gate Six to victory on desktop, touch and emulated XR. An Emscripten test clock drives the simulation through normal inputs. Emulation exercises controller events and stereo drawing. [C session tests](../../../tests/session.c) · [Browser adapter tests](../../../tests/web-unit.cjs) · [Desk and profiler tests](../../../tests/desk-unit.cjs)

The local Linux build passed all 21 CTest entries. Unit-only line coverage was 95.30% for first-party C including unmeasured target adapters, 95.97% for shipped Lua and 97.22% for handwritten browser JavaScript. The 18 OpenSpec items passed strict validation. [Coverage command](../../../tools/coverage.py) · [Builds and automated checks](https://github.com/stbange/1prts/actions/workflows/game.yml)

## Reference routes

These Linux release runs used Mesa software OpenGL on an AMD EPYC 9V74 at 960 × 540. Each route has five warmup frames, 100 simulation-and-draw samples and ten timed slate inputs. Stereo uses two 480 × 540 views. A GPU fence completes each draw before timing stops. Startup to the first completed game frame took 331 ms with full shadows and 106 ms on the second process launch. Peak process RSS across the two runs was 127.0 MiB.

| Renderer setting and route | Frame p95 | Frames over 16.67 ms | Input to completed draw p95 |
| --- | ---: | ---: | ---: |
| Full, second-tank yard | 30.39 ms | 100 / 100 | 30.93 ms |
| Full, moving ferry | 13.76 ms | 3 / 100 | 16.36 ms |
| Full, traffic lift | 10.64 ms | 2 / 100 | 11.57 ms |
| Full, tracked stereo lift | 17.74 ms | 11 / 100 | 24.27 ms |
| Low shadows, second-tank yard | 17.17 ms | 13 / 100 | 16.88 ms |
| Low shadows, moving ferry | 9.80 ms | 1 / 100 | 9.90 ms |
| Low shadows, traffic lift | 8.37 ms | 0 / 100 | 8.37 ms |
| Low shadows, tracked stereo lift | 10.36 ms | 1 / 100 | 13.44 ms |

Run the release build with `fun_probe build/bin output sectors`. Use `sectors-low` for the lower-cost graphics preset. [Measured frames, inputs and startup](sector-timings.csv) · [Renderer probe](../../../tests/fun_probe.c)

## Physical-device and first-player observations

The physical phone, headset and first-time player sessions are open. The recorder starts after the game loads. Select the device class, enter the model, browser, refresh rate and route, then choose **Measure route**. Use the normal controls and finish the measurement to see the local JSON result. The recorder measures the interval to the next presented game frame after input. It tracks overruns and estimates missed intervals from frame timing. Heap measurements cover WebAssembly memory. [Open the local device recorder](https://stbange.github.io/1prts/?profile=1)

| Session | Evidence to record |
| --- | --- |
| Phone | Run the busy yard, moving ferry and Crown lift in portrait and landscape. Record the reference device, startup, peak heap, frame p95, input p95 and any unreachable control. |
| Headset | Repeat the three routes at the display's configured refresh rate. Record posture, perceived scale, wrist text legibility, controller targeting, discomfort, interruption and return to flat play. |
| First watch | Let a new player find and commission the mill, identify acceptance and completion, board their produced tank and issue an order while it moves. Record elapsed time, help requested and the player's own account. |

The flat desktop reference uses a 16.67 ms frame budget, the phone uses 33.33 ms and the headset uses one display refresh interval. Browser startup has a five-second budget and the heap budget is 256 MiB. Input-to-next-frame budgets are 50 ms on desktop, 100 ms on phone and two display intervals in XR. The profiler leaves an unsampled input measurement empty. [Qualification change](../../../openspec/changes/qualify-real-devices/tasks.md) · [First-player observation](../../../openspec/changes/teach-gate-six/tasks.md)
