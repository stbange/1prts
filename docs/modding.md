# Editing an installed campaign

Open the distribution folder containing `1stprts` or `orra.wasm`. Its `content.json` lists the individual base files and the enabled pack directories. To try the bundled example, replace the empty `packs` array with `["mods/long-watch"]`. Keep the existing `files` list. Restart the application or reload the web page.

Gate Six now asks for a 90-second hold. Tide Window starts with 15:00 on its clock. The Lintel and Kestrel carry an oxide livery, and the dispatch acknowledgment uses a different recording. Removing the directory from `packs` restores the original content.

## Write a pack

Create a directory below `mods/`. Give it a `pack.json`, a Lua entry script, any media it uses and its credits. Pack directories resolve relative to the distribution. File paths within a pack resolve relative to that pack. Enabled directories load in array order.

```json
{
  "api": 1,
  "entry": "init.lua",
  "files": ["init.lua", "CREDITS.md"],
  "overrides": ["gate-six.hold_ticks"]
}
```

```lua
local api = ...
api.override("gate-six.hold_ticks", 75 * 30)
api.job("check-lamps", "Check harbor lamps", function(state, sector)
    state.report = "Port watch reports both approach lamps lit."
    return true
end)
```

| API | Contract |
| --- | --- |
| `api.mission(def)` | Supply a unique `id`, a display `name`, a base `sector` and `build(ctx)`. Return a rules table with `hz`, `hold_ticks`, `actions`, `new`, `command` and `step`. See the shipped mission scripts for the state fields read by the host. |
| `api.unit(def)` | Register a unique `id`, `name`, `role`, `family` and dimensions `w, h, d` in metres. An optional `asset` names a mesh in the pack. A mission can call `ctx.add_entity(key, id, {x,y,z,yaw})` to place the unit and its operators. |
| `api.job(id, label, run)` | Add a slate order. The handler receives the mission state and sector. Return true when the order is accepted. |
| `api.on(event, fn)` | Subscribe to `order`, `complete`, `berth`, `crew` or `result`. The callback receives the state and the order ID, completed job, landing, crew ID or outcome respectively. |
| `api.override(key, value)` | Declare the key in the pack manifest first. Timers use simulation ticks at 30 Hz. A model name ending in `.mesh` or a sound name ending in `.wav` resolves to a replacement file in the pack. |

World coordinates use metres with Y up and angles in radians. A moving entity's frame contains X, Y, Z, yaw and its parent's one-based scene index. Zero selects the world frame. A ferry can append a shore connection flag (1 moored, −1 underway) and its bow ramp reach in metres. An embarked vehicle and its crew keep their local coordinates as the parent moves.

## Reproduce a change

The headless runner accepts `order`, `reject`, `wait`, `alloy` and `result` lines. From any working directory, point it at the installed content and pass `--replay -` to read from standard input.

```text
order hoist
order quay
order install
wait 13050
result victory
alloy 2
```

Use this sequence with `orra-sim --mission tide-window --replay route.txt`. The normal quay-hoist route takes 7:15. For visual input and camera paths, the native game accepts recorded keyboard and mouse actions. [Example ferry crossing](../tests/tide-window.route) · [Working pack](../mods/long-watch/pack.json)

## Sector and campaign extensions

Add `mods/wire-quay` to the `packs` array in the distributed `content.json` to play Wire Quay and Wire Quay / Return. It can share that array with `mods/long-watch`. Choose Wire Quay from Operations. Dispatch the Wire Courier's freight assignment to Stores, stamp the receipt on the slate, and continue to the return shift.

`api.sector` registers a named sector, owner, terrain, units, buildings, spawn, positions and route junctions in metres. `api.unit` accepts a loose mesh, an existing interior template and a field-work role. `api.link` connects two registered operations. A mission also accepts a `next` ID. Read the sample entry and its attribution beside the asset. [Wire Quay](../mods/wire-quay/init.lua) · [Pack manifest](../mods/wire-quay/pack.json)

The base interior register places a named machine in each crewed asset. Positions use fractions of beam and length. The station and lift fittings, 16-pixel surface tile, music and ambient loops all ship as ordinary files. [Room register](../assets/rooms.txt) · [Mesh authoring](../tools/rooms_art.py) · [Audio authoring](../tools/basin_music.py)
