## MODIFIED Requirements

### Requirement: Load loose game content

Graphics, audio, shaders and scripts SHALL ship as individually editable files in content folders beside the native executable or web loader and Wasm module. An external manifest beside those folders SHALL identify their contents. The browser SHALL load individual content paths from the deployment. Native lookup SHALL be independent of the launch working directory. Asset failures SHALL identify the file and offer retry without replacing the last playable state. Bootstrap SHALL show stage and progress. An unsupported renderer SHALL direct the player to native builds.

#### Scenario: Change media after installation

- **WHEN** a pack changes the ferry livery and a radio sound, and the same build is relaunched from a different working directory or reloaded under a web URL prefix
- **THEN** the ferry and radio use the replacements and the native executable or Wasm module retains its original checksum.

#### Scenario: Recover an interrupted content load

- **WHEN** a sector model fetch fails and is retried
- **THEN** only the missing asset is fetched again and a single valid operation is opened.
