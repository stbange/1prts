## MODIFIED Requirements

### Requirement: Restore a committed game at its simulation tick

Native and browser builds SHALL save and resume the commander, mission, content identity and simulation state from the in-game interface. Writes SHALL preserve the last complete save until its replacement is committed. Invalid or incompatible saves SHALL give a readable recovery option. The interface SHALL provide two independent manual slots and a separate operation-entry checkpoint, identify their missions and show write, completion and failure status.

#### Scenario: Quit in a moving nested interior

- **WHEN** the commander saves inside the crawler on the ferry with a crew working ashore, exits, then resumes and supplies the same subsequent inputs as uninterrupted play
- **THEN** the commander occupies the same cab, the ferry, crew and cargo resume at the saved tick and subsequent checkpoints, payments and mission result agree.

#### Scenario: A save is interrupted or a pack is missing

- **WHEN** a replacement write fails or a load requires an absent pack
- **THEN** a previous valid slot remains readable, the current session stays intact and the slate identifies the failure and available recovery.

#### Scenario: Protect independent operations

- **WHEN** two manual routes are saved and the next operation starts
- **THEN** both manual slots survive and Checkpoint names the new operation after durable commit.

## ADDED Requirements

### Requirement: Serialize concurrent slot writes

A manual save requested while an automatic checkpoint is committing SHALL capture the state at the input tick and keep each transaction confined to its selected slot.

#### Scenario: Save immediately after boarding

- **WHEN** the commander boards and saves while the checkpoint is still being written
- **THEN** continuing from the manual slot restores the commander aboard, and the checkpoint remains independently loadable.
