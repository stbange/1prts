# commander-feedback Specification

## Purpose

Help the Gate Six commander time production orders and see an approaching loss of control. A paired playtest measures how those decisions work in the shipped mission.

## Requirements

### Requirement: Dispatch orders through a crew change

The commander SHALL be able to issue one affordable follow-on production order within the last three 30 Hz ticks of a required crew transition. Dispatch SHALL recheck the bay, prerequisite and funds before charging the order once.

#### Scenario: Handover at the Plate Mill

- **WHEN** the commander orders a Watch House one to three ticks before the Plate Mill finishes and repeats that input
- **THEN** the crew starts one Watch House, once the Plate Mill exists, and charges its normal price once.

#### Scenario: A crew cannot hand over

- **WHEN** power loss stalls the required transition beyond the three-tick window
- **THEN** the held order expires and the commander receives a radio message.

### Requirement: Read a threat to the objective

The rival SHALL respond to a field section crossing toward the gate. The commander SHALL receive an audible muster signal and a persistent visual dispatch notice before the first rival section can occupy the switch room. An enemy hold SHALL display the seconds left to recover control.

#### Scenario: Race to the switch room

- **WHEN** a player sends infantry early while an opposing section can muster
- **THEN** the Compact announces its relief party, travels to the objective and can interrupt an unsupported claim.

### Requirement: Repeat the encounter audit

The game SHALL support matched seeded policy runs against its shipped mission rules and rendered checks using normal commander input. The evaluation SHALL distinguish observed game events, controlled timing probes and inapplicable personal-damage metrics.

#### Scenario: Replay both phases

- **WHEN** a tester selects the same two disjoint seed blocks on the instrumented baseline and the updated game
- **THEN** the harness emits per-episode outcomes, tick and input traces, measured frame timings and first-person captures suitable for comparison.
