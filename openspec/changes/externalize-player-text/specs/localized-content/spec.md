## ADDED Requirements

### Requirement: Load localized player-facing text and fonts

The distribution SHALL declare English fallback, load editable text by stable key and support pack-supplied glyphs. Missing keys SHALL fall back independently. Long text SHALL wrap or scroll with actions accessible.

#### Scenario: Load translated interface content

- **WHEN** an incomplete language pack supplies a long Cyrillic label and omits a description
- **THEN** the label renders with its supplied glyphs, the description falls back to English and the order is reachable.
