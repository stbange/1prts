# Inhabited interiors

## ADDED Requirements

### Requirement: Qualify every crewed type through ordinary play

Each shipped craft and structure SHALL have a metre-scale exterior, a traversable normal entrance, an occupied operating station, a distinct area with an observable function and a connected exit. Crew, solid machinery and station state SHALL agree with the simulation. Each personnel type SHALL be observable doing its assigned work or boarding its transport.

#### Scenario: Inspect the entire roster

- **WHEN** the per-type routes are played for the 31 craft, 30 structures and eight personnel types
- **THEN** each craft and structure has evidence of the entrance-to-work-area-to-exit walk, and each type has evidence of a role-specific state change. The routes cover idle, moving, newly commissioned and rival examples where applicable.

#### Scenario: Trace a load at the depot

- **WHEN** the commander accompanies a shipment from the apron through the 65-metre Shore Depot and up to its gantry
- **THEN** the dock crew and dispatcher show the same shipment's progress, the equipment blocks walking through its solid surfaces, and the gantry overlooks the actual loading operation.

### Requirement: Traverse moving and vertically connected rooms

Ramps, ladders, lifts, hatches and collars SHALL connect compatible surfaces at their real locations. Changing frames or rendering detail SHALL preserve the commander's pose, occupant identities and ongoing work.

#### Scenario: Ride the station lift

- **WHEN** the commander crosses the docked tender's collar, calls the lift and selects traffic control
- **THEN** the doors and platform stop at the selected landing, the commander can walk out at eye level, and an active surface job continues throughout.

#### Scenario: Cross from a nested moving vehicle

- **WHEN** the commander rides in the crawler on a turning ferry, leaves the cab and disembarks at a docked ramp
- **THEN** local drift while standing still stays below 2 cm, the steps across the connected surfaces stay continuous and the commander remains on the quay when the ferry departs.

### Requirement: Author working sectors in loose content

A versioned Lua pack SHALL be able to register a sector, compatible crewed asset, interior, target and mission job. The distribution SHALL load those definitions and their attributed media beside the executable or web loader.

#### Scenario: Play an added sector with an existing pack

- **WHEN** an author enables the test sector alongside the existing Long Watch pack and launches the native or web distribution
- **THEN** the added sector is selectable, its crew accepts the new job from aboard its asset and the declared Long Watch overrides still take effect using the same binary.
