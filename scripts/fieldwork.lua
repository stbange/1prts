-- A crew can take a local assignment alongside the sector's scheduled jobs.
local freight = { freight = true, salvage = true, supplies = true, refine = true }
local observers = { survey = true, lookout = true, relay = true, guide = true, schedule = true }
local watch = { guard = true, support = true, occupy = true, patrol = true }
return function(ctx)
    local function position(s, key)
        if ctx.position then
            local p = ctx.position(s, key)
            return p[1], p[2], p[3]
        end
        local i = ctx.index[key]
        local row = ctx.site[i]
        local frame = s.frames[i]
        if frame then
            return frame[1], frame[2], frame[3]
        end
        if key == "rival-wake" then
            return row[2] - s.red.drive / 210 * 13, 0, row[4] + s.red.drive / 210 * 34
        end
        return row[2], row[3], row[4]
    end
    local function start(s, key, target)
        local i, j = ctx.index[key], ctx.index[target]
        if not i or not j or s.service[key] then
            return false
        end
        local node = ctx.site[i]
        if
            not node.role
            or node[6] and node[6] ~= "fitters" and node[6] ~= "haul" and not s.blue[node[6]]
        then
            return false
        end
        for _, job in pairs(s.jobs or {}) do
            if job.actor == key then
                return false
            end
        end
        local x, y, z = position(s, key)
        local tx, ty, tz = position(s, target)
        local dest = ctx.site[j]
        local aircraft = node.family == "aircraft" or node.family == "tiltrotor"
        local spacecraft = node.family == "tender" or node.family == "tug"
        if not (aircraft or spacecraft or observers[node.role]) and math.abs(ty - y) > 12 then
            return false
        end
        if
            ctx.terrain == "estuary"
            and z * tz < 0
            and math.abs(z - tz) > 95
            and node.hull
            and not (
                aircraft
                or node.family == "ferry"
                or node.family == "launch"
                or node.family == "pontoon"
            )
        then
            return false
        end
        if node.family == "rail" and dest.family ~= "rail" and not dest.station then
            return false
        end
        local moves = node.hull and not node[6] and i ~= j and not (s.frames[i] and s.frames[i][5] > 0)
        local distance = math.sqrt((tx - x) ^ 2 + (tz - z) ^ 2)
        local seconds = moves and math.min(60, math.max(7, distance / (aircraft and 16 or 6))) or 5
        local job = {
            kind = "field",
            target = target,
            role = node.role,
            begin = s.tick,
            finish = s.tick + math.ceil(seconds * 30),
        }
        if moves then
            local offset = (dest.size and dest.size[1] or 2) * 0.5
                + (node.size and node.size[1] or 2) * 0.5
                + 2
            job.origin = { x, y, z }
            job.destination = { tx + offset, ty, tz }
            job.air = aircraft
            local route = ctx.navigation.plan(ctx, s, key, target)
            if not route then
                return false
            end
            job.route = route
        end
        s.service[key] = job
        return true
    end
    local function advance(s, key, job)
        if not job.origin then
            return
        end
        local t = math.min(1, (s.tick - job.begin) / (job.finish - job.begin))
        local p, q = job.origin, job.destination
        if job.route then
            local span = #job.route
            local part = math.min(span - 1, math.floor(t * span))
            p = part > 0 and job.route[part] or p
            q = job.route[part + 1]
            t = t == 1 and 1 or t * span - part
        end
        s.frames[ctx.index[key]] = {
            p[1] + (q[1] - p[1]) * t,
            p[2] + (q[2] - p[2]) * t + (job.air and 20 * math.sin(math.pi * t) or 0),
            p[3] + (q[3] - p[3]) * t,
            0,
            0,
        }
    end
    local function finish(s, key, job)
        local x, _, z = position(s, job.target)
        s.contacts[key] =
            { target = job.target, x = math.floor(x + 0.5), z = math.floor(z + 0.5), tick = s.tick }
        if job.kind ~= "field" or observers[job.role] then
            return "position reported"
        end
        if freight[job.role] then
            if s.supply_stock == 0 then
                return "reserve stores empty"
            end
            s.supply_stock = s.supply_stock - 1
            s.blue.alloy = s.blue.alloy + 1
            return "one alloy delivered"
        end
        if watch[job.role] then
            s.watchposts[job.target] = key
            return "watch established"
        end
        for _, order in pairs(s.jobs or {}) do
            if order.actor == job.target and not order.assisted then
                order.assisted = true
                order.elapsed = math.min(order.length - 1, order.elapsed + 60)
                return "shift reinforced"
            end
        end
        local bay = ({ fitters = "fit", watch = "watch", motor = "motor" })[job.target]
        local queue = bay and s.blue.queues[bay]
        if queue and not queue.assisted then
            queue.assisted = true
            queue.left = math.max(1, queue.left - 30)
            return "bay crew reinforced"
        end
        return "station serviced"
    end
    return { start = start, advance = advance, finish = finish }
end
