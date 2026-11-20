# Platform play

## Purpose

Make the same first-person mission playable on desktop computers, touch-based browsers and immersive WebXR devices.

## ADDED Requirements

### Requirement: Start and command on each flat-screen target

The game SHALL run on Windows, Linux, macOS and browsers with WebGL 2 via Emscripten. A player on each target SHALL be able to walk, look, interact, board and issue, confirm or cancel orders through that target's controls.

#### Scenario: Board through the rendered game

- **WHEN** the player launches a distributed build on each native target or desktop web, boards a crewed tank at the factory and orders a move to the gate
- **THEN** the driver acknowledges and the player sees the yard move past the hatch.

### Requirement: Switch mobile web to touch controls automatically

On a touch-first mobile browser the game SHALL present touch controls on entry. Touch SHALL support simultaneous movement and looking, interaction, crew selection, job and target selection, confirmation and cancellation. Rotation or a switch of active input SHALL preserve the current order draft.

#### Scenario: Complete an operation using fingers

- **WHEN** a player opens the game on a touch-first phone and plays through Gate Six with touch alone
- **THEN** the touch layout appears automatically and the player can mark a foundation, queue a unit and order the occupation from first person.

#### Scenario: Rotate during command entry

- **WHEN** the player selects a crew and rotates the phone before confirming its job
- **THEN** the controls reflow and the selected crew and draft job persist until confirmed or canceled.

### Requirement: Enter and leave WebXR in the same world

On a supported secure browser, a user gesture SHALL enter immersive VR with tracked stereoscopic first-person views and controller access to all mission actions. Ending the session SHALL return the player to flat play with their location and mission progress preserved. A browser without immersive support SHALL offer the flat game.

#### Scenario: Use a wrist slate from a tank

- **WHEN** the player enters XR aboard a moving Lintel, turns their head, points at the switch room marker and confirms an order to the Gate Section
- **THEN** each eye sees the compartment from its tracked pose, the infantry receive one order and Gate Six can be finished using XR controls.

#### Scenario: End the immersive session

- **WHEN** the player ends XR inside the Motor Hall after a unit rolls out
- **THEN** flat play resumes in the hall with the same commander, produced unit and crew jobs.

#### Scenario: Immersive VR is unavailable

- **WHEN** the browser lacks immersive VR support
- **THEN** the player can start and finish the flat-screen mission with the available pointer or touch controls.
