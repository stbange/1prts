## ADDED Requirements

### Requirement: Apply and retain player preferences

Players SHALL be able to apply, cancel and reset applicable graphics, audio, language, text size, difficulty and camera comfort settings. Applied settings SHALL survive a relaunch.

#### Scenario: Keep player preferences

- **WHEN** the commander applies larger text and muted music, previews a reset, cancels and relaunches
- **THEN** the larger text and muted music remain active while the radio channel is independent.

### Requirement: Preserve drafts during interruption

Settings SHALL retain unapplied edits while the application loses focus. Cancel SHALL discard the edits. Browser-managed display settings SHALL identify that restriction.

#### Scenario: Cancel a draft after switching applications

- **WHEN** the commander edits the music level, switches applications, returns and cancels
- **THEN** the last applied level is used in play and after relaunch.
