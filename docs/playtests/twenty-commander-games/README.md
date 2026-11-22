# Twenty commander games

On 22 November 2026, 20 fresh games on Linux build `0.2-4037ef2986ba` ended in **17 victories and 3 defeats** across all seven operations. All seven reference routes won. The 20 games covered **1 hour, 9 minutes and 36.77 seconds** of simulation.

The commanders issued **195 inputs**. Of those, 150 submitted orders to the simulation, which accepted 141 and refused 9. The other 45 inputs moved the commander, changed the view, boarded craft or operated panels. [Commander action log](commander-actions.csv) · [Game results](games.csv) · [Replay routes, raw events and captured frames](replay-evidence.zip)

## What decided the games

- **Keeping the Lintel in the yard cost Gate Six.** Game 04 built armor and tried to advance without a line section. The order was refused. The Compact took the gate after holding it for a minute, with 26 alloy still in the commander's stores.
- **Dispatch timing mattered even when every order was valid.** In Westbound game 12, the exchange points were ready at 0:58. The commander sent the freight at 3:49. The 4:25 shift deadline left it about four seconds short.
- **The White Mile commander missed the chance to retry.** In game 15 the commander reopened the pump passage at 0:53.33 and asked for analysis at 1:12.00. Barrow reported the sample at 1:12.33. The next order requested its return, which still needed analysis. Barrow still held the sample when the shift closed at 4:15.
- **The ferry timetable rewarded spending six alloy early.** In Tide Window, the hoist and public quay finished in 7:15, the extended ramp and gravel slip in 9:35, and the crawler and public quay in 11:00. The crawler reached the occupied quay at 5:20 and waited until 8:00 to unload. All three routes installed both cores.
- **A second docking collar changed when crews could accept orders.** In High Water games 19 and 20 the commander issued the docking order at 1:18.67 in both runs. The four-alloy collar let docking finish at 1:40.67 during servicing. On one collar, docking finished at 2:31.33. The commander then successfully retried both handoffs.

## Every game

Times are elapsed simulation minutes and seconds, rounded to the nearest hundredth. The order columns count submissions that reached the game rules.

