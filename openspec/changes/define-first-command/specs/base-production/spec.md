# Base production

## Purpose

Give the two basin armies a visible resource cycle, walkable production bases and military forces with distinct uses.

## ADDED Requirements

### Requirement: Reclaim a finite load before spending it

A crewed hauler SHALL remove plate from a surveyed yard, carry it to a staffed, powered Plate Mill and credit alloy once per processed load. The ledger SHALL distinguish available stock, reserved stock and cargo in transit for each faction.

#### Scenario: Follow a load with the commander

- **WHEN** the commander rides a full crawler from the yard and walks into the batch office during unloading
- **THEN** the yard has one fewer load, the load earns no alloy while in transit, and completing the batch adds four available alloy to the owner's ledger exactly once.

### Requirement: Commission and staff production facilities

Both factions SHALL use surveyed pads, fitters, alloy and an operating crew to add facilities. A commissioned Plate Mill SHALL unlock the Watch House and Motor Hall. Accepted jobs SHALL reserve their inputs without double spending. A production queue SHALL retain its progress when power or operators are unavailable.

#### Scenario: Check the production prerequisite from the field

- **WHEN** the commander inspects a Motor Hall plan before the first Plate Mill is commissioned
- **THEN** the slate names the missing mill, leaves the pad and alloy available, and offers the plan for placement once the mill is commissioned.

#### Scenario: Cancel a queued foundation

- **WHEN** the commander reserves a pad and six alloy for a Motor Hall, repeats the confirmation and cancels before work starts
- **THEN** there is one reservation, the pad and six alloy become available once on cancellation, and no duplicate building exists.

#### Scenario: Restore a factory during a shift

- **WHEN** a vehicle is partway through production, the Motor Hall loses power, and its power is restored while the hall is staffed
- **THEN** the crew reports the interruption, the same job resumes at its recorded progress, and one crewed vehicle leaves the bay for one payment.

### Requirement: Field identifiable forces with different capabilities

The Assembly SHALL muster Gate Sections and build Lintel tanks. The Compact SHALL muster Channel Sections and build Wake amphibious carriers. Both line sections SHALL be able to occupy a switch room. A Wake SHALL carry a section through marked shallows. A Lintel SHALL use dry routes and have the advantage in a sustained yard contest.

#### Scenario: Cross from the Compact bank

- **WHEN** a Wake carrying a Channel Section and a Lintel each receive a destination across a marked shallow lane
- **THEN** the Wake can cross and disembark its section at the waterside door, and the Lintel crew identifies the dry causeway as its available route.

#### Scenario: Meet on the dry yard

- **GIVEN** a staffed, full-readiness Lintel and Wake holding the same dry yard without reinforcements
- **WHEN** their sustained contest resolves
- **THEN** the Wake yields first and the Lintel remains able to contest the approach.

### Requirement: Expand a base according to its faction

When introduced in the campaign, the Assembly's Accumulator Gallery SHALL let one designated bay finish one job on a charged reserve during a shortage. The Compact's Slip Shed SHALL be towable between surveyed moorings and SHALL suspend repairs in transit.

#### Scenario: Spend the Assembly's reserve

- **WHEN** the grid loses capacity with a charged gallery and the designated bay has two jobs queued
- **THEN** its first job can finish on reserve and its second awaits adequate power.

#### Scenario: Relocate a Compact repair berth

- **WHEN** a Hinge moves a staffed Slip Shed from one surveyed mooring to another during a repair
- **THEN** the repair stops advancing while underway and resumes from its recorded progress when the shed is moored and staffed again.
