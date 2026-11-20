# Gate Six

## Purpose

Open the basin campaign with two working bases competing to occupy a single floodgate through first-person command.

## ADDED Requirements

### Requirement: Fund the opening from a field base

Each faction SHALL start with eight alloy, a staffed Survey Hall and Dynamo House, a crewed Mole, fitters and operating personnel. The mill, Watch House and Motor Hall SHALL cost four, three and six alloy and use two, one and three grid capacity respectively. The starting Dynamo House SHALL supply six. A line section SHALL cost two alloy and a faction vehicle four.

#### Scenario: Muster infantry first

- **WHEN** the player commissions the mill and Watch House, processes one full load and musters a Gate Section
- **THEN** the section assembles at the door, three alloy remain available and the commander can order it to the switch room.

#### Scenario: Produce armor first

- **WHEN** the player commissions the mill, processes a load, builds the Motor Hall, processes a second load and finishes a Lintel
- **THEN** a staffed tank rolls out with two alloy remaining. A third load pays for the Watch House and a section with one alloy remaining.

### Requirement: Compete against a working rival

The Compact commander SHALL use its own observed contacts, finite plate, alloy, power, crew and production queues. The opponent SHALL be able to win by the same occupation rule as the player.

#### Scenario: Leave the rival unopposed

- **WHEN** the player stays in the Survey Hall and gives no orders
- **THEN** the Compact processes plate, pays for its fielded forces, sends a section into the station and can win while the player's view stays inside the hall.

### Requirement: Decide control inside the station

A faction SHALL win after its line section holds the switch room for 60 consecutive seconds with no opposing line section in the room and no operational enemy armor in the painted yard. Losing eligibility SHALL reset the count. The commander and neutral gatekeeper SHALL contribute no occupation time themselves.

#### Scenario: Interrupt just before the handover

- **GIVEN** an Assembly section has held the room for 1,799 of the required 1,800 simulation ticks
- **WHEN** an operational Wake enters the yard on the next tick
- **THEN** the claim resets, the field report identifies the yard contest and no winner is declared.

#### Scenario: Clear the yard and hold again

- **WHEN** the yard clears and the same section holds the room unopposed for 1,800 new ticks
- **THEN** the Assembly wins once, including when the commander is aboard a vehicle elsewhere.

#### Scenario: Arrive with armor alone

- **WHEN** the commander rides a Lintel into the yard and enters the switch room alone while the neutral gatekeeper is present
- **THEN** the yard is contested for opposing claims but the Assembly claim begins only when a Gate Section enters the room.
