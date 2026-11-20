# Platform play

## Purpose

Make the same first-person mission playable on desktop computers, touch-based browsers and immersive WebXR devices.

## ADDED Requirements

### Requirement: Start and command on each flat-screen target

The game SHALL run on Windows, Linux, macOS and browsers with WebGL 2 via Emscripten. A player on each target SHALL be able to walk, look, interact, board and issue, confirm or cancel orders through that target's controls.

#### Scenario: Board through the rendered game

- **WHEN** the player launches a distributed build on each native target or desktop web, walks up the ferry ramp and orders departure
- **THEN** the bridge crew acknowledges and the player sees the quay move from the deck.

### Requirement: Switch mobile web to touch controls automatically

On a touch-first mobile browser the game SHALL present touch controls on entry. Touch SHALL support simultaneous movement and looking, interaction, crew selection, job and target selection, confirmation and cancellation. Rotation or a switch of active input SHALL preserve the current order draft.

#### Scenario: Complete an operation using fingers

- **WHEN** a player opens the game on a touch-first phone and plays through Tide Window with touch alone
- **THEN** the movement, look, interaction and slate controls are available automatically and the relay can be brought online.

#### Scenario: Rotate during command entry

- **WHEN** the player selects a crew and rotates the phone before confirming its job
- **THEN** the controls reflow and the selected crew and draft job persist until confirmed or canceled.

### Requirement: Enter and leave WebXR in the same world

On a supported secure browser, a user gesture SHALL enter immersive VR with tracked stereoscopic first-person views and controller access to all mission actions. Ending the session SHALL return the player to flat play with their location and mission progress preserved. A browser without immersive support SHALL offer the flat game.

#### Scenario: Use a wrist slate on a moving deck

- **WHEN** the player enters XR aboard the ferry, turns their head, selects the slip with a controller and confirms
- **THEN** each eye sees the moving deck from its tracked pose, the pilot receives one order and the mission can be finished using XR controls.

#### Scenario: End the immersive session

- **WHEN** the player ends XR while on the island following a delivery
- **THEN** flat play resumes at the island with the same commander, delivered cargo and crew jobs.

#### Scenario: Immersive VR is unavailable

- **WHEN** the browser lacks immersive VR support
- **THEN** the player can start and finish the flat-screen mission with the available pointer or touch controls.
