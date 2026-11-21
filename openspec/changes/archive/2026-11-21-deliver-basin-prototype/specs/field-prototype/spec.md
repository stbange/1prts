# Field prototype

## Purpose

Deliver a playable first-person ground operation with repeatable outcomes and captures on the implemented platforms.

## ADDED Requirements

### Requirement: Play the opening battle

The commander SHALL be able to commission a mill, dispatch a hauler, build a mustering hall and factory, field infantry and armor and hold the gate for 60 seconds. The rival SHALL pay for its units and be able to win when unopposed.

#### Scenario: Win aboard a tank

- **WHEN** the commander builds a Lintel, boards at its rear hatch and sends armor and infantry to the gate
- **THEN** the commander rides the moving hull and the friendly section can win the operation.

#### Scenario: Leave the rival alone

- **WHEN** the commander gives no orders
- **THEN** the Compact can occupy the room, win and trigger a first-person debrief.

### Requirement: Verify the rendered game

The build SHALL support input replays with actual framebuffer screenshots. The browser build SHALL use WebGL 2, switch touch controls on for mobile input and preserve the operation across entry into and exit from an available immersive VR session.

#### Scenario: Inspect command input on mobile

- **WHEN** a touch-first player taps the orders control and commissions the mill
- **THEN** the ledger drops to four alloy and the crew acknowledges through the on-screen radio.

#### Scenario: Inspect screenshots

- **WHEN** the native acceptance routes issue a capture during field play
- **THEN** the saved image shows the current first-person world and its rendered interface.
