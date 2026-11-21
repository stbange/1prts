-- Per-instance yards and shared plate lots for Gate Six. The original three
-- teaching bays use their original prices and input bindings.
return function(ctx, rules)
    local M = {}
    local pads = { ["west-pad"] = true, ["east-pad"] = true, ["bank-pad"] = true }
    local recipes = rules.jobs
    local function faction(s, a)
        return a == s.red and "rival" or ctx.owner
    end
    local function say(s, a, message)
        if a == s.blue then
            s.report = message
        end
        return false
    end
    function M.new(s)
        s.army = {
            bays = {},
            sites = {},
            counts = {},
            offline = {},
            priority = {},
            lots = { ["home-lot"] = 8, ["north-lot"] = 8, ["common-lot"] = 1 },
        }
        s.blue.lot, s.red.lot = "home-lot", "north-lot"
    end
    local function output(id, rival)
        if id == "section" then
            return rival and "channel-section" or "gate-section"
        end
        if id == "armor" then
            return rival and "wake" or "lintel"
        end
        return id
    end
    local function ready_space(s)
        local pending = 1
        for _ in pairs(s.army.bays) do
            pending = pending + 1
        end
        return not orra_reserve_scene or orra_reserve_scene(#ctx.site + pending * 3)
    end
    function M.queue(s, a, id, producer, pad)
        local recipe = recipes[id]
        if not recipe then
            return false
        end
        local rival = a == s.red
        if pad and s.army.sites[pad] then
            return say(s, a, "Survey pad occupied.")
        end
        producer = producer or (rival and "rival-" or "") .. recipe.bay
        local works = s.army.sites[producer]
        if not works and recipe.needs and not a[recipe.needs] then
            return say(s, a, "Commission " .. recipe.needs .. " first.")
        end
        if pad and (not pads[pad] or s.army.sites[pad]) then
            return say(s, a, "Survey pad occupied.")
        end
        if s.army.bays[producer] or a.queues[recipe.bay] and not works then
            return say(s, a, "Bay is already working.")
        end
        if a.alloy < recipe.cost then
            return say(s, a, "More alloy needed.")
        end
        if not ready_space(s) then
            return say(s, a, "No room for another crew in this sector.")
        end
        s.sequence = s.sequence + 1
        a.alloy, a.reserved = a.alloy - recipe.cost, a.reserved + recipe.cost
        s.army.bays[producer] = {
            id = id,
            owner = faction(s, a),
            pad = pad,
            serial = s.sequence,
            left = recipe.duration,
            cost = recipe.cost,
        }
        if pad then
            s.army.sites[pad] = { owner = faction(s, a), reserved = true }
        end
        say(s, a, producer .. " accepted " .. recipe.label .. ".")
        return true
    end
    function M.cancel(s, a, minimum)
        local key, newest
        for bay, q in pairs(s.army.bays) do
            if q.owner == faction(s, a) and (not newest or q.serial > newest.serial) then
                key, newest = bay, q
            end
        end
        if not newest or minimum and minimum > newest.serial then
            return false
        end
        if not newest.started then
            a.alloy = a.alloy + newest.cost
            a.reserved = a.reserved - newest.cost
        end
        if newest.pad then
            s.army.sites[newest.pad] = nil
        end
        s.army.bays[key] = nil
        say(s, a, "Released " .. key .. ".")
        return true
    end
    local function exit_position(s, bay, q)
        local pos = ctx.site[ctx.index[q.pad or bay]]
        if q.pad then
            return { pos[2], 0, pos[4] }
        end
        local x, z = pos[2], pos[4]
        if pos[6] == "motor" then
            x, z = -16, 19
        elseif pos[6] == "redmotor" then
            x, z = -33, -80
        else
            z = z + ((pos.size and pos.size[3] or 12) / 2) + 9
        end
        for _, entry in ipairs(s.spawned) do
            local row = ctx.site[ctx.index[entry.key]]
            local f = s.frames[ctx.index[entry.key]] or entry.pos
            if row.hull and math.abs(f[1] - x) < 4 and math.abs(f[3] - z) < 6 then
                return nil
            end
        end
        return { x, 0, z }
    end
    local function finish(s, a, bay, q, pos)
        local unit = output(q.id, a == s.red)
        local prefix = a == s.red and "rival-" or ""
        local counter = prefix .. unit
        local n = (s.army.counts[counter] or 1) + 1
        local key = counter .. "-" .. n
        local first = #ctx.site + 1
        ctx.add_entity(key, unit, pos)
        ctx.register(s, first)
        local instance = ctx.site[ctx.index[key]]
        instance.owner = q.owner
        s.army.counts[counter] = n
        s.spawned[#s.spawned + 1] = { key = key, unit = unit, pos = pos, owner = q.owner }
        if q.pad then
            a.draw = a.draw + (recipes[q.id].draw or 0)
            a[q.id] = true
            s.army.sites[q.pad] = { owner = q.owner, building = key }
            s.army.sites[key] = { owner = q.owner, type = unit, pad = q.pad }
        end
        s.army.bays[bay] = nil
        say(s, a, instance.name .. " ready at " .. bay .. ".")
    end
    function M.tick(s)
        if not next(s.army.bays) then
            return
        end
        local keys = {}
        for key in pairs(s.army.bays) do
            keys[#keys + 1] = key
        end
        table.sort(keys)
        for _, bay in ipairs(keys) do
            local q = s.army.bays[bay]
            local a = q.owner == "rival" and s.red or s.blue
            local powered = q.pad
                or a.power and not s.army.offline[bay]
                or a.reserve > 0 and s.army.priority[a == s.red and "rival" or "home"] == bay
            if a.staffed and not s.service[bay] and powered and not (q.pad and a.away) then
                if not q.started then
                    q.started = true
                    a.reserved = a.reserved - q.cost
                end
                if not q.pad and not a.power then
                    a.reserve = a.reserve - 1
                end
                q.left = math.max(0, q.left - 1)
                if q.left == 0 then
                    local pos = exit_position(s, bay, q)
                    if pos then
                        finish(s, a, bay, q, pos)
                    else
                        say(s, a, bay .. " exit occupied. Move the parked hull.")
                    end
                end
            end
        end
    end
    function M.haul(s, a)
        local rival = a == s.red
        local home = rival and "north-lot" or "home-lot"
        local lot = a.lot or home
        local lots = s.army.lots
        if not a.harvesting or a.haul == 0 and lots[lot] == 0 then
            return
        end
        if a.haul < 299 then
            a.haul = a.haul + 1
            if a.haul == 150 then
                if lots[lot] == 0 then
                    a.haul = 0
                    a.lot = home
                    say(s, a, "Last load claimed. Returning to the home lot.")
                else
                    lots[lot] = lots[lot] - 1
                    a.stock = 1
                    a.loads = lots[home]
                end
            end
        elseif a.mill and a.power and a.staffed then
            a.haul, a.stock, a.alloy = 0, 0, a.alloy + 4
            if lots[lot] == 0 then
                a.lot = home
            end
            say(s, a, "Mole to dispatch. Four alloy delivered.")
        end
        if lot == "common-lot" then
            local i = ctx.index[rival and "rival-mole" or "mole"]
            local row = ctx.site[i]
            local target = ctx.site[ctx.index[lot]]
            local t = 1 - math.abs(a.haul / 150 - 1)
            s.frames[i] =
                { row[2] + (target[2] - row[2]) * t, 0, row[4] + (target[4] - row[4]) * t, row[5], 0 }
        elseif a.haul == 0 then
            s.frames[ctx.index[rival and "rival-mole" or "mole"]] = nil
        end
    end
    function M.lot(s, a, lot)
        if a.haul ~= 0 or not s.army.lots[lot] or s.army.lots[lot] <= 0 then
            return say(s, a, "Hauler busy or lot empty.")
        end
        a.lot, a.harvesting = lot, true
        say(s, a, "Hauler dispatched to " .. lot .. ".")
        return true
    end
    function M.status(s, key)
        local bay = s.army.bays[key]
        if bay then
            local a = bay.owner == "rival" and s.red or s.blue
            local reason = bay.left == 0 and "exit occupied"
                or s.service[key] and "crew away"
                or (not a.power or s.army.offline[key]) and not bay.pad and "grid isolated"
                or "on shift"
            return ("%s / %s / %ds / %s"):format(key, bay.id, math.ceil(bay.left / 30), reason)
        end
        local lots = s.army.lots[key]
        if lots then
            return ("%s / %d loads"):format(key, lots)
        end
        if key == "mole" then
            return ("%s / cargo %d / %d alloy"):format(s.blue.lot, s.blue.stock, s.blue.alloy)
        end
    end
    function M.command(s, id)
        if id == "common" then
            return M.lot(s, s.blue, "common-lot")
        end
        local kind, pad = id:match("^build:([%w-]+)@([%w-]+)$")
        if kind and recipes[kind] and recipes[kind].bay == "fit" then
            return M.queue(s, s.blue, kind, "fit", pad)
        end
        local action, bay = id:match("^bay:([%w-]+)@([%w-]+)$")
        if
            action
            and (
                s.army.sites[bay] and s.army.sites[bay].owner == ctx.owner
                or (bay == "watch" or bay == "motor") and s.blue[bay]
            )
        then
            if action == "power" then
                s.army.offline[bay] = not s.army.offline[bay]
                return true
            end
            if action == "priority" then
                s.army.priority.home = bay
                return true
            end
            if
                recipes[action]
                and recipes[action].bay == (s.army.sites[bay] and s.army.sites[bay].type or bay)
            then
                return M.queue(s, s.blue, action, bay)
            end
        end
        return false
    end
    return M
end
