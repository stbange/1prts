## ADDED Requirements

### Requirement: Session desk and guarded transitions

On startup the command desk SHALL offer New, Continue, Load, Settings, Credits and exit. A separate pause action and platform interruption SHALL freeze the clock until explicit resume. Destructive actions SHALL identify their target and default to Cancel.

#### Scenario: Protect the live field session

- **WHEN** the player selects another mission from an unsaved Gate Six session and cancels
- **THEN** the same commander, queue, resources and clock are preserved.
