# Game verification

## Purpose

Give contributors reproducible evidence that each runtime layer is exercised and that the game is playable through its public controls.

## ADDED Requirements

### Requirement: Enforce unit line coverage

The build verification SHALL require at least 95% executable-line coverage from unit tests independently for first-party runtime C, shipped Lua and handwritten browser glue. The denominator SHALL include untouched production files and platform adapters. Only generated code, third-party code, tests and fixtures SHALL be excluded.

#### Scenario: One language falls below the gate

- **WHEN** Lua unit coverage is 94.9% and C and browser unit coverage are both 100%
- **THEN** the gate fails and reports the measured line counts per language, even if game tests exercise the remaining lines.

#### Scenario: A module has no tests

- **WHEN** a first-party production module is shipped but never loaded by the unit suite
- **THEN** its executable lines contribute zero covered lines to the appropriate language total.

### Requirement: Replay gameplay through ordinary actions

Automated game tests SHALL drive ordinary player actions at recorded simulation ticks. Gate Six replays SHALL exercise both build openings, power recovery, an interrupted claim and a rival victory. Tide Window replays SHALL exercise both winning routes, a berth hold and a deadline loss. Each mission SHALL reproduce its outcome from the same seed and actions independently of display cadence.

#### Scenario: Verify the opening land battle

- **WHEN** a recorded Gate Six operation builds a mill, produces forces and captures the gate at two presentation rates
- **THEN** both runs agree on deposits, facility queues, spawned crews, claim resets, faction outcome and result tick.

#### Scenario: Repeat a delivery at two render rates

- **WHEN** the same mission and actions run with 30 Hz and 90 Hz presentation
- **THEN** the crews, orders, funds, cargo locations, relay state, outcome and result tick agree across both runs.

### Requirement: Exercise the rendered entry points

The playable milestone SHALL include automated rendered checks on Windows, Linux, macOS, desktop web, a touch-only browser profile and an emulated WebXR device. Checks SHALL observe player-visible state and retain an action log and frame on failure.

#### Scenario: A command breaks between input and simulation

- **WHEN** a platform accepts the physical confirm input but the destination order never appears on the slate or reaches the selected crew
- **THEN** that target's game test fails with the input history and rendered state even if the headless mission replay passes.
