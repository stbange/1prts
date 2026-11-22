## MODIFIED Requirements

### Requirement: Verify the rendered game

The build SHALL support input replays with actual framebuffer screenshots. The browser build SHALL use WebGL 2, switch touch controls on for mobile input and preserve the operation across entry into and exit from an available immersive VR session. The public release and desk SHALL identify the playable build and its representative captures.

#### Scenario: Inspect command input on mobile

- **WHEN** a touch-first player taps the orders control and commissions the mill
- **THEN** the ledger drops to four alloy and the crew acknowledges through the on-screen radio.

#### Scenario: Inspect screenshots

- **WHEN** the native acceptance routes issue a capture during field play
- **THEN** the saved image shows the current first-person world and its rendered interface.

## ADDED Requirements

### Requirement: Inspect local failures

A detectable abnormal termination SHALL leave a local diagnostic, a recovery path and player-controlled sharing.

#### Scenario: Identify builds and local failures

- **WHEN** the player restarts after a detectable renderer failure
- **THEN** they can read its build, mission and category, recover a committed save, and copy the report.