| Game | Operation | Approach | Result | Time | Accepted | Refused | Final alloy | What happened |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| 01 | Gate Six | Infantry infrastructure first | Victory | 1:53.13 | 8 | 0 | 21 | Boarded the Lintel, then ordered it to guard and sent the line section to the gate. |
| 02 | Gate Six | Armor infrastructure first | Victory | 1:46.87 | 8 | 0 | 21 | Ordered armor to guard and infantry to advance at 0:36.90. Secured the gate 6.27 seconds before game 01. |
| 03 | Gate Six | Two Lintels and a crew order | Victory | 2:02.50 | 10 | 0 | 17 | Boarded the second Lintel and used the crew register to send its driver to the gate. The driver reported on station. |
| 04 | Gate Six | Keep the armor in the yard | Defeat | 2:49.93 | 4 | 1 | 26 | Produced a Lintel and left it in the yard. The advance order needed a line section. The Compact held the gate for 60 seconds. |
| 05 | Tide Window | Crawler to the public quay | Victory | 11:00.00 | 4 | 0 | 8 | The crawler brought both cores, then the ferry waited for the quay to clear at 8:00. Both cores were installed. |
| 06 | Tide Window | Hoist to the public quay | Victory | 7:15.00 | 4 | 0 | 2 | Spent six alloy on the hoist. The ferry unloaded before the quay reservation and the relay came online. |
| 07 | Tide Window | Extended ramp to the gravel slip | Victory | 9:35.00 | 3 | 0 | 2 | Took the gravel slip and transferred from Kestrel to Mole during the crossing. Both cores reached the relay. |
| 08 | Gate Six / Compact | Sound and repair before towing | Victory | 1:33.83 | 7 | 0 | 8 | Started repairs before the tow. The fitters restored the watch after repairs finished and the section landed. |
| 09 | Gate Six / Compact | Early tow and patrol sortie | Victory | 1:34.17 | 8 | 2 | 8 | An early tow and an early restoration were refused. Retried both after their prerequisites, and the patrol also finished. |
| 10 | Westbound | Heater and gallery reserve | Victory | 1:43.57 | 9 | 0 | 4 | Boarded the shunter, fitted a heater and switched workshop power twice. The winter freight arrived. |
| 11 | Westbound | Gallery reserve without a heater | Victory | 1:47.33 | 6 | 0 | 8 | The charged reserve kept track work moving through the feeder outage. Freight arrived with all eight alloy. |
| 12 | Westbound | Dispatch freight at 3:49 | Defeat | 4:25.00 | 6 | 0 | 4 | All six orders were accepted. Freight had completed 1,079 of 1,200 work ticks when the shift closed. |
| 13 | White Mile | Wide Shoes from aboard Barrow | Victory | 1:38.10 | 7 | 0 | 4 | Walked aboard Barrow and fitted Wide Shoes. The load reached the assay office. |
| 14 | White Mile | Standard kit on the field route | Victory | 2:03.83 | 6 | 0 | 8 | Marked, drained and surveyed the camp without purchasing the upgrade. Barrow returned the sample. |
| 15 | White Mile | Close and reopen the sump passage | Defeat | 4:15.00 | 8 | 3 | 4 | Reopened the passage. The analysis request came ten ticks before sample collection finished. It was never retried. |
| 16 | Night Mail | Install the Night Board mid-flight | Victory | 2:22.03 | 8 | 0 | 4 | Rode Kite and installed the Night Board during the flight. All three mailbags arrived. |
| 17 | Night Mail | Share a single apron | Victory | 3:08.00 | 7 | 1 | 8 | The early escort request was refused. Retried after the patrol, and the crews delivered all three bags on one apron. |
| 18 | High Water | Service early, ride to the station | Victory | 2:42.47 | 9 | 0 | 8 | Rode Lark to Crown Station, transferred aboard and reopened station traffic with the original collar. |
| 19 | High Water | Overlap docking with a second collar | Victory | 2:34.67 | 10 | 0 | 4 | Commissioned a second collar. Lark finished docking while Cleat was still servicing the first collar. |
| 20 | High Water | Overlap docking with a single collar | Victory | 3:26.33 | 9 | 2 | 8 | Both handoff orders were refused while Lark waited for the occupied collar. Retried after docking and reopened traffic. |

## How to read the evidence

Each game used a fresh process, the stock mission, eight starting alloy and its own save directory. Scripted keyboard and mouse inputs drove the native first-person commander through the game's replay entry point. The simulation ran at 30 ticks per second. The start and outcome frames came from SDL3 and software OpenGL. Each final checkpoint agreed with the recorded victory or defeat and its exact tick. All 29 assertions embedded in the routes passed.

The test set includes each operation's reference route, two alternate Gate Six fixtures and eleven additional schedules chosen to exercise timing, upgrades and prerequisites. Game 03 continues the two-driver fixture until the gate is resolved. These deliberately selected runs measure the recorded routes. Estimating a human win rate would require a different sample. [Tested source revision](https://github.com/stbange/1prts/tree/4037ef2986ba5069c9d313b8efdea059bba8b966)

- The **action log** has one row for each key press, movement hold or mouse look. It includes the order, the simulation's decision, the radio text, elapsed tick, alloy, panel and vessel. A blank order result denotes a movement or interface action. An unchanged radio field means the text and timestamp matched the previous values. [Commander actions CSV](commander-actions.csv)
- The **results file** records the first terminal tick, the final report, both starting and ending alloy, state and world checksums, and frame hashes. [Game results CSV](games.csv)
- The **evidence archive** includes the 20 exact routes, one event timeline per game, checkpoint output, 40 framebuffer captures and the recorder source. Waits and test assertions appear in those timelines. Its README includes a replay command. [Replay evidence ZIP](replay-evidence.zip)
