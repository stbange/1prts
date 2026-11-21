local M = {}
local homes = {
    weir = "WEIR ASSEMBLY",
    sounding = "SOUNDING COMPACT",
    terrace = "TERRACE RAILWAY",
    salt = "SALT SURVEY",
    lantern = "LANTERN PILOTS",
    meridian = "MERIDIAN DOCK TRUST",
}
local plans = {
    ["gate-six"] = {
        "weir",
        "terrain",
        "porter sump tag dray plumb span tally reed hinge points barrow gimbal",
        "post depot pump stores infirmary signals slipway quarters siding mooring gallery shed",
    },
    ["compact-bank"] = {
        "sounding",
        "estuary",
        "wake hinge reed channel-section crossbill fitters mole span",
        "shed mooring slipway watch motor signals post",
    },
    ["tide-window"] = {
        "weir",
        "estuary",
        "kestrel mole fitters tag porter reed",
        "depot relay mooring customs slipway",
    },
    westbound = {
        "terrace",
        "railbed",
        "shunter points anvil sleeper whistle dray tally fitters gate-section",
        "exchange roundhouse siding stores gallery survey post quarters dynamo",
    },
    ["white-mile"] = {
        "salt",
        "saltpan",
        "barrow chain-crew glint prism shoe sump mole plumb tag",
        "assay pump infirmary relay stores signals mill",
    },
    ["night-mail"] = {
        "lantern",
        "airfield",
        "kite tern rook crossbill gimbal gantry ground-watch porter",
        "flight tower beacon customs quarters depot",
    },
    ["high-water"] = {
        "meridian",
        "orbit",
        "lark cleat spindle rivet loom collar-watch dray",
        "station spaceport cradle lock mooring beacon",
    },
}
local fixed = {
    ["tide-window"] = {
        kestrel = { 0, 0, 0 },
        mole = { 21, 0, 55 },
        depot = { -33, 0, 73 },
        relay = { -30, 0, -284 },
        mooring = { 36, 0, -264 },
        customs = { 45, 0, 80 },
    },
    westbound = {
        shunter = { 0, 0, -3 },
        exchange = { 3, 0, -165 },
        roundhouse = { -35, 0, -25 },
        siding = {
            29,
            0,
            -80,
        },
    },
    ["white-mile"] = { barrow = { 0, 0, -3 }, assay = { -25, 0, -136 }, pump = { 30, 0, -63 } },
    ["night-mail"] = {
        kite = { 0, 0, -10 },
        flight = { -43, 0, -28 },
        tower = { 46, 0, -55 },
        beacon = {
            17,
            0,
            -165,
        },
    },
    ["high-water"] = {
        lark = { 0, 0, 0 },
        spaceport = { -45, 0, 65 },
        station = { 0, 84, -668 },
        cradle = {
            64,
            0,
            23,
        },
    },
    ["compact-bank"] = {
        wake = { 0, 0, -3 },
        shed = { -25, 0, -27 },
        hinge = { -27, 0, 5 },
        mooring = {
            28,
            0,
            -105,
        },
    },
}
local spawn = {
    ["tide-window"] = { 0, 0, 50, 0 },
    westbound = { 0, 0, 13, 0 },
    ["white-mile"] = { 0, 0, 13, 0 },
    ["night-mail"] = { 0, 0, 14, 0 },
    ["high-water"] = { 1, 0, 45, 0 },
    ["compact-bank"] = { 0, 0, 11, 0 },
}
local facilities = { hall = true, plant = true, works = true, tower = true, hangar = true, gate = true }
local people_models =
    { fitters = "worker", ["gate-section"] = "section", ["channel-section"] = "enemysection" }
