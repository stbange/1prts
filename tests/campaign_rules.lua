local root = ...
local runtime = assert(loadfile(root .. "/scripts/runtime.lua"))
local decode = assert(loadfile(root .. "/scripts/manifest.lua"))()
assert(decode([[{"a\u0062":["\n",true,false,-42]}]]).ab[4] == -42)
for _, bad in ipairs({
    '{"x":1,"x":2}',
    '["\\v"]',
    '"\\uZZ00"',
    '["x"',
    '"bad\n"',
    '"open',
    "{} tail",
    "{1",
    "null",
}) do
    assert(not pcall(decode, bad), bad)
end
local function scenario(id)
    local bundle = runtime(root, id)
    return bundle.gate_six, bundle.gate_six.new(), bundle
end
assert(not pcall(runtime, root, "absent"))
local r, s, b = scenario("gate-six")
assert(#b.operations == 7 and #b.catalog.units == 39 and #b.catalog.buildings == 30)
local roster = {}
for _, mission in ipairs(b.operations) do
    local theater = runtime(root, mission.id)
    local present = {}
    for _, node in ipairs(theater.site) do
        if node.type then
            roster[node.type] = true
            present[node.type] = true
        end
    end
    if mission.id == "gate-six" then
        for _, group in pairs(b.catalog.theater) do
            for _, unit in ipairs(group) do
                assert(present[unit], unit)
            end
        end
    end
end
for _, group in ipairs({ b.catalog.units, b.catalog.buildings }) do
    for _, item in ipairs(group) do
        assert(roster[item.id], item.id)
    end
end
assert(not r.command(s, "crew:absent") and not r.command(s, "crew:motor"))
assert(r.command(s, "work:stores@stores") and not r.command(s, "crew:stores"))
s.tick = 149
r.step(s)
assert(s.blue.alloy == 9 and not r.command(s, "crew:stores"))
assert(r.command(s, "mill") and s.reserved == 4 and r.command(s, "cancel") and s.blue.alloy == 9)
assert(not r.command(s, "cancel") and r.command(s, "staff"))
assert(r.command(s, "mill"))
r.step(s)
assert(s.blue.queues.fit.left == 120)
assert(r.command(s, "staff"))
r.step(s)
assert(s.reserved == 0 and r.command(s, "cancel") and s.blue.alloy == 5)
s.blue.motor = true
assert(r.command(s, "reserve") and not r.command(s, "reserve"))
s.blue.alloy = 4
assert(r.command(s, "armor") and r.command(s, "power"))
assert(r.command(s, "priority"))
r.step(s)
assert(s.blue.reserve == 150)
assert(r.command(s, "priority"))
r.step(s)
assert(s.blue.reserve == 149)
local function at(rule, state, tick)
    state.tick = tick - 1
    rule.step(state)
end
local function voyage(gear, landing, start, wait_for_crew, redirect)
    local g, v = scenario("tide-window")
    v.tick = start
    assert(not g.command(v, "invalid") and not g.command(v, "hold") and not g.command(v, "cancel"))
    assert(not g.command(v, "slip"))
    assert(g.command(v, "ramp") and g.command(v, "slip") and g.command(v, "cancel"))
    assert(v.blue.alloy == 8 and not v.destination)
    assert(g.command(v, gear) and not g.command(v, gear))
    assert(g.command(v, "quay") and g.command(v, "hold") and g.command(v, landing))
    assert(not g.command(v, landing))
    if not wait_for_crew then
        assert(g.command(v, "install") and not g.command(v, "install"))
    end
    assert(g.command(v, "report"))
    g.step(v)
    assert(not g.command(v, "cancel"))
    local loaded = start + (gear == "hoist" and 95 or 160) * 30
    at(g, v, loaded - 1)
    assert(v.cargo == 1)
    at(g, v, loaded)
    assert(v.stage == "embark")
    at(g, v, loaded + 750)
    assert(v.stage == "crossing")
    local arrived = loaded + 750 + 4050
    at(g, v, arrived)
    if v.stage == "berth" then
        assert(v.warned and (redirect or v.destination == "quay"))
        if redirect then
            assert(g.command(v, "slip"))
            at(g, v, v.tick + 24 * 30)
        else
            at(g, v, 480 * 30)
        end
    end
    assert(v.stage == "discharge" and not g.command(v, "quay"))
    at(g, v, v.tick + 900)
    assert(v.stage == "island")
    if wait_for_crew then
        g.step(v)
        assert(g.command(v, "install"))
    end
    at(g, v, v.tick + (v.landing == "slip" and 225 or 150) * 30)
    assert(v.result == "victory" and v.installed == 2)
    assert(not g.command(v, "report"))
    g.step(v)
    return v
end
assert(voyage("hoist", "quay", 0, false).tick == 435 * 30)
assert(voyage("ramp", "slip", 0, false).tick == 575 * 30)
assert(voyage("load", "quay", 0, false).blue.alloy == 8)
assert(voyage("ramp", "quay", 0, true, true).landing == "slip")
assert(voyage("hoist", "quay", 60 * 30, false).tick == 660 * 30)
r, s, b = scenario("tide-window")
assert(r.command(s, "crew:fitters@rival-ferry") and r.command(s, "hoist"))
r.step(s)
assert(s.reserved == 6 and s.report:match("Waiting"))
assert(r.command(s, "release:fitters") and not r.command(s, "release:fitters"))
r.step(s)
assert(s.reserved == 0)
assert(r.command(s, "crew:reed@rival-ferry"))
at(r, s, s.tick + 150)
local contact = s.contacts.reed
assert(contact.target == "rival-ferry")
at(r, s, 240 * 30)
assert(s.contacts.reed == contact and s.tick > contact.tick and s.frames)
assert(
    r.command(s, "crew:reed@relay")
        and r.command(s, "crew:reed@rival-ferry")
        and not r.command(s, "crew:reed@rival-ferry")
)
assert(not r.command(s, "crew:reed@unknown"))
r, s = scenario("tide-window")
at(r, s, r.deadline)
assert(s.result == "defeat")
local function finish(rule, state, order)
    assert(rule.command(state, order), order .. " / " .. state.report)
    local job = state.jobs[order]
    job.elapsed = job.length - 1
    rule.step(state)
    assert(state.done[order], order)
end
local itineraries = {
    ["compact-bank"] = "sound embark cross land repair tow patrol restore",
    westbound = "heater reserve survey track switch guard freight",
    ["white-mile"] = "shoes scout drain chart sample analyze assay-load",
    ["night-mail"] = "board beacon clear patrol mail-1 mail-2 escort mail-3",
    ["high-water"] = "collar module service lift supply dock transfer watch repair traffic",
}
for id, sequence in pairs(itineraries) do
    r, s = scenario(id)
    assert(not r.command(s, "absent") and not r.command(s, "cancel") and r.command(s, "report"))
    for job in sequence:gmatch("%S+") do
        finish(r, s, job)
    end
    assert(s.result == "victory")
    r.step(s)
    assert(not r.command(s, "report"))
    r, s = scenario(id)
    at(r, s, r.deadline)
    assert(s.result == "defeat")
end
r, s = scenario("night-mail")
assert(not r.command(s, "mail-1") and r.command(s, "board") and not r.command(s, "board"))
assert(r.command(s, "cancel") and s.reserved == 0 and s.blue.alloy == 8)
assert(r.command(s, "board"))
r.step(s)
assert(r.command(s, "cancel") and s.blue.alloy == 4)
s.blue.alloy = 0
assert(not r.command(s, "board"))
finish(r, s, "beacon")
finish(r, s, "clear")
assert(r.command(s, "mail-1") and not r.command(s, "crew:tern") and r.command(s, "patrol"))
r.step(s)
assert(s.jobs["mail-1"].elapsed == 1 and s.jobs.patrol.elapsed == 0)
assert(r.command(s, "crew:kite") and not r.command(s, "mail-2"))
r, s = scenario("compact-bank")
finish(r, s, "sound")
assert(r.command(s, "repair") and r.command(s, "tow"))
r.step(s)
assert(s.jobs.repair.elapsed == 0 and s.jobs.tow.elapsed == 1)
r, s = scenario("westbound")
finish(r, s, "survey")
assert(r.command(s, "track") and r.command(s, "power"))
r.step(s)
assert(s.jobs.track.elapsed == 0)
finish(r, s, "reserve")
r.step(s)
assert(s.priority == "track" and s.jobs.track.elapsed == 2)
assert(r.command(s, "cancel") and not s.reserve)
assert(r.command(s, "power"))
r, s = scenario("westbound")
assert(not r.command(s, "work:no-one@post"))
assert(r.command(s, "work:anvil@siding"))
at(r, s, s.service.anvil.finish)
assert(s.watchposts.siding == "anvil")
assert(r.command(s, "work:whistle@exchange"))
at(r, s, s.service.whistle.finish)
assert(s.contacts.whistle.target == "exchange")
assert(not r.command(s, "work:shunter@points"))
finish(r, s, "survey")
assert(r.command(s, "track") and r.command(s, "work:roundhouse@sleeper"))
at(r, s, s.service.roundhouse.finish)
assert(s.jobs.track.assisted)
r, s = scenario("high-water")
assert(not r.command(s, "work:dray@station") and r.command(s, "work:lark@station"))
r, s = scenario("tide-window")
assert(not r.command(s, "work:mole@relay"))
r, s = scenario("night-mail")
assert(r.command(s, "work:kite@flight"))
at(r, s, s.service.kite.finish)
assert(s.roster.kite)
r, s = scenario("gate-six")
assert(r.command(s, "mill") and r.command(s, "work:infirmary@fitters"))
at(r, s, s.service.infirmary.finish)
assert(s.blue.queues.fit.assisted)
s.supply_stock = 0
assert(r.command(s, "work:stores@stores"))
at(r, s, s.service.stores.finish)
assert(s.report:find("reserve stores empty"))
-- Load optional packs through the shipping reader, substituting only the test manifest and entry.
local original_open, original_load = io.open, loadfile
local manifest = '{"version":1,"packs":["mods/long-watch"]}'
local definition, entry
io.open = function(name, mode)
    local value = (name == root .. "/content.json" and manifest)
        or (name:match("/long%-watch/pack%.json$") and definition)
    if value then
        return {
            read = function()
                return value
            end,
            close = function() end,
        }
    end
    return original_open(name, mode)
end
loadfile = function(name, ...)
    if entry and name:match("/long%-watch/init%.lua$") then
        return entry
    end
    return original_load(name, ...)
end
r, s, b = scenario("gate-six")
assert(r.hold_ticks == 2700 and b.site[9].asset:match("oxide.mesh"))
r, s = scenario("tide-window")
assert(r.deadline == 900 * 30)
at(r, s, 720 * 30)
assert(s.result == "active")
at(r, s, 900 * 30)
assert(s.result == "defeat")
entry = function(api)
    api.unit({ id = "runner", name = "Courier", role = "freight", family = "truck", w = 3, h = 4, d = 8, asset = "oxide.mesh" })
    api.mission({
        id = "pack-mission",
        name = "New route",
        sector = "westbound",
        build = function(ctx)
            ctx.add_entity("courier", "runner", { 0, 0, 0 })
            return assert(original_load(root .. "/scripts/operations.lua"))()("westbound", ctx)
        end,
    })
    api.job("lights", "Light the signals", function(state)
        state.lit = true
        return true
    end)
    api.on("order", function(state, data)
        state.last_order = data
    end)
end
r, s, b = scenario("pack-mission")
assert(r.command(s, "lights") and s.lit and s.last_order == "lights")
entry = function(api)
    api.job("report", "Duplicate", function() end)
end
assert(not pcall(runtime, root, "westbound"))
entry = function(api)
    api.override("westbound.unknown", 1)
end
assert(not pcall(runtime, root, "westbound"))
entry = nil
definition = '{"api":77}'
assert(not pcall(runtime, root, "gate-six"))
manifest = '{"version":99}'
assert(not pcall(runtime, root, "gate-six"))
manifest = '{"version":1,"packs":["../x"]}'
assert(not pcall(runtime, root, "gate-six"))
io.open, loadfile = original_open, original_load
