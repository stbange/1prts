# Gate Six game fun audit

The automated score rose from **61.4 to 91.0 out of 100**. The changes with the clearest consequences were accepting a command at a crew handover and giving the Compact time to challenge a lone section. A commander who fields both arms can still win from the seat of a moving Lintel.

## Before and after

| Pillar | Before | After | Observed in play |
| --- | ---: | ---: | --- |
| Kinesthetics and feel | 0.48 | 1.00 | The 95th-percentile time from an order to a changed radio frame was 18.1 → 16.4 ms. All six early-order probes originally dropped. All six now execute. Feedback averaged 2.25 → 2.75 channels across four events. |
| Flow and skill gap | 0.75 | 0.75 | Mean objective completion for novice, human-proxy and oracle policies went from 11.2%, 100%, 100% to 9.2%, 100%, 100%. The skill gap widened from 0.89 to 0.91. Human-proxy variation stayed at zero. |
| Meaningful choices | 1.00 | 1.00 | Normalized order entropy stayed at 0.90. The largest order share was 12.5%, and repeatedly pressing Advance earned zero objective progress. A separate infantry-only opening exposed a weakness the entropy score missed. |
| Pacing and tension | 0.65 | 0.89 | The mean coefficient of variation of tension fell from 2.89 to 1.09. The target band is 0.25–0.85. Neither phase had a sustained overload. |
| Fairness and clarity | 0.46 | 1.00 | On an unopposed approach, the dedicated warning increased from zero to 17.97 s before occupation. Median enemy-to-background contrast at the sampled sightline rose from 3.20 to 4.89. The share of sampled pixels meeting 3.5 to 1 rose from 38% to 63%. |
| Loop friction | 1.00 | 1.00 | First interactive frame took 0.35 → 0.30 s. Restart input to a fresh rendered mission took 16 → 17 ms. Median small reward interval was 1 s, with a 60 s median between operation milestones. |

### What changed for the commander

- Dispatch now holds one affordable follow-on order during the final three simulation ticks of a crew transition. The crew checks the bay, price and prerequisites again before charging. A duplicate press cannot buy a second job.
- The Compact calls its relief party when it spots infantry crossing. The field notice counts down to departure and then, if the rival occupies the switch room, to loss of control. The objective tells the commander how to bring a Lintel to the yard. The notice and radio cue also reach a commander aboard a vehicle.
- Walking accelerates and settles over a few ticks. The first movement still happens on the first tick. A crew finishing a job and the operation ending now have audible cues. A pale helmet band and darker coat separate Compact infantry from machinery at the tested sightline.

### What the bots missed

The infantry-only policy won **40 of 40** baseline runs in 83 seconds, before the original fixed rival departure at 100 seconds. It won **0 of 40** after the change and averaged 21.6% of the required hold. The human-proxy and greedy policies won **40 of 40 in both phases**. A player following a known build sequence still faces little uncertainty. The proxy knows the orders and prerequisites, so these wins cannot establish that a new player will discover the response.

The novice won neither phase. **31 → 29 of its 40 runs** were still active at the four-minute cutoff. The rest ended in defeat. The score also leaves room for a stronger end-of-mission signal. The loss produced two feedback channels, below the skill's high-impact target of 3.5. The next human playtest should check whether a first-time commander notices the relief call, understands which crew to send and chooses to retry.

## Evidence

These 960 × 540 frames come from the native game. The peak capture is the last tick before an unopposed Compact victory. Both phases use the same first-person observation point.

| Moment | Before | After |
| --- | --- | --- |
| Entering the yard | [Opening frame](before-boot.png) | [Opening frame](after-boot.png) |
| One tick before losing the gate | [Yard and HUD](before-peak.png) | [Yard and countdown](after-peak.png) |
| Debrief and retry | [Control lost](before-game-over.png) | [Control lost](after-game-over.png) |

The evidence includes 400 individual outcomes, both score inputs and raw command and rendering logs. Paired images in the archive show the same enemy approach with and without the rival sprite for an exact background comparison. [Episodes](episodes.csv) · [Measurements](measurements.json) · [Telemetry and contrast frames](telemetry.zip)

## Method and limits

Each phase ran 20 paired seeds from 42–61 and 20 holdout seeds from 1042–1061 for each of five policies. The novice pressed a random order roughly 2–4 times a second. The human proxy checked the slate every 2–7 seconds and reacted after 200 ± 35 ms, quantized to 30 Hz. It alternated between infantry-first and armor-first plans across seeds. The greedy oracle could act every tick. The controls repeatedly pressed Advance or followed a competent infantry-only plan. Seeds vary the controllers. The encounter runs the shipped deterministic Lua rules. Objective completion is the best uninterrupted friendly hold divided by 60 seconds.

The timing fixtures exercised two different transitions at one, two and three ticks early. The ordinary proxy issues orders when its local rules judge them available. The six fixtures measure forgiveness in that specific window. The native instrument sampled actual HUD pixels, positions, queued audio and world-model changes. Its 150-frame real-time pass targeted 60 Hz on Linux with software OpenGL. Average delivery was 58.4 → 58.7 FPS. The 95th-percentile render cost was 18.5 → 17.7 ms, above the 16.7 ms frame budget. These single-machine timings include GPU completion. Physical display and speaker latency were outside this test.

For the tension index, rival infantry and armor approach contribute 55%, order rate contributes 30%, and the fraction of the enemy's hold contributes 15%. Gate Six has no commander health, damaging projectiles or pickup hitboxes. Personal near misses and hurtbox ratios are inapplicable. The fairness score averages the median foreground contrast and dedicated warning lead at the sampled encounter. Baseline enemy movement was already visible to a commander looking toward the road.

The scoring code converts each applicable observation to a target-relative value capped at one and averages within each pillar. It then uses the skill's weighted composite. Human-proxy completion and its variation are separate flow components. A strong result on the aggregate can coexist with perfect proxy wins, below-target pixels or long novice stalemates. [Scoring implementation](../../../tools/fun_audit.py)

## Repeat the audit

From a Linux checkout with SDL3, CMake, a C compiler, Lua 5.4, Python, NumPy, Pillow and an offscreen OpenGL driver, run the following commands. The harness writes full-resolution frames and CSV and JSON data to the output directory.

```sh
cmake -S . -B build-audit -DCMAKE_BUILD_TYPE=Release
cmake --build build-audit --target fun_probe --parallel
python3 tools/fun_audit.py build-audit /tmp/gate-six-audit --seed 42 --holdout 1042 --episodes 20
```

For the original rules and rendering, run the same commands in a checkout of the instrumented baseline with a separate output directory. [Baseline revision](https://github.com/stbange/1prts/commit/e0f1397)
