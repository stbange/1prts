# Army operations

## ADDED Requirements

### Requirement: Produce independently commandable crews

A staffed, powered facility SHALL be able to repeat an eligible production order when a surveyed site or exit, crew and funds are available. Each accepted order SHALL reserve its own inputs and deliver one instance with a distinct identity and crew.

#### Scenario: Ride the second tank

- **GIVEN** the Assembly owns a Lintel, its Motor Hall has a qualified crew and six alloy are free
- **WHEN** the commander orders another four-alloy Lintel and boards it on rollout
- **THEN** two alloy remain free, both call signs can receive different orders, and the commander's first-person view follows the second hull.

#### Scenario: A bay is blocked during a handover

- **WHEN** a paid unit is ready at an occupied exit, the commander repeats confirmation and the road later clears
- **THEN** the bay reports the blockage, releases one crewed unit and charges for one order.

### Requirement: Compete for finite material

Both factions SHALL be able to attempt collection from the same world-owned salvage lot. The ledger SHALL account for source stock, reservations, cargo in transit and processed alloy once per load.

#### Scenario: Two haulers reach the final load

- **WHEN** both factions reach a lot containing one load on the same tick
- **THEN** one crew takes the load by the deterministic reservation rule, the other receives a new status, and exactly one staffed, powered mill can subsequently credit four alloy for it.

### Requirement: Direct groups using terrain and observations

Crews SHALL execute role-appropriate movement, scouting, escort, engagement, hold, withdrawal and support orders against reachable destinations. An opponent SHALL choose orders through the same economy, terrain, information and objective rules.

#### Scenario: A route and a contact change

- **WHEN** a Mole takes a different legal route and an enemy last reported at the original yard moves out of sight
- **THEN** its assigned escort follows the Mole, the scout's earlier report shows its increasing age, and support sent to the reported yard resolves against the occupants actually present.

#### Scenario: The rival chooses between competing jobs

- **GIVEN** two otherwise equivalent Compact starts with different scouting reports
- **WHEN** one reports an occupied approach and the other reports an exposed salvage opportunity
- **THEN** the rival can issue different legal orders, pay their costs and contest the switch room using the same 60-second rule as the player.

### Requirement: Recover an occupied disabled vehicle

A disabled vehicle SHALL retain its instance, crew, interior and cargo, cease contributing operational strength and accept compatible recovery work.

#### Scenario: Restore a field hull

- **WHEN** a Hinge crew tows a disabled occupied hull to a staffed service berth and repair finishes
- **THEN** the commander can reassign the restored vehicle under its original call sign, and its crew and cargo are accounted for once.