function M.fleet(read)
    local fleet = {}
    for line in read("assets/fleet.txt"):gmatch("[^\r\n]+") do
        if line:sub(1, 1) ~= "#" then
            local id, f, w, h, d, owner, role =
                line:match("^(%S+)%s+(%S+)%s+([%d.]+)%s+([%d.]+)%s+([%d.]+)%s+(%S+)%s+(%S+)")
            assert(id, "invalid fleet definition: " .. line)
            fleet[id] =
                { family = f, w = tonumber(w), h = tonumber(h), d = tonumber(d), owner = owner, role = role }
        end
    end
    for line in read("assets/rooms.txt"):gmatch("[^\r\n]+") do
        if line:sub(1, 1) ~= "#" then
            local id, name, gear, x, z = line:match("^(%S+) (%S+) (%S+) (%S+) (%S+)$")
            local def = assert(fleet[id], "room references unknown asset")
            def.room = { name = name, gear = gear, x = assert(tonumber(x)) * def.w, z = assert(tonumber(z))
                * def.d }
        end
    end
    return fleet
end
function M.make(id, catalog, fleet, base_site, sectors)
    local custom = sectors and sectors[id]
    local plan = custom and { custom.owner, custom.terrain, custom.units, custom.buildings }
        or assert(plans[id], "unknown sector " .. id)
    local gate = id == "gate-six"
    local scene = gate and base_site or { { plan[2], 0, 0, 0, 0 } }
    local names = {}
    for _, group in ipairs({ catalog.units, catalog.buildings }) do
        for _, v in ipairs(group) do
            names[v.id] = v.name
        end
    end
    local index = {}
    if gate then
        for i, key in ipairs({
            "terrain",
            "gate",
            "survey",
            "dynamo",
            "mill",
            "watch",
            "motor",
            "mole",
            "lintel",
            "wake",
            "gate-section",
            "channel-section",
            "fitters",
        }) do
            local row, def = scene[i], fleet[key]
            if def then
                row.type = key
                row.role, row.name = def.role, names[key]
            end
            if def and key ~= "wake" and key ~= "channel-section" then
                row.key, row.role, row.name = key, def.role, names[key]
                index[key] = i
            end
        end
        for _, slot in ipairs({ 10, 24, 25, 26, 27, 28, 29 }) do
            local parent = scene[slot]
            local operator = { "worker", 0.9, slot == 10 and 0.84 or 0.08, -1.3, 0 }
            operator.parent = slot
            operator.key = "rival-watch-" .. slot
            operator.name = "Compact watch"
            operator.family = "operator"
            scene[#scene + 1] = operator
        end
        for _, berth in ipairs({ { 19, 8 }, { 20, 9 } }) do
            local operator, hull = scene[berth[1]], scene[berth[2]]
            operator[2], operator[4] = operator[2] - hull[2], operator[4] - hull[4]
            operator[6] = nil
            operator.parent = berth[2]
            operator.family = "operator"
            operator.key = hull.key .. "-driver"
            operator.name = hull.name .. " / driver"
        end
        for _, pair in ipairs({ { 16, 3 }, { 17, 4 }, { 18, 2 }, { 21, 5 }, { 22, 6 }, { 23, 7 } }) do
            local worker, room = scene[pair[1]], scene[pair[2]]
            local dx, dz = worker[2] - room[2], worker[4] - room[4]
            local co, si = math.cos(room[5]), math.sin(room[5])
            worker[2], worker[4], worker[6] = dx * co + dz * si, -dx * si + dz * co, nil
            worker.parent, worker.family = pair[2], "operator"
            worker.key = (room.key or room[1]) .. "-steward"
        end
        for i, key in ipairs({
            "rival-survey",
            "rival-dynamo",
            "rival-mill",
            "rival-watch",
            "rival-motor",
            "rival-mole",
        }) do
            local row = scene[i + 23]
            row.key = key
            row.type = row[1]
            row.role = fleet[row.type].role
        end
        scene[12].key = "rival-section"
        scene[12].type = "channel-section"
        scene[10].key = "rival-wake"
        scene[10].name = "Compact Wake"
        index["rival-wake"] = 10
    end
    local equipped = false
    local function furnish(i)
        local host = scene[i]
        local def = fleet[host.type]
        local room = def and def.room
        if not room then
            return
        end
        local floor = facilities[def.family] and 0.08 or 0.84
        local key = host.key .. "-work"
        local width, depth = math.min(2.6, def.w * 0.29), math.min(1.9, def.w * 0.23)
        scene[#scene + 1] = {
            "work-" .. (def.room_model or host.type),
            room.x,
            floor,
            room.z,
            0,
            key = key,
            type = "equipment",
            name = room.name:gsub("-", " "),
            owner = host.owner,
            parent = i,
            family = "fixture",
            size = { width, 1.8, depth },
        }
        index[key] = #scene
    end
    local function entity(key, unit, pos)
        assert(type(key) == "string" and key:match("^[%w-]+$") and #key <= 50, "invalid instance ID")
        local def = assert(fleet[unit], unit)
        assert(not index[key], "duplicate entity " .. key)
        local model = def.model or people_models[unit] or unit
        local row = { model, pos[1], pos[2], pos[3], pos[4] or 0 }
        row.owner = key:match("^rival%-") and "rival" or plan[1]
        row.type = unit
        row.key, row.name, row.family, row.role = key, names[unit], def.family, def.role
        row.name = row.name or key
        if key ~= unit then
            row.name = row.name .. " / " .. key
        end
        row.size = { def.w, def.h, def.d }
        row.asset = def.asset
        row.hull = not facilities[def.family] and def.family ~= "people"
        row.station = def.family ~= "people"
        scene[#scene + 1] = row
        index[key] = #scene
        local host = #scene
        if row.station then
            for n, z in ipairs({ -0.29, 0.26 }) do
                local worker = { "worker", def.w * 0.23, row.hull and 0.84 or 0.08, def.d * z, 0 }
                worker.owner, worker.type = row.owner, "operator"
                worker.parent = #scene - (n - 1)
                worker.key = key .. "-" .. n
                worker.family = "operator"
                worker.name = row.name .. (n == 1 and " / pilot" or " / watch")
                scene[#scene + 1] = worker
                index[worker.key] = #scene
            end
        end
        if equipped then
            furnish(host)
        end
    end
    local positions = custom and custom.positions or fixed[id] or {}
    for group = 3, 4 do
        local j = 0
        for unit in plan[group]:gmatch("%S+") do
            j = j + 1
            local x = (group == 3 and 1 or -1)
                * (gate and (83 + ((j - 1) % 2) * 34) or (75 + ((j - 1) % 2) * 37))
            local z = 60 - math.floor((j - 1) / 2) * 37
            entity(unit, unit, positions[unit] or { x, 0, z })
        end
    end
    if id == "tide-window" then
        entity("rival-ferry", "kestrel", { 78, 0, -156 })
        scene[#scene + 1] = { "fore-ramp", 0, -6, -48, 0, parent = index.kestrel, key = "fore-ramp" }
        index["fore-ramp"] = #scene
        for n = 1, 2 do
            local key = "core-" .. n
            scene[#scene + 1] = { "core", 13 + 2 * n, 0, 54, 0, key = key, name = "Power core " .. n }
            index[key] = #scene
            key = "bow-spray-" .. n
            scene[#scene + 1] = { "spray", 0, -4, 0, 0, key = key, family = "sprite" }
            index[key] = #scene
        end
    end
    if id == "high-water" then
        for shift = 0, 4 do
            scene[#scene + 1] = {
                "worker",
                7,
                0.84,
                800 * (0.42 - shift * 0.21) - 5,
                0,
                parent = index.station,
                key = "collar-operator-" .. shift,
                name = "Crown watch " .. (shift + 1),
                family = "operator",
            }
        end
    end
    if gate then
        entity("rival-recovery", "hinge", { -63, 0, -81 })
        for _, marker in ipairs({
            { "west-pad", -48, 9 },
            { "east-pad", 53, 15 },
            { "bank-pad", -58, -20 },
            { "common-lot", -17, -32 },
            { "home-lot", -16, -29 },
            { "north-lot", 48, -76 },
        }) do
            scene[#scene + 1] = {
                marker[1]:find("pad") and "survey-pad" or "plate-lot",
                marker[2],
                0,
                marker[3],
                0,
                key = marker[1],
                name = marker[1]:gsub("-", " "),
                family = "marker",
                type = "marker",
                owner = "neutral",
            }
        end
    end
    for i, row in ipairs(scene) do
        row.key = row.key or ("site-" .. i)
        assert(
            #row.key < 64 and (not index[row.key] or index[row.key] == i),
            "invalid instance ID " .. row.key
        )
        row.type = row.type or row[1]
        local rival = (row[6] or ""):match("^red")
            or (row[6] or ""):match("^enemy")
            or row.key:match("^rival%-")
        row.owner = row.owner or (row.parent and scene[row.parent].owner) or (rival and "rival" or plan[1])
        index[row.key] = i
    end
    for i = 1, #scene do
        furnish(i)
    end
    equipped = true
    local function fitting(model, key, host, x, y, z)
        local parent = assert(index[host])
        scene[#scene + 1] = {
            model,
            x,
            y,
            z,
            0,
            key = key,
            type = model,
            name = key,
            owner = scene[parent].owner,
            parent = parent,
            family = "fitting",
        }
        index[key] = #scene
    end
    local parcels = {
        westbound = { "roundhouse", "winter-freight" },
        ["white-mile"] = { "pump", "salt-sample" },
        ["night-mail"] = { "flight", "bag-one", "bag-two", "bag-three" },
        ["high-water"] = { "cradle", "pressure-module" },
    }
    local manifest = parcels[id]
    if manifest then
        for n = 2, #manifest do
            fitting("core", manifest[n], manifest[1], n * 1.4, 0.08, 2)
        end
    end
    if index.depot then
        fitting("depot-gantry", "depot-gallery", "depot", 0, 0, 0)
    end
    if index.station then
        fitting("lift-guide", "lift-guide", "station", -4, 0.84, 326)
        fitting("lift-car", "traffic-lift", "station", -4, 0.84, 326)
        for level = 0, 1 do
            for side = 1, 2 do
                fitting(
                    "lift-door",
                    "lift-door-" .. level .. "-" .. side,
                    "station",
                    -4 + (side * 2 - 3) * 1.9,
                    0.84 + 6 * level,
                    327.88
                )
            end
        end
    end
    local function ground(key)
        local x, z = key:match("^g%-(%d+)%-(%d+)$")
        x, z = tonumber(x), tonumber(z)
        if not x or x > 840 or z > 1320 then
            return false
        end
        if index[key] then
            return true
        end
        scene[#scene + 1] = {
            "survey-pad",
            x - 420,
            0,
            z - 1100,
            0,
            key = key,
            type = "marker",
            family = "marker",
            name = "Ground " .. (x - 420) .. ", " .. (z - 1100),
            owner = "neutral",
        }
        index[key] = #scene
        return true
    end
    local function register(s, first)
        for i = first, #scene do
            local row = scene[i]
            local parent = row.parent and scene[row.parent]
            s.instances[row.key] = {
                type = row.type,
                owner = row.owner,
                parent = parent and parent.key,
                crew = {},
                status = "ready",
            }
            if parent and row.family == "operator" then
                table.insert(s.instances[parent.key].crew, row.key)
            end
        end
    end
    local function truncate(count)
        for i = #scene, count + 1, -1 do
            index[scene[i].key] = nil
            scene[i] = nil
        end
    end
    return {
        site = scene,
        ground = ground,
        register = register,
        truncate = truncate,
        fleet = fleet,
        owner = plan[1],
        sector = id,
        index = index,
        add_entity = entity,
        terrain = plan[2],
        routes = custom and custom.routes,
        faction = homes[plan[1]] or plan[1],
        spawn = custom and custom.spawn or spawn[id] or { 3, 0, 22, -0.052 },
    }
end
return M
