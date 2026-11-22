## MODIFIED Requirements

### Requirement: Exercise the rendered entry points

The playable milestone SHALL include automated rendered checks on Windows, Linux, macOS, desktop web, a touch-only browser profile and an emulated WebXR device. Checks SHALL observe player-visible state and retain an action log and frame on failure. Release profiles SHALL declare device, viewport, refresh rate and memory limits, measure frame, input, startup and memory budgets and preserve phone and headset observations as independently identified evidence.

#### Scenario: A command breaks between input and simulation

- **WHEN** a platform accepts the physical confirm input but the destination order never appears on the slate or reaches the selected crew
- **THEN** that target's game test fails with the input history and rendered state even if the headless mission replay passes.

#### Scenario: Measure physical-device play

- **WHEN** the yard, ferry and lift are profiled on the declared device
- **THEN** the report identifies each route, its measured values, budget result and any outstanding human observation.
