## MODIFIED Requirements

### Requirement: Enter and leave WebXR in the same world

On a supported secure browser, a user gesture SHALL enter immersive VR with tracked stereoscopic first-person views and controller access to all mission actions. Ending the session SHALL return the player to flat play with their location and mission progress preserved. A browser without immersive support SHALL offer the flat game. Entering the pause desk or losing XR visibility SHALL suspend simulation and clear held controller input.

#### Scenario: Use a wrist slate from a tank

- **WHEN** the player enters XR aboard a moving Lintel, turns their head, points at the switch room marker and confirms an order to the Gate Section
- **THEN** each eye sees the compartment from its tracked pose, the infantry receive one order and Gate Six can be finished using XR controls.

#### Scenario: End the immersive session

- **WHEN** the player ends XR inside the Motor Hall after a unit rolls out
- **THEN** flat play resumes in the hall with the same commander, produced unit and crew jobs.

#### Scenario: Immersive VR is unavailable

- **WHEN** the browser lacks immersive VR support
- **THEN** the player can start and finish the flat-screen mission with the available pointer or touch controls.

#### Scenario: Interrupt a wrist order

- **WHEN** XR visibility is lost while the commander is choosing a crew assignment
- **THEN** the watch pauses with the draft intact, and simulation resumes only after an explicit selection.
