# Identify builds and local failures

## Why

A player reporting a failed operation needs to identify the exact build and level and decide whether to share the details.

## What Changes

Stamp the executable and distribution with the source revision, show representative captures and keep detectable failure reports local until copied.

## Capabilities

### Modified Capabilities

- `field-prototype` covers verify the rendered game.

## Impact

Build configuration, local diagnostics and the public release page. The delta extends the existing field-prototype requirement.
