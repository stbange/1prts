local root = ...
local runtime = assert(loadfile(root .. "/scripts/runtime.lua"))
local function setup(id)
    local b = runtime(root, id or "gate-six")
    return b.gate_six, b.gate_six.new(), b.site
end
local function ticks(g, s, n)
    for _ = 1, n do
        g.step(s)
    end
end
local function find(site, key)
    for i, row in ipairs(site) do
        if row.key == key then
            return i, row
        end
    end
end
local g, s, site = setup()
local base = #site
s.blue.alloy = 6
s.blue.motor, s.blue.armor = true, true
orra_reserve_scene = function()
    return false
end
assert(not g.command(s, "armor") and s.blue.alloy == 6 and not next(s.army.bays))
orra_reserve_scene = nil
assert(g.command(s, "armor") and s.blue.alloy == 2 and s.blue.reserved == 4 and not g.command(s, "armor"))
assert(g.command(s, "cancel") and s.blue.alloy == 6)
assert(g.command(s, "armor"))
ticks(g, s, 150)
local i, second = find(site, "lintel-2")
assert(second.type == "lintel" and second.owner == "weir" and site[i + 1].parent == i and #site == base + 4)
assert(s.blue.reserved == 0 and g.inspect(s, "mole"):find("cargo"))
s.blue.alloy = 90
assert(g.command(s, "armor"))
ticks(g, s, 165)
assert(
    s.army.bays.motor.left == 0 and not find(site, "lintel-3") and g.inspect(s, "motor"):find("exit occupied")
)
assert(g.command(s, "move:lintel-2@west-pad"))
ticks(g, s, 100)
assert(find(site, "lintel-3") and not s.army.bays.motor and #site == base + 8)
assert(g.command(s, "hold:lintel@motor") and g.command(s, "scout:lintel-2@gate"))
assert(g.command(s, "group:lintel") and g.command(s, "group:lintel-2"))
assert(g.command(s, "move:lintel@east-pad"))
assert(s.tactical.orders.lintel.slot ~= s.tactical.orders["lintel-2"].slot)
assert(g.command(s, "group:lintel-2") and g.command(s, "withdraw:lintel-2@west-pad"))
assert(g.command(s, "release:lintel-2") and not g.command(s, "release:lintel-2"))
assert(not g.command(s, "escort:lintel@rival-wake") and not g.command(s, "scout:lintel@missing"))
assert(not g.command(s, "group:motor") and not g.command(s, "group:missing"))
s.blue.mill = true
assert(g.command(s, "build:motor@west-pad") and not g.command(s, "build:motor@west-pad"))
assert(not g.command(s, "build:watch@missing") and not g.command(s, "bay:armor@missing"))
assert(g.command(s, "cancel") and not s.army.sites["west-pad"])
assert(g.command(s, "build:motor@west-pad"))
ticks(g, s, 150)
assert(find(site, "motor-2") and s.army.sites["west-pad"].building == "motor-2")
assert(g.command(s, "bay:armor@motor-2") and not g.command(s, "bay:section@motor-2"))
assert(g.command(s, "bay:power@motor-2"))
ticks(g, s, 1)
assert(s.army.bays["motor-2"].left == 150)
assert(g.command(s, "bay:power@motor-2") and g.command(s, "bay:priority@motor-2"))
ticks(g, s, 1)
assert(g.command(s, "cancel") and not s.army.bays["motor-2"])
g, s, site = setup()
assert(g.command(s, "common") and not g.command(s, "build:unknown@west-pad"))
assert(g.army.lot(s, s.red, "common-lot"))
ticks(g, s, 150)
assert(s.army.lots["common-lot"] == 0 and s.blue.stock == 1 and s.red.stock == 0 and s.red.haul == 0)
assert(g.inspect(s, "common-lot"):find("0 loads") and not g.command(s, "common"))
s.blue.mill = true
ticks(g, s, 150)
assert(s.blue.alloy == 12 and s.blue.stock == 0)
s.red.motor, s.red.armor, s.red.alloy = true, true, 12
assert(g.army.queue(s, s.red, "armor"))
ticks(g, s, 150)
local _, wake = find(site, "rival-wake-2")
assert(wake and wake.owner == "rival" and wake.type == "wake")
assert(not g.command(s, "move:rival-wake-2@gate"))
local records = assert(loadfile(root .. "/scripts/records.lua"))()
local decode = assert(loadfile(root .. "/scripts/manifest.lua"))()
local v = { ["3"] = 'a"b', [3] = "c\n", frac = 0.0625, truth = false }
local encoded = records.encode(v)
assert(encoded == records.encode(records.decode(encoded, decode)))
assert(not pcall(records.encode, function() end))
v.loop = v
assert(not pcall(records.encode, v))
assert(not pcall(records.decode, "[2]", decode))
assert(not pcall(records.decode, "[1,2,1,3]", decode) and not pcall(records.encode, { [true] = 5 }))
assert(not pcall(records.encode, math.huge))
assert(g.record(s) == g.record(s))
local old = #site
s = g.new()
assert(#site < old and not find(site, "rival-wake-2"))
g, s = setup("westbound")
assert(not g.command(s, "move:shunter@quarters") and g.command(s, "move:shunter@siding"))
ticks(g, s, 1000)
assert(s.tactical.posted.shunter)
g, s = setup("tide-window")
assert(not g.command(s, "move:mole@relay") and g.command(s, "escort:porter@mole"))
ticks(g, s, 700)
assert(s.tactical.posted.porter)
g, s = setup("high-water")
assert(not g.command(s, "move:dray@station") and g.command(s, "move:lark@station"))
ticks(g, s, 1650)
assert(s.tactical.posted.lark)

-- The qualified fitter changes rooms. The other factory keeps working.
g, s, site = setup()
s.blue.mill, s.blue.watch, s.blue.motor, s.blue.armor, s.blue.alloy = true, true, true, true, 30
assert(g.command(s, "armor") and g.command(s, "section"))
assert(g.command(s, "bay:staff@motor"))
local seat = s.army.relief.motor
assert(seat and s.instances[seat].parent == "quarters")
local before = s.army.bays.motor.left
ticks(g, s, 40)
assert(s.army.bays.motor.left == before and s.blue.queues.watch.left < 90)
s = g.restore(g.save(s))
assert(s.army.bays.motor.left == before and g.command(s, "bay:staff@motor"))
ticks(g, s, 1)
assert(s.army.bays.motor.left == before - 1 and s.instances[seat].parent == "motor")
-- A blocked road has a surveyed passing lane. A rail junction keeps its reservation.
g, s, site = setup()
assert(g.command(s, "move:mole@gate"))
local j = s.tactical.orders.mole
j.path = { { 6, 0, -24, "road-6--24" }, { 3, 0, -54 } }
j.leg = 1
s.tactical.locks["road-6--24"] = "fitters"
ticks(g, s, 90)
assert(j.path[1][4]:match("layby") and s.tactical.locks["road-6--24"] == "fitters")
-- A sighted armored hull and a utility crew produce different paid rival responses.
for _, pair in ipairs({ { "lintel", "armor" }, { "mole", "section" } }) do
    g, s, site = setup()
    local key, recipe = pair[1], pair[2]
    s.red.motor, s.red.watch, s.red.armor, s.red.section, s.red.alloy = true, true, true, true, 20
    s.blue.armor = true
    s.tactical.intel[key] = { observer = "rival-survey", target = key, x = 19, z = -45, tick = s.tick }
    g.tactics.reinforcement(s)
    local _, job = next(s.army.bays)
    assert(job and job.id == recipe and s.tactical.claims[key] and s.red.alloy < 20)
    s.tick = 400
    g.tactics.reinforcement(s)
    assert(s.tactical.intel[key].diverted and s.tactical.orders["rival-wake"].reported[1] == 19)
end
g, s, site = setup()
s.blue.mill, s.blue.alloy = true, 30
assert(g.command(s, "build:dynamo@bank-pad"))
ticks(g, s, 122)
assert(s.army.sites["bank-pad"].building == "dynamo-2")
