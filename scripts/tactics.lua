-- Crews keep their call signs through movement, a disabled shift and recovery.
return function(ctx, nav)
    local M = {}
    local combat = { guard = true, occupy = true, patrol = true, landing = true, support = true }
    local repair = { repair = true, tow = true, recover = true, refit = true }
    local verbs = {
        move = true,
        scout = true,
        escort = true,
        engage = true,
        hold = true,
        withdraw = true,
        support = true,
    }
    local function sorted(t)
        local keys = {}
        for k in pairs(t) do
            keys[#keys + 1] = k
        end
        table.sort(keys)
        return keys
    end
    local function say(s, text)
        s.report = text
        return false
    end
    function M.new(s)
        s.tactical =
            { orders = {}, group = {}, health = {}, locks = {}, posted = {}, intel = {}, claims = {} }
    end
    local function active(s, row)
        if not row then
            return false
        end
        local cond = row[6]
        if row.owner == "rival" then
            return cond ~= "enemyarmor" and cond ~= "enemysection"
                or (cond == "enemyarmor" and s.red.armor or cond == "enemysection" and s.red.section)
        end
        return not cond or cond == "haul" or cond == "fitters" or s.blue[cond]
    end
    local function role(row)
        return row.role or (ctx.fleet[row.type] or {}).role
    end
    local function movable(row)
        local def = ctx.fleet[row.type]
        return def
            and (
                row.hull
                or row[6] == "armor"
                or row[6] == "haul"
                or row[6] == "enemyarmor"
                or row[6] == "redhaul"
                or def.family == "people"
            )
    end
    local function clear(s, key)
        local t = s.tactical
        for k, holder in pairs(t.locks) do
            if holder == key then
                t.locks[k] = nil
            end
        end
        t.orders[key], t.posted[key] = nil, nil
    end
    local function sight(s, key, target)
        local a, b = nav.position(ctx, s, key), nav.position(ctx, s, target)
        return a and b and (a[1] - b[1]) ^ 2 + (a[2] - b[2]) ^ 2 + (a[3] - b[3]) ^ 2 < 58 ^ 2
    end
    local function observe(s, key, target)
        local p = nav.position(ctx, s, target)
        local contact = {
            observer = key,
            target = target,
            x = math.floor(p[1] + 0.5),
            z = math.floor(p[3] + 0.5),
            tick = s.tick,
        }
        local row = ctx.site[ctx.index[key]]
        if row.owner == "rival" then
            s.tactical.intel[target] = contact
        else
            s.contacts[key] = contact
        end
    end
    function M.issue(s, key, verb, target, owner, single, validate)
        local row, dest = ctx.site[ctx.index[key]], ctx.site[ctx.index[target]]
        if not active(s, row) or row.owner ~= owner or not row.role and owner ~= "rival" then
            return say(s, "Crew unavailable.")
        end
        local t = s.tactical
        if verb == "group" then
            if not movable(row) then
                return say(s, "Select a mobile crew.")
            end
            t.group[key] = not t.group[key] or nil
            say(s, key .. (t.group[key] and " joined the group." or " left the group."))
            return true
        end
        if not verbs[verb] then
            return false
        end
        if not active(s, dest) or not movable(row) or (t.health[key] or 3) <= 0 or s.service[key] then
            return say(s, "Crew or destination unavailable.")
        end
        for _, job in pairs(s.jobs or {}) do
            if job.actor == key then
                return say(s, "Crew is on a scheduled shift.")
            end
        end
        if
            verb == "engage" and (not combat[role(row)] or owner == dest.owner or not sight(s, key, target))
        then
            return say(s, "No opposing contact in sight.")
        end
        if verb == "support" and not (combat[role(row)] or repair[role(row)] or row.type == "span") then
            return say(s, "This crew cannot support the assignment.")
        end
        if verb == "escort" and (owner ~= dest.owner or key == target) then
            return say(s, "Choose another friendly crew to escort.")
        end
        local old = t.orders[key] or t.posted[key]
        if old and old.verb == verb and old.target == target then
            return say(s, "Order already acknowledged.")
        end
        if not single and t.group[key] then
            -- Validate the entire group on a temporary order table before committing.
            local staged = {}
            for _, member in ipairs(sorted(t.group)) do
                local other = ctx.site[ctx.index[member]]
                if not active(s, other) or (t.health[member] or 3) <= 0 or s.service[member] then
                    return say(s, "A selected crew is unavailable.")
                end
                if not M.issue(s, member, verb, target, owner, true, true) then
                    return false
                end
                staged[#staged + 1] = member
            end
            local accepted = false
            for _, member in ipairs(staged) do
                accepted = M.issue(s, member, verb, target, owner, true) or accepted
            end
            return accepted
        end
        local occupied = {}
        for _, collection in ipairs({ t.orders, t.posted }) do
            for member, job in pairs(collection) do
                if member ~= key and job.target == target then
                    occupied[job.slot] = true
                end
            end
        end
        local slot = 0
        while occupied[slot] do
            slot = slot + 1
        end
        local reported
        if owner ~= dest.owner and dest.owner ~= "neutral" and not sight(s, key, target) then
            local contacts = owner == "rival" and t.intel or s.contacts
            local latest
            for _, contact in pairs(contacts) do
                if contact.target == target and (not latest or latest.tick < contact.tick) then
                    latest = contact
                end
            end
            if not latest then
                return say(s, "No report of this crew. Send an observer.")
            end
            reported = { latest.x, 0, latest.z }
        end
        local path, why = nav.plan(ctx, s, key, target, slot, reported)
        if not path then
            return say(s, why)
        end
        if validate then
            return true
        end
        clear(s, key)
        s.sequence = s.sequence + 1
        local order = {
            verb = verb,
            target = target,
            path = path,
            leg = 1,
            slot = slot,
            serial = s.sequence,
            opened = s.crossing or false,
            reported = reported,
        }
        if verb == "hold" then
            t.posted[key] = order
        else
            t.orders[key] = order
        end
        say(s, key .. " acknowledged " .. verb .. " / " .. target .. ".")
        return true
    end
    function M.command(s, id)
        local member = id:match("^group:([%w-]+)$")
        if member then
            return M.issue(s, member, "group", member, ctx.owner)
        end
        local action, key, target = id:match("^([%a]+):([%w-]+)@([%w-]+)$")
        if verbs[action] then
            return M.issue(s, key, action, target, ctx.owner)
        end
    end
    function M.release(s, key)
        if not s.tactical.orders[key] and not s.tactical.posted[key] then
            return false
        end
        clear(s, key)
        say(s, key .. " holding position. Field order released.")
        return true
    end
    local function arrive(s, key, job)
        local t = s.tactical
        local row = ctx.site[ctx.index[key]]
        local target = job.target
        if job.towing then
            local p = nav.position(ctx, s, job.towing)
            s.frames[ctx.index[job.towing]] = { p[1], p[2], p[3], p[4], 0 }
            t.health[job.towing] = 3
            s.instances[job.towing].status = "ready"
            s.instances[job.towing].parent = nil
            say(s, job.towing .. " recommissioned at " .. job.target .. ".")
            t.orders[key], t.posted[key] = nil, job
            return
        end
        t.orders[key] = nil
        t.posted[key] = job
        if job.verb == "support" and row.type == "span" then
            s.crossing = true
        elseif job.verb == "support" and repair[role(row)] and t.health[target] == 0 then
            local shop = row.owner == "rival" and "rival-motor"
                or ctx.index.shed and "shed"
                or ctx.index.motor and "motor"
                or ctx.index.cradle and "cradle"
            local tow = shop and (row.type == "hinge" or row.type == "cleat") and nav.plan(ctx, s, key, shop)
            if tow then
                s.frames[ctx.index[target]] = { 0, 0, 16, 0, ctx.index[key] }
                s.instances[target].parent = key
                job.towing, job.target, job.path, job.leg = target, shop, tow, 1
                t.orders[key], t.posted[key] = job, nil
                say(s, key .. " towing " .. target .. " to " .. shop .. ".")
                return
            end
            t.health[target] = 3
            s.instances[target].status = "ready"
            say(s, target .. " recommissioned by " .. key .. ".")
            return
        elseif job.verb == "engage" and sight(s, key, target) then
            local cover = s.watchposts[target]
                or (ctx.terrain == "railbed" and ctx.site[ctx.index[target]].type == "anvil")
            t.health[target] = math.max(0, (t.health[target] or 3) - (cover and 1 or 3))
            if t.health[target] == 0 then
                s.instances[target].status = "disabled"
                clear(s, target)
                say(s, target .. " disabled. Crew remains aboard.")
                return
            end
        elseif job.verb == "support" then
            s.watchposts[target] = key
        elseif job.verb == "scout" and sight(s, key, target) then
            observe(s, key, target)
        end
        say(s, key .. " on station / " .. target .. ".")
    end
    function M.tick(s)
        local t = s.tactical
        if next(t.posted) and s.tick % 30 == 0 then
            for _, key in ipairs(sorted(t.posted)) do
                local job = t.posted[key]
                if job.verb == "escort" then
                    local path = nav.plan(ctx, s, key, job.target, job.slot)
                    local goal, p = path[#path], nav.position(ctx, s, key)
                    if (goal[1] - p[1]) ^ 2 + (goal[3] - p[3]) ^ 2 > 25 then
                        job.path, job.leg = { goal }, 1
                        t.orders[key] = job
                        t.posted[key] = nil
                    end
                end
            end
        end
        if next(t.orders) then
            for _, key in ipairs(sorted(t.orders)) do
                local job = t.orders[key]
                if job then
                    local row = ctx.site[ctx.index[key]]
                    if not not s.crossing ~= job.opened then
                        job.path = assert(nav.plan(ctx, s, key, job.target, job.slot, job.reported))
                        job.leg = 1
                        job.opened = not not s.crossing
                    end
                    local p = nav.position(ctx, s, key)
                    local q = job.path[job.leg]
                    local lock = q[4]
                    if lock and t.locks[lock] and t.locks[lock] ~= key then
                        job.waited = (job.waited or 0) + 1
                        if job.waited == 90 then
                            local alternative = nav.bypass(ctx, q)
                            if alternative then
                                job.path[job.leg] = alternative
                                say(s, key .. " diverting through the passing lane.")
                            end
                        end
                    else
                        job.waited = nil
                        if lock then
                            t.locks[lock] = key
                        end
                        local dx, dy, dz = q[1] - p[1], q[2] - p[2], q[3] - p[3]
                        local d = math.sqrt(dx * dx + dy * dy + dz * dz)
                        local speed = ({
                            people = 0.18,
                            aircraft = 0.72,
                            tiltrotor = 0.64,
                            tender = 0.9,
                            tug = 0.75,
                        })[nav.family(ctx, row)] or 0.36
                        local ratio = d > 0 and math.min(1, speed / d) or 1
                        s.frames[ctx.index[key]] =
                            { p[1] + dx * ratio, p[2] + dy * ratio, p[3] + dz * ratio, math.atan(dx, -dz), 0 }
                        if ratio == 1 then
                            if lock then
                                t.locks[lock] = nil
                            end
                            job.leg = job.leg + 1
                            if not job.path[job.leg] then
                                arrive(s, key, job)
                            end
                        end
                    end
                    if job.verb == "scout" and s.tick % 30 == 0 and sight(s, key, job.target) then
                        observe(s, key, job.target)
                    end
                end
            end
        end
        if s.army and s.tick % 60 == 0 then
            for _, entry in ipairs(s.spawned) do
                if entry.owner and entry.owner ~= "rival" and sight(s, "rival-survey", entry.key) then
                    observe(s, "rival-survey", entry.key)
                end
            end
        end
    end
    function M.yard(s)
        local t = s.tactical
        if not next(t.orders) and not next(t.posted) and not next(t.health) then
            return
        end
        local forces = {
            home = { occupy = s.blue.walk == 300, guard = s.blue.drive == 210 },
            rival = { occupy = s.red.walk == 300, guard = s.red.drive == 210 },
        }
        for _, entry in ipairs({
            { "gate-section", "home", "occupy" },
            { "lintel", "home", "guard" },
            { "rival-section", "rival", "occupy" },
            { "rival-wake", "rival", "guard" },
        }) do
            if s.frames[ctx.index[entry[1]]] or t.health[entry[1]] == 0 then
                forces[entry[2]][entry[3]] = false
            end
        end
        for _, set in ipairs({ t.orders, t.posted }) do
            for key in pairs(set) do
                local row = ctx.site[ctx.index[key]]
                local p = nav.position(ctx, s, key)
                if (t.health[key] or 3) > 0 and math.abs(p[1]) <= 19 and math.abs(p[3] + 66) <= 14 then
                    local side = row.owner == "rival" and forces.rival or forces.home
                    if role(row) == "occupy" then
                        side.occupy = true
                    elseif combat[role(row)] then
                        side.guard = true
                    end
                end
            end
        end
        return forces
    end
    function M.reinforcement(s)
        local a = s.red
        for _, key in ipairs(sorted(s.tactical.health)) do
            local row = ctx.site[ctx.index[key]]
            if
                row.owner == "rival"
                and s.tactical.health[key] == 0
                and ctx.index["rival-recovery"]
                and not s.tactical.orders["rival-recovery"]
            then
                M.issue(s, "rival-recovery", "support", key, "rival", true)
            end
        end
        if not a.motor or not a.armor then
            return
        end
        for _, key in ipairs(sorted(s.tactical.intel)) do
            local contact = s.tactical.intel[key]
            if s.tick - contact.tick <= 240 then
                local row = ctx.site[ctx.index[key]]
                local kind = combat[role(row)] and "armor" or "section"
                if not s.tactical.claims[key] and ctx.army.queue(s, a, kind) then
                    s.tactical.claims[key] = true
                end
            elseif not contact.diverted and not s.tactical.orders["rival-wake"] then
                contact.diverted = true
                M.issue(s, "rival-wake", "scout", key, "rival", true)
            end
        end
    end
    return M
end
