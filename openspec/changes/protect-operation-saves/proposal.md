# Protect independent operations

## Why

A new operation could replace the only saved watch. An entry checkpoint and independent manual slots let a player revisit the field with the original crew and position.

## What Changes

Keep two manual watches and a separate checkpoint, recover each slot from its own previous committed version and display storage failures.

## Capabilities

### Modified Capabilities

- `campaign-continuity` covers restore a committed game at its simulation tick.

## Impact

Local save files, the browser IndexedDB adapter and legacy Watch A saves. The delta extends the existing campaign-continuity requirement.
