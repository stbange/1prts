# Protect the live field session

## Why

A live orders slate lets the crews keep working. The commander also needs a way to put a watch down, particularly when switching applications or choosing another mission.

## What Changes

Freeze the simulation at the desk and on focus loss. Confirm mission changes, restarts and departure from a dirty watch, with Cancel focused first.

## Capabilities

### New Capabilities

- `session-flow` covers session desk and guarded transitions.

### Modified Capabilities

- `platform-play` covers pausing on the wrist and losing headset visibility.

## Impact

`src/session.c`, platform interruption handlers and the browser desk.
