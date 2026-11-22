# Fit the active input device

## Why

A phone can rotate or gain a keyboard while a crew assignment is open. The player needs to keep the selected crew and reach its next action.

## What Changes

Track the most recent input, project the same actions into safe-area-aware touch controls and update binding labels when the player changes device.

## Capabilities

### Modified Capabilities

- `platform-play` covers switch mobile web to touch controls automatically.

## Impact

First-person HUD prompts, browser controls and the XR wrist slate. The delta extends the existing platform-play requirement.
