# Crewed world

## Purpose

Make the size and staffing of every facility and craft tangible to a commander who can board, explore and observe the work.

## ADDED Requirements

### Requirement: Ship an explorable interior for every building and craft

Every building, ground vehicle, ship, aircraft, spacecraft and space station introduced into play SHALL have metre-scale geometry, people on watch, a physical entrance and connected walkable spaces leading to an operating station and a second functional area. This SHALL apply to friendly and rival assets, including idle craft.

#### Scenario: Inspect a newly introduced asset type

- **GIVEN** an instance of any of the six families in an accessible berth, landing or site
- **WHEN** the commander uses its normal entrance
- **THEN** they can walk to an occupied operating station and a distinct work or service space at a consistent human scale, look back along the route and leave through a connected exit.

#### Scenario: See a ship sized for its cargo

- **WHEN** the 22 m crawler boards the 96 m ferry and the commander walks to the upper deck
- **THEN** the crawler clears the ramp and deck opening and is visible in the vehicle bay below at its original scale.

### Requirement: Carry occupants through motion and frame changes

A moving craft SHALL carry the commander, crew and embarked vehicles. Movement through a connected portal SHALL preserve a continuous first-person pose at the crossing, including when passing between nested moving interiors.

#### Scenario: Stand inside a crawler aboard a moving ferry

- **WHEN** the ferry translates and turns while the commander stands still in the secured crawler
- **THEN** the commander's local position relative to the cab changes by less than 2 cm over a full crossing and their world pose follows both craft.

#### Scenario: Disembark after docking

- **WHEN** the commander walks across the aligned ferry ramp onto the quay and the ferry subsequently departs
- **THEN** the step across the seam fits the ordinary walking movement for that tick, the eye pose stays continuous, and the commander remains on the quay.

### Requirement: Crew presence governs operation

An operating facility or craft SHALL have assigned crew at its required stations. Unloading and reloading an interior for rendering SHALL preserve crew identities and ongoing work.

#### Scenario: Return to a crew on watch

- **WHEN** the commander leaves the depot during a loading job and returns after the crew has completed it
- **THEN** the same assigned workers are present, the job log records one completion and the delivered cargo is accounted for once.

#### Scenario: Reassign the required fitter

- **WHEN** the only fitter is assigned away from a queued workshop job
- **THEN** that job reports it is waiting for a fitter and can resume when a qualified crew member is available.

### Requirement: Show scale and work through retro 3D art

Playable scenes SHALL combine traversable 3D geometry with sprites, a limited color palette and low-resolution world textures. Work signals and text SHALL remain legible at their intended viewing distance. Voxel dressing SHALL be optional for gameplay.

#### Scenario: Read a loading signal on approach

- **WHEN** the commander approaches the ferry through spray sprites and steps through its 3D bow opening
- **THEN** the open ramp admits the commander and its ready signal is recognizable by shape or label as well as color, with voxel effects disabled.
