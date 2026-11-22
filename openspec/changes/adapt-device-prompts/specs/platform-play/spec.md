## MODIFIED Requirements

### Requirement: Switch mobile web to touch controls automatically

On a touch-first mobile browser the game SHALL present touch controls on entry. Touch SHALL support simultaneous movement and looking, interaction, crew selection, job and target selection, confirmation and cancellation. Rotation or a switch of active input SHALL preserve the current order draft. Device prompts SHALL follow the active input and binding. All touch menu actions SHALL be accessible within safe areas with 44 CSS-pixel targets and reachable in portrait, landscape and wide windows.

#### Scenario: Complete an operation using fingers

- **WHEN** a player opens the game on a touch-first phone and plays through Gate Six with touch alone
- **THEN** the touch layout appears automatically and the player can mark a foundation, queue a unit and order the occupation from first person.

#### Scenario: Rotate during command entry

- **WHEN** the player selects a crew and rotates the phone before confirming its job
- **THEN** the controls reflow and the selected crew and draft job persist until confirmed or canceled.

#### Scenario: Fit the active input device

- **WHEN** a player rotates a 360-pixel phone with a crew selected and switches to a keyboard
- **THEN** the draft survives and the new labels and confirmation remain reachable.
