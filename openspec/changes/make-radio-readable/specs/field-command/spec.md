## MODIFIED Requirements

### Requirement: Distinguish reports from current observation

Remote contacts and locations SHALL display the source and age of their last report. Crew acknowledgments, blockages and completions SHALL have a readable equivalent to their audio cues. Essential speech and sound SHALL have labeled captions with adjustable size and background and reviewable history. Crew voices SHALL remain intelligible over the score and location ambience, with ducking of at least 6 dB during calls and restoration within a second. Critical signals SHALL have non-color cues.

#### Scenario: A contact moves outside observation

- **WHEN** a reported contact moves beyond sight and supplies no further report
- **THEN** the chart keeps its last reported position and increasing age until new information arrives.

#### Scenario: Keep dispatch readable below deck

- **WHEN** a player receives a blocked-berth report with speech muted below deck
- **THEN** the pilot, subject, time and reason remain readable and the chart distinguishes reported contacts by shape.
