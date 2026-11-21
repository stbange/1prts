# Campaign continuity

## ADDED Requirements

### Requirement: Restore a committed game at its simulation tick

Native and browser builds SHALL save and resume the commander, mission, content identity and simulation state from the in-game interface. Writes SHALL preserve the last complete save until its replacement is committed. Invalid or incompatible saves SHALL give a readable recovery option.

#### Scenario: Quit in a moving nested interior

- **WHEN** the commander saves inside the crawler on the ferry with a crew working ashore, exits, then resumes and supplies the same subsequent inputs as uninterrupted play
- **THEN** the commander occupies the same cab, the ferry, crew and cargo resume at the saved tick and subsequent checkpoints, payments and mission result agree.

#### Scenario: A save is interrupted or a pack is missing

- **WHEN** a replacement write fails or a load requires an absent pack
- **THEN** a previous valid slot remains readable, the current session stays intact and the slate identifies the failure and available recovery.

### Requirement: Carry eligible assets through mission results

Mission data SHALL declare eligible transfers and starting slots. A campaign victory SHALL record its result, surviving crews and hulls and installed upgrades exactly once. Standalone play and the Compact side operation SHALL keep separate state from the Assembly campaign inventory.

#### Scenario: Continue from Gate Six

- **WHEN** a victorious Assembly commander continues to Tide Window with an eligible surviving Mole
- **THEN** its hull and crew fill the authored crawler slot once, the debrief records their transfer, and starting Tide Window independently supplies its normal starter crew.

#### Scenario: Use an installed upgrade in a later operation

- **WHEN** a later operation calls on a depot that already commissioned the Quay Hoist or accepts a Kestrel fitted with the Extended Bow Ramp
- **THEN** the depot's loading job gains the hoist's effect or the same equipped hull can use a compatible gravel landing. The ledger preserves the upgrade under its original owner without charging for it again.

#### Scenario: Reopen a debrief

- **WHEN** the commander returns to a victory debrief, retries a loss or plays the alternate faction's operation
- **THEN** completed rewards are granted once, retry restores its own entry state, and each faction retains its own stock and units.
