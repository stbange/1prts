# Field command

## Purpose

Let the commander direct several independent crews and learn about the operation from a physical place in the world.

## ADDED Requirements

### Requirement: Command from the commander's eyes

The game SHALL present all playable views from the commander's body and eye pose. Charts SHALL be physical displays showing schematic symbols. Construction placement, production, combat orders, boarding, briefings and mission outcomes SHALL preserve the first-person perspective.

#### Scenario: Build and command from inside a tank

- **WHEN** the commander marks a surveyed building pad, queues a unit in a powered hall and orders a section forward while riding a Lintel
- **THEN** the view stays at the commander's eyes inside the tank, its crew keeps driving and the slate shows the pad, production entry and section acknowledgment.

#### Scenario: Plot a course on a moving bridge

- **WHEN** the commander raises the slate, selects the public quay and confirms a sailing order while on the ferry bridge
- **THEN** the slate shows the selected destination, the world view still originates at the commander's eyes on the bridge, and lowering the slate reveals the ferry's current surroundings.

### Requirement: Delegate concurrent work

The commander SHALL be able to select a crew by sight or call sign, specify a job and target, confirm, inspect, cancel a queued order or redirect an active crew. Crews SHALL continue accepted work while the commander moves or uses another station.

#### Scenario: Send an order to shore from the vehicle deck

- **WHEN** a commander aboard a sailing ferry assigns a job to a reachable shore crew
- **THEN** the shore crew acknowledges and begins the job, the pilot continues the crossing, and both jobs appear under their respective call signs.

#### Scenario: Diagnose and revise a blocked order

- **WHEN** the pilot reports the public quay blocked and the commander redirects a ramp-equipped ferry to the slip
- **THEN** the log identifies the pilot, berth conflict and new destination, and the pilot proceeds to the slip.

### Requirement: Distinguish reports from current observation

Remote contacts and locations SHALL display the source and age of their last report. Crew acknowledgments, blockages and completions SHALL have a readable equivalent to their audio cues.

#### Scenario: A contact moves outside observation

- **WHEN** a reported contact moves beyond sight and supplies no further report
- **THEN** the chart keeps its last reported position and increasing age until new information arrives.
