# Complete the Orra Basin commander campaign

## Why

At the factory door in Gate Six, a commander can board the first Lintel and order its crew forward. With six alloy left, a request for a second four-alloy Lintel is rejected. The game records whether the side has armor. Fielding several crews, keeping track of each one from inside a vehicle, and bringing them home is the next step toward the original RTS design.

The seven playable operations on `main` already connect ground, ferry, air and orbital missions. This proposal covers the remaining simulation, interior, targeting and campaign work, plus the two outstanding human playtests. [Playable baseline at 4255587](https://github.com/stbange/1prts/tree/425558792d98346cad9f57bf2690ae272c422080) · [Original concept](../../../docs/concept.md)

## What Changes

- Give surveyed sites, salvage lots, production jobs, groups, hulls and people individual identities. Both sides compete for resources and direct multiple field units from first person.
- Give each catalog entry a job that changes the simulation. Qualify the interiors of all 31 craft and 30 structures, including friendly and rival examples. Exercise the eight personnel types as crews and passengers.
- Use one selection and order system for desktop, touch, wrist slate and controller pointing throughout the campaign.
- Add campaign saves, resume, eligible crew and equipment transfers, and independent operation selection. Give each of the seven operations a replay that demonstrates its strategic decision.
- Carry new content through the loose-file mod API, mobile and headset performance checks, the independent 95% unit coverage gates, rendered game tests, screenshots and observed playtests.

## Capabilities

### New Capabilities

- `army-operations` covers repeated production, contested stocks, group orders, recovery and an opponent that acts on reports.
- `inhabited-interiors` covers working compartments, scale, per-type completion and authorable sectors.
- `world-targeting` covers shared target selection and confirmation across all input devices.
- `campaign-continuity` covers durable saves, mission transitions and transfers through the seven operations.

## Impact

The work touches the Lua mission rules, the C simulation and renderer, desktop and web input, persistence, external content and release verification. The existing first-playable change owns the physical-headset and first-time-player sign-offs. This plan schedules the sessions and links the evidence back to those items. [Implementation checklist](tasks.md) · [Architecture, roster and mission acceptance](design.md) · [First-playable checklist](../define-first-command/tasks.md)
