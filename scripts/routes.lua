-- Metre-scale route junctions. Each lane has its own entrance and exit.
-- Ground units join the road on their bank, rail stock stays on the rail bed.
local routes = {
    terrain = {
        road = { { 6, 24 }, { 6, -24 }, { 3, -54 }, { 3, -74 }, { 6, -89 } },
        detour = { { -12, -24 }, { -12, -54 } },
        span = { 2, 3 },
        laybys = { { 6, -24, 15, -25 }, { 3, -54, 15, -54 } },
        width = 6,
    },
    estuary = {
        road = { { 4, 69 }, { 30, 20 }, { 78, -35 }, { 78, -168 }, { 30, -218 }, { 4, -263 } },
        water = { { 0, 24 }, { 0, -40 }, { 12, -146 }, { 0, -225 } },
        shallows = { { 22, 21 }, { 36, -42 }, { 41, -148 }, { 55, -174 } },
        width = 8,
        laybys = { { 30, 20, 42, 20 }, { 30, -218, 43, -216 } },
    },
    railbed = {
        road = { { 20, 45 }, { 20, -50 }, { 20, -110 }, { 20, -165 } },
        rail = { { 0, 50 }, { 0, -30 }, { 0, -82 }, { 0, -128 }, { 0, -166 } },
        width = 6,
    },
    saltpan = {
        road = { { 10, 50 }, { 16, -28 }, { 32, -79 }, { 22, -112 }, { 5, -141 } },
        detour = { { 0, -28 }, { 0, -99 } },
        span = { 2, 4 },
        width = 6,
    },
    airfield = {
        road = { { 8, 55 }, { 8, -40 }, { 8, -90 }, { 8, -161 } },
        air = {
            { 0, 38 },
            { -8, -45 },
            { 8, -120 },
            { 16, -161 },
        },
        width = 8,
    },
    orbit = {
        road = { { 20, 65 }, { 20, 12 }, { 20, -40 } },
        dock = { { 0, 65 }, { 0, -85 }, { 0, -175 }, { 0, -268 }, { 8, -405 }, { 8, -658 } },
        width = 12,
    },
}
local M = { sectors = routes }
function M.bypass(ctx, node)
    if not node[4] or not node[4]:match("^road%-") then
        return
    end
    local map = ctx.routes or routes[ctx.terrain]
    for _, side in ipairs(map.laybys or {}) do
        if node[1] == side[1] and node[3] == side[2] then
            return { side[3], 0, side[4], "layby-" .. side[3] .. "-" .. side[4] }
        end
    end
end
local function dist(a, b)
    return math.sqrt((a[1] - b[1]) ^ 2 + (a[2] - b[2]) ^ 2 + (a[3] - b[3]) ^ 2)
end
function M.position(ctx, s, key)
    local i = ctx.index[key]
    if not i then
        return
    end
    local row = ctx.site[i]
    local f = s.frames[i]
    local x, y, z, yaw, parent
    if f then
        x, y, z, yaw, parent = f[1], f[2], f[3], f[4] or 0, f[5] or 0
    else
        x, y, z, yaw, parent = row[2], row[3], row[4], row[5], row.parent or 0
        local cond = row[6]
        local b, r = s.blue, s.red
        if cond == "haul" or cond == "redhaul" then
            local red = cond == "redhaul"
            local t = 1 - math.abs((red and r.haul or b.haul) / 150 - 1)
            x, z = x + (red and 12 or -8) * t, z + (red and 7 or -31) * t
        elseif cond == "armor" then
            local run, bay = b.drive / 210, 1 - b.rollout / 120
            x, z = x - run * 7 - bay * 29, z + bay * 22 - run * 58
        elseif cond == "enemyarmor" then
            x, z = x - r.drive / 210 * 13, z + r.drive / 210 * 34
        elseif cond == "section" then
            x, z = x - b.walk / 300 * 17, z - b.walk / 300 * 47
        elseif cond == "enemysection" then
            x, z = x - r.walk / 300 * 12, z + r.walk / 300 * 21
        end
    end
    if parent > 0 then
        local base = M.position(ctx, s, ctx.site[parent].key)
        local c, t = math.cos(base[4]), math.sin(base[4])
        x, y, z, yaw = base[1] + c * x - t * z, base[2] + y, base[3] + t * x + c * z, base[4] + yaw
    end
    return { x, y, z, yaw }
end
function M.family(ctx, row)
    return row.family or (ctx.fleet[row.type] or {}).family
end
function M.plan(ctx, s, key, target, offset, reported)
    local row, other = ctx.site[ctx.index[key]], ctx.site[ctx.index[target]]
    if not row or not other then
        return nil, "Unknown destination."
    end
    local start, goal = M.position(ctx, s, key), reported or M.position(ctx, s, target)
    local f = M.family(ctx, row)
    local kind = ({
        rail = "rail",
        aircraft = "air",
        tiltrotor = "air",
        tender = "dock",
        tug = "dock",
        ferry = "water",
        launch = "water",
        pontoon = "water",
    })[f] or "road"
    local map = ctx.routes or routes[ctx.terrain]
    if not map then
        return nil, "No routes charted here."
    end
    if
        kind == "rail"
        and not (
            M.family(ctx, other) == "rail"
            or target == "siding"
            or target == "roundhouse"
            or target == "exchange"
        )
    then
        return nil, "No rail siding at destination."
    end
    if kind == "road" and math.abs(goal[2] - start[2]) > 12 then
        return nil, "A docking transfer is required."
    end
    if
        kind == "road"
        and ctx.terrain == "estuary"
        and (start[3] + 100) * (goal[3] + 100) < 0
        and not s.crossing
    then
        return nil, "Water cuts the road. Request a crossing."
    end
    if kind == "water" and ctx.terrain ~= "estuary" and f ~= "launch" then
        return nil, "No navigable channel."
    end
    local stations = kind == "water" and f ~= "ferry" and map.shallows or map[kind] or map.road
    local lane = {}
    local short = kind == "road"
        and map.span
        and (s.crossing or ctx.terrain == "saltpan" and s.done and s.done.shoes)
    for i, junction in ipairs(stations) do
        if not short or i <= map.span[1] or i >= map.span[2] then
            lane[#lane + 1] = {
                junction[1],
                kind == "air" and 24
                    or kind == "dock" and math.max(0, math.min(84, -junction[2] * 0.34))
                    or 0,
                junction[2],
            }
        end
        if short and i == map.span[1] then
            for _, shortcut in ipairs(map.detour) do
                lane[#lane + 1] = { shortcut[1], 0, shortcut[2] }
            end
        end
    end
    local function nearest(p)
        local best = 1
        for i = 2, #lane do
            if dist(p, lane[i]) < dist(p, lane[best]) then
                best = i
            end
        end
        return best
    end
    local from, to = nearest(start), nearest(goal)
    local path = {}
    for i = from, to, from <= to and 1 or -1 do
        local node = lane[i]
        path[#path + 1] = { node[1], node[2], node[3], kind .. "-" .. node[1] .. "-" .. node[3] }
    end
    local extent = (other.size or { ctx.fleet[other.type] and ctx.fleet[other.type].w or 2 })[1] * 0.5
    if other.type == "station" and kind == "dock" then
        goal[3] = goal[3] + (other.size[3] * 0.5)
    end
    local slot = offset or 0
    path[#path + 1] = { goal[1] + extent + 3 + (slot % 3) * 7, goal[2], goal[3] + math.floor(slot / 3) * 15 }
    return path
end
return M
