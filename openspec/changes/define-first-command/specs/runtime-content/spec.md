# Runtime content

## Purpose

Let players and authors change the game's rules and media in a distributed build using ordinary editable files.

## ADDED Requirements

### Requirement: Load loose game content

Graphics, audio, shaders and scripts SHALL ship as individually editable files in content folders beside the native executable or web loader and Wasm module. An external manifest beside those folders SHALL identify their contents. The browser SHALL load individual content paths from the deployment. Native lookup SHALL be independent of the launch working directory.

#### Scenario: Change media after installation

- **WHEN** a pack changes the ferry livery and a radio sound, and the same build is relaunched from a different working directory or reloaded under a web URL prefix
- **THEN** the ferry and radio use the replacements and the native executable or Wasm module retains its original checksum.

### Requirement: Extend game rules through a versioned Lua interface

An external Lua pack SHALL be able to add a mission, register a unit or job, subscribe to gameplay events and override a declared content ID using a compatible API version. Enabled packs SHALL load in declared order. Invalid versions, missing required files and undeclared duplicate IDs SHALL identify the offending pack and cause before play.

#### Scenario: Extend the tide deadline

- **WHEN** a compatible pack overrides the mission deadline from 720 to 900 seconds and the commander starts a new mission on native or web
- **THEN** the briefing shows 15:00, time 12:00 passes with an unfinished objective still active and time 15:00 ends it with the original executable or Wasm module.

#### Scenario: Add a second mission

- **WHEN** a compatible enabled pack registers a new mission ID and a handler for arrival at a berth
- **THEN** the new mission is selectable alongside Tide Window and its handler receives the arrival during play.

#### Scenario: Diagnose an invalid pack

- **WHEN** a pack references a missing ferry model
- **THEN** the loader identifies the pack and model path before starting the mission.

### Requirement: Distribute source and art with their licenses

Project code, including Lua and shaders, SHALL carry the MIT license. Original art and audio SHALL carry CC BY 4.0. The distribution SHALL provide the license texts and per-work credits identifying title, creator, source, license and modifications for the art.

#### Scenario: Attribute a modified texture

- **WHEN** an original texture is adapted for a shipped livery
- **THEN** its credits identify the original creator and source, link CC BY 4.0 and describe the change, and both license texts are available in the distribution.
