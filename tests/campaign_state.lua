local root = ...
local runtime = assert(loadfile(root .. "/scripts/runtime.lua"))
local function scene(id)
    local b = runtime(root, id)
    return b.gate_six, b.gate_six.new(), b
end
local function step(r, s, n)
    for _ = 1, n do
        r.step(s)
    end
end
local r, s, b = scene("gate-six")
assert(r.command(s, "move:mole@g-482-1140") and s.instances["g-482-1140"])
assert(not r.command(s, "move:missing@g-400-1188") and not s.instances["g-400-1188"])
assert(not r.command(s, "move:mole@g-999-111"))
assert(r.command(s, "release:mole"))
s.blue.alloy = 20
s.blue.armor = true
s.blue.motor = true
assert(r.command(s, "armor"))
step(r, s, 150)
assert(r.command(s, "move:lintel-2@gate") and r.command(s, "armor"))
step(r, s, 55)
local saved = r.save(s)
local mirror = r.restore(saved)
assert(r.record(mirror) == r.record(s) and r.save(mirror) == saved)
assert(not pcall(r.restore, saved:gsub('"format",1', '"format",9', 1)))
local copy = runtime(root, "gate-six").gate_six
local resumed = copy.restore(saved)
step(r, s, 140)
step(copy, resumed, 140)
assert(r.record(s) == copy.record(resumed))
-- A report ages in place after a visible rival retreats.
r, s, b = scene("gate-six")
local loc = {}
for i, v in ipairs(b.site) do
    loc[v.key] = i
end
s.blue.armor = true
s.red.armor = true
s.frames[loc.lintel] = { 5, 0, -70, 0, 0 }
s.frames[loc["rival-wake"]] = { 10, 0, -76, 0, 0 }
assert(r.command(s, "engage:lintel@rival-wake"))
step(r, s, 600)
assert(
    s.tactical.health["rival-wake"] == 0
        or s.tactical.orders["rival-recovery"]
        or s.tactical.posted["rival-recovery"]
)
step(r, s, 1200)
assert(s.instances["rival-wake"].status == "ready" and s.instances["rival-wake"].crew[1])
assert(r.command(s, "support:span@common-lot"))
step(r, s, 1900)
assert(s.crossing)
assert(r.command(s, "support:tally@gate"))
step(r, s, 1600)
assert(s.watchposts.gate == "tally")
-- Same public field job, with one observable receipt for every catalog type.
local tested = {}
for _, mission in ipairs(b.operations) do
    r, s, b = scene(mission.id)
    if s.army then
        for _, flag in ipairs({ "mill", "watch", "motor", "section", "armor" }) do
            s.blue[flag] = true
        end
    end
    for _, unit in ipairs(b.site) do
        if unit.role and unit.owner ~= "rival" and not tested[unit.type] then
            local key = unit.key
            assert(r.command(s, "work:" .. key .. "@" .. key), key)
            assert(not r.command(s, "work:" .. key .. "@" .. key), key)
            s.tick = s.service[key].finish - 1
            r.step(s)
            local receipt = s.works.receipts[key]
            assert(receipt and receipt.target == key and receipt.message, unit.type)
            tested[unit.type] = receipt.job
        end
    end
end
for _, list in ipairs({ b.catalog.units, b.catalog.buildings }) do
    for _, entry in ipairs(list) do
        assert(tested[entry.id], entry.id)
    end
end
r, s, b = scene("high-water")
assert(r.command(s, "lift-call-1") and not r.command(s, "lift-call-0"))
step(r, s, 121)
assert(s.lift.height == 6 and r.command(s, "lift-call-0"))
step(r, s, 121)
assert(s.lift.height == 0)
assert(r.command(s, "passage") and r.command(s, "module"))
step(r, s, 50)
assert(s.jobs.module.elapsed == 0)
assert(r.command(s, "passage"))
step(r, s, 541)
assert(s.freight.items["pressure-module"] == "lark")
r, s, b = scene("tide-window")
assert(r.command(s, "ramp") and r.command(s, "slip") and r.command(s, "install"))
step(r, s, 6000)
assert(r.command(s, "crew:reed@rival-ferry"))
saved = r.save(s)
copy = runtime(root, "tide-window").gate_six
resumed = copy.restore(saved)
step(copy, resumed, 200)
step(r, s, 200)
assert(copy.record(resumed) == r.record(s))
step(r, s, 16000)
assert(s.result == "victory" and r.next(s) == "westbound")
local ledger = r.ledger(s)
assert(r.ledger(s) == ledger)
local gate, old = scene("gate-six")
old.result = "victory"
local previous = gate.ledger(old)
r, s = scene("tide-window")
assert(r.enter(s, previous) and s.instances.mole.veterans)
local compact, cs = scene("compact-bank")
compact.enter(cs, previous)
assert(not cs.instances.mole.veterans)
r, s = scene("night-mail")
r.enter(s, ledger)
assert(not s.linked_hoist and r.command(s, "handoff") and not r.command(s, "handoff"))
local ferry, port = scene("tide-window")
port.result = "victory"
port.gear = "hoist"
r, s = scene("night-mail")
r.enter(s, ferry.ledger(port))
assert(s.linked_hoist and r.command(s, "handoff") and s.handoff == 150)
local original = io.open
io.open = function(file, mode)
    if file == root .. "/content.json" then
        return {
            read = function()
                return '{"version":1,"packs":["mods/long-watch","mods/wire-quay"]}'
            end,
            close = function() end,
        }
    end
    return original(file, mode)
end
r, s, b = scene("wire-quay")
assert(r.enter(s, ledger) and s.inherited_ramp)
assert(r.command(s, "gravel-call") and not r.command(s, "gravel-call") and not r.command(s, "waybill"))
assert(r.command(s, "work:quay-courier@stores"))
s.tick = s.service["quay-courier"].finish - 1
r.step(s)
assert(r.command(s, "waybill") and r.next(s) == "wire-return")
r, s = scene("wire-return")
assert(not r.command(s, "gravel-call"))
s.tick = 5399
r.step(s)
assert(s.result == "defeat")
io.open = original
