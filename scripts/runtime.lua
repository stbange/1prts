-- The same distribution manifest is consumed by the native host and the browser.
local root, requested = ...
local function path(base, name)
    assert(
        type(name) == "string"
            and name:match("^[%w_./%-]+$")
            and not name:find("..", 1, true)
            and name:sub(1, 1) ~= "/"
            and not name:find("//", 1, true),
        "invalid content path " .. tostring(name)
    )
    return base .. "/" .. name
end
local function read(name)
    local f = assert(io.open(path(root, name), "rb"))
    local data = f:read("a")
    f:close()
    return data
end
local function script(name)
    return assert(loadfile(path(root, "scripts/" .. name .. ".lua")))()
end
local decode = script("manifest")
local manifest = decode(read("content.json"))
assert(manifest.version == 1, "unsupported base content version " .. tostring(manifest.version))
local catalog, scenes = script("catalog"), script("scenes")
local fleet = scenes.fleet(read)
local missions, overrides, listeners, extra_jobs = {}, {}, {}, {}
local operations = {}
for _, m in ipairs(catalog.missions) do
    missions[m.id] = m
    operations[#operations + 1] = m
end
local compact = {
    id = "compact-bank",
    name = "Gate Six / Compact",
    role = "Sounding Compact harbor command.",
    owner = "sounding",
}
missions[compact.id] = compact
operations[#operations + 1] = compact
local function emit(event, s, data)
    for _, fn in ipairs(listeners[event] or {}) do
        fn(s, data)
    end
end
for _, directory in ipairs(manifest.packs or {}) do
    local success, err = pcall(function()
        local pack = decode(read(directory .. "/pack.json"))
        assert(pack.api == 1, "unsupported API " .. tostring(pack.api))
        local declared = {}
        for _, key in ipairs(pack.overrides or {}) do
            declared[key] = true
        end
        for _, file in ipairs(pack.files or {}) do
            local f = assert(io.open(path(root, directory .. "/" .. file), "rb"))
            f:close()
        end
        local api = { version = 1 }
        function api.override(key, value)
            assert(declared[key], "undeclared override " .. key)
            if key:match("%.mesh$") or key:match("%.wav$") then
                value = directory .. "/" .. value
                local f = assert(io.open(path(root, value), "rb"))
                f:close()
            else
                assert(
                    key == "gate-six.hold_ticks" or key == "tide-window.deadline",
                    "unknown override " .. key
                )
                assert(math.type(value) == "integer" and value > 0, "invalid duration")
            end
            overrides[key] = value
        end
        function api.mission(v)
            assert(type(v.id) == "string" and v.name and type(v.build) == "function", "invalid pack mission")
            assert(not missions[v.id], "duplicate mission " .. v.id)
            v.role = v.role or v.name
            v.owner = v.owner or "campaign"
            missions[v.id] = v
            operations[#operations + 1] = v
            catalog.missions[#catalog.missions + 1] = v
        end
        function api.unit(v)
            assert(v.id and v.name and v.role and v.family and v.w and v.h and v.d, "invalid pack unit")
            assert(not fleet[v.id], "duplicate unit " .. v.id)
            if v.asset then
                v.asset = directory .. "/" .. v.asset
                local file = assert(io.open(path(root, v.asset), "rb"))
                file:close()
            end
            fleet[v.id] = v
            catalog.units[#catalog.units + 1] = v
        end
        function api.job(id, label, run)
            assert(not extra_jobs[id] and type(run) == "function", "invalid or duplicate job " .. id)
            extra_jobs[id] = { label = label, run = run }
        end
        function api.on(event, fn)
            assert(type(fn) == "function", "invalid event handler")
            listeners[event] = listeners[event] or {}
            table.insert(listeners[event], fn)
        end
        assert(loadfile(path(root, directory .. "/" .. pack.entry)))(api)
    end)
    assert(success, directory .. ": " .. tostring(err))
end
local id = requested or "gate-six"
local mission = assert(missions[id], "unknown mission " .. id)
local sector = mission.sector or id
local ctx = scenes.make(sector, catalog, fleet, script("site"))
ctx.emit = emit
local rules
if mission.build then
    rules = mission.build(ctx)
elseif id == "gate-six" then
    rules = script("gate_six")
elseif id == "tide-window" then
    rules = script("tide_window")(ctx)
else
    rules = script("operations")(id, ctx)
end
if id == "gate-six" and overrides["gate-six.hold_ticks"] then
    rules.hold_ticks = overrides["gate-six.hold_ticks"]
    for _, m in ipairs(catalog.missions) do
        if m.id == id then
            m.role = ("Hold the floodgate for %g seconds."):format(rules.hold_ticks / 30)
        end
    end
end
if id == "tide-window" and overrides["tide-window.deadline"] then
    rules.deadline = overrides["tide-window.deadline"]
    rules.hold_ticks = rules.deadline
end
if id == "gate-six" then
    for _, action in ipairs({
        { "cancel", "Cancel latest bay order" },
        { "staff", "Change production shift" },
        { "reserve", "Charge gallery / 2 alloy" },
        { "priority", "Assign reserve priority" },
    }) do
        rules.actions[#rules.actions + 1] = action
    end
end
local assigned = id == "gate-six"
        and {
            harvest = "mole",
            mill = "fitters",
            watch = "fitters",
            motor = "fitters",
            section = "watch",
            armor = "motor",
            advance = "gate-section",
            guard = "lintel",
            power = "dynamo",
            staff = "fitters",
            reserve = "gallery",
            priority = "gallery",
        }
    or id == "tide-window" and {
        hoist = "fitters",
        ramp = "fitters",
        load = "mole",
        quay = "kestrel",
        slip = "kestrel",
        install = "relay",
        hold = "kestrel",
        report = "kestrel",
        cancel = "fitters",
    }
    or {}
for _, action in ipairs(rules.actions) do
    action[3] = action[3] or assigned[action[1]]
end
local job_ids = {}
for key in pairs(extra_jobs) do
    job_ids[#job_ids + 1] = key
end
table.sort(job_ids)
for _, key in ipairs(job_ids) do
    for _, action in ipairs(rules.actions) do
        assert(action[1] ~= key, "duplicate order " .. key)
    end
    rules.actions[#rules.actions + 1] = { key, extra_jobs[key].label }
end
for _, v in ipairs(ctx.site) do
    v.asset = overrides[v[1] .. ".mesh"] or v.asset
end
local fieldwork = script("fieldwork")(ctx)
local original_new, original_command, original_step = rules.new, rules.command, rules.step
function rules.new()
    local s = original_new()
    s.frames = s.frames or {}
    s.service = {}
    s.roster = {}
    s.reports = {}
    s.report_at = 0
    s.supply_stock = 3
    s.contacts = {}
    s.log = {}
    s.watchposts = {}
    if id == "gate-six" and rules.hold_ticks ~= 1800 then
        s.report = mission.role
    end
    if s.report then
        s.log[1] = { tick = 0, text = s.report }
    end
    return s
end
local function record(s, text)
    s.log[#s.log + 1] = { tick = s.tick, text = text }
    if #s.log > 12 then
        table.remove(s.log, 1)
    end
end
function rules.command(s, order)
    if s.result ~= "active" then
        return false
    end
    local worker, place = order:match("^work:([%w-]+)@([%w-]+)$")
    if worker then
        if not fieldwork.start(s, worker, place) then
            return false
        end
        s.report = ctx.site[ctx.index[worker]].name .. " / " .. s.service[worker].role .. " at " .. place
        s.report_at = s.tick
        s.reports[worker] = s.tick
        record(s, s.report)
        return true
    end
    local release = order:match("^release:(.+)$")
    if release then
        if not s.service[release] then
            return false
        end
        s.service[release] = nil
        s.report = ctx.site[ctx.index[release]].name .. " / duty released."
        s.report_at = s.tick
        record(s, s.report)
        return true
    end
    local key, target = order:match("^crew:([%w-]+)@([%w-]+)$")
    key = key or order:match("^crew:([%w-]+)$")
    if key then
        local idx = ctx.index[key]
        if not idx or target and not ctx.index[target] then
            return false
        end
        local node = ctx.site[idx]
        if not node.role or (s.roster[key] or s.service[key]) and not target then
            return false
        end
        if s.jobs then
            for _, j in pairs(s.jobs) do
                if j.actor == key then
                    return false
                end
            end
        end
        if node[6] and not s.blue[node[6]] and node[6] ~= "haul" and node[6] ~= "fitters" then
            return false
        end
        if s.service[key] and s.service[key].target == target then
            return false
        end
        s.service[key] = { finish = s.tick + 150, role = node.role, target = target or key }
        s.report = node.name .. " / " .. node.role .. " shift at " .. (target or key) .. "."
        s.report_at = s.tick
        s.reports[key] = s.tick
        record(s, s.report)
        return true
    end
    local accepted
    if extra_jobs[order] then
        accepted = extra_jobs[order].run(s, ctx)
    else
        accepted = original_command(s, order)
    end
    s.report_at = s.tick
    record(s, s.report)
    if id == "gate-six" then
        s.reserved = s.blue.reserved
    end
    if accepted then
        emit("order", s, order)
    end
    return accepted
end
function rules.step(s)
    if s.result ~= "active" then
        return
    end
    local previous = s.report
    if id == "gate-six" and s.service.fitters then
        s.blue.away = true
    else
        s.blue.away = false
    end
    original_step(s)
    if id == "gate-six" then
        s.reserved = s.blue.reserved
    end
    if next(s.service) then
        local keys = {}
        for key in pairs(s.service) do
            keys[#keys + 1] = key
        end
        table.sort(keys)
        for _, key in ipairs(keys) do
            local job = s.service[key]
            fieldwork.advance(s, key, job)
            if s.tick >= job.finish then
                s.service[key] = nil
                s.roster[key] = (s.roster[key] or 0) + 1
                local result = fieldwork.finish(s, key, job)
                s.reports[key] = s.tick
                s.report = ctx.site[ctx.index[key]].name .. " / " .. job.target .. " / " .. result .. "."
                emit("crew", s, key)
            end
        end
    end
    if previous ~= s.report then
        s.report_at = s.tick
        record(s, s.report)
    end
    if s.result ~= "active" then
        emit("result", s, s.result)
    end
end
return {
    catalog = catalog,
    site = ctx.site,
    gate_six = rules,
    operations = operations,
    terrain = ctx.terrain,
    media = overrides,
    faction = ctx.faction,
    title = mission.name,
    spawn = ctx.spawn,
}
