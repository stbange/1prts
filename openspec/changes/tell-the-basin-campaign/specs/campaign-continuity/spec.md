## MODIFIED Requirements

### Requirement: Carry eligible assets through mission results

Mission data SHALL declare eligible transfers and starting slots. A campaign victory SHALL record its result, surviving crews and hulls and installed upgrades exactly once. Standalone play and the Compact side operation SHALL keep separate state from the Assembly campaign inventory. First-person briefings SHALL identify the speaker, faction stakes, objective and first actionable order. The final High Water dispatch SHALL explain the outcome. The local operation record SHALL show completion times and earned accomplishments.

#### Scenario: Continue from Gate Six

- **WHEN** a victorious Assembly commander continues to Tide Window with an eligible surviving Mole
- **THEN** its hull and crew fill the authored crawler slot once, the debrief records their transfer, and starting Tide Window independently supplies its normal starter crew.

#### Scenario: Use an installed upgrade in a later operation

- **WHEN** a later operation calls on a depot that already commissioned the Quay Hoist or accepts a Kestrel fitted with the Extended Bow Ramp
- **THEN** the depot's loading job gains the hoist's effect or the same equipped hull can use a compatible gravel landing. The ledger preserves the upgrade under its original owner without charging for it again.

#### Scenario: Reopen a debrief

- **WHEN** the commander returns to a victory debrief, retries a loss or plays the alternate faction's operation
- **THEN** completed rewards are granted once, retry restores its own entry state, and each faction retains its own stock and units.

#### Scenario: Explain the basin campaign through its crews

- **WHEN** a player restores Crown traffic after an eight-alloy crossing
- **THEN** the final dispatch explains restored service and the record lists the crossing once.
