# Campaign catalog

## Purpose

Make the scale, ownership and function of the expanded Orra Basin roster inspectable in the game itself.

## ADDED Requirements

### Requirement: Expose the expanded campaign roster

The field guide SHALL list 39 unit types, 30 facility or objective types, six factions, six named missions and six upgrades from loose Lua content. Shared types SHALL count once.

#### Scenario: Inspect the theater

- **WHEN** the catalog loads
- **THEN** Gate Six references 18 distinct unit IDs and 18 distinct structure IDs from the campaign roster.

#### Scenario: Read a unit entry

- **WHEN** the commander pages through a field-guide category
- **THEN** the slate displays the selected entry's name, ownership and role while the camera stays with the commander.
