# Recover an interrupted content load

## Why

A failed sector model download used to strand the commander between watches. The loaded sector can remain playable while an individual file is recovered.

## What Changes

Show startup progress, identify the failed file, allow retry or cancel and keep the running simulation intact until the destination loads.

## Capabilities

### Modified Capabilities

- `runtime-content` covers load loose game content.

## Impact

Streaming web loader and the C operation transition. The delta extends the existing runtime-content requirement.
