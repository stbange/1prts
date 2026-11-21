-- A field shift acts on a named workplace and leaves a receipt the next crew can use.
return function(ctx)
    local M = {}
    local tasks = {
        fitters = "fit",
        mole = "load",
        kestrel = "berth",
        porter = "crew",
        sump = "drain",
        kite = "cargo",
        lark = "dock",
        tag = "survey",
        dray = "cargo",
        ["gate-section"] = "guard",
        lintel = "guard",
        plumb = "survey",
        span = "crossing",
        tally = "support",
        ["channel-section"] = "embark",
        wake = "embark",
        reed = "sound",
        hinge = "recover",
        crossbill = "survey",
        points = "switch",
        shunter = "cargo",
        anvil = "guard",
        sleeper = "track",
        whistle = "survey",
        ["chain-crew"] = "survey",
        glint = "assay",
        barrow = "sample",
        prism = "survey",
        shoe = "guard",
        ["ground-watch"] = "guard",
        tern = "mail",
        gimbal = "guide",
        gantry = "recover",
        rook = "survey",
        ["collar-watch"] = "guard",
        cleat = "berth",
        spindle = "cargo",
        rivet = "recover",
        loom = "crew",
        survey = "survey",
        dynamo = "power",
        mill = "refine",
        watch = "crew",
        motor = "fit",
        post = "guard",
        flight = "recover",
        depot = "store",
        relay = "sound",
        pump = "drain",
        stores = "store",
        infirmary = "crew",
        signals = "survey",
        slipway = "berth",
        quarters = "crew",
        siding = "cargo",
        customs = "berth",
        beacon = "guide",
        lock = "dock",
        mooring = "berth",
        spaceport = "dock",
        gallery = "reserve",
        shed = "recover",
        roundhouse = "fit",
        assay = "assay",
        tower = "guide",
        cradle = "recover",
        gate = "guard",
        exchange = "switch",
        station = "dock",
    }
    function M.new(s)
        s.works =
            { sites = {}, receipts = {}, inventory = { freight = 12, mail = 3, sample = 3, alloy = 6 }, cargo = {}, crew = {} }
    end
    function M.ready(s, key, target)
        local row = ctx.site[ctx.index[key]]
        if not row or row.owner == "rival" or s.tactical.orders[key] then
            return false
        end
        local dest = s.works.sites[target]
        local task = tasks[row.type]
        if dest and dest.closed and task ~= "drain" and task ~= "recover" then
            return false
        end
        if s.tactical.health[key] == 0 or s.service[target] and s.service[target].station == key then
            return false
        end
        local source = (
            { cargo = "freight", mail = "mail", sample = "sample", refine = "alloy", load = "freight" }
        )[task]
        return not source or s.works.inventory[source] > 0
    end
    function M.finish(s, key, target)
        local row = ctx.site[ctx.index[key]]
        local job = tasks[row.type] or ctx.fleet[row.type] and ctx.fleet[row.type].work
        local w = s.works
        local place = w.sites[target] or {}
        w.sites[target] = place
        local message
        if job == "cargo" or job == "load" or job == "mail" or job == "sample" or job == "refine" then
            local stock = ({ sample = "sample", mail = "mail", refine = "alloy" })[job] or "freight"
            if w.inventory[stock] <= 0 then
                return "store empty"
            end
            w.inventory[stock] = w.inventory[stock] - 1
            w.cargo[target] = (w.cargo[target] or 0) + 1
            if job == "refine" then
                s.blue.alloy = s.blue.alloy + 1
            end
            message = stock .. " received / " .. w.cargo[target]
        elseif job == "store" then
            local count = w.cargo[target] or 0
            w.inventory.freight = w.inventory.freight + count
            w.cargo[target] = 0
            place.stock = w.inventory.freight
            message = "stores checked / " .. place.stock
        elseif job == "survey" or job == "sound" or job == "assay" then
            local p = ctx.position(s, target)
            place.chart = {
                x = p[1],
                z = p[3],
                observer = key,
                at = s.tick,
                loads = s.army and s.army.lots[target] or w.cargo[target] or 0,
            }
            message = job .. " report / " .. place.chart.loads .. " loads"
        elseif job == "recover" or job == "fit" then
            s.tactical.health[target] = 3
            s.instances[target].status = "ready"
            place.closed = false
            place.serviced = s.tick
            message = "crew cleared for service"
        elseif job == "crew" or job == "embark" then
            local roster = s.instances[key].crew
            local worker = roster[1] or key
            w.crew[worker] = target
            if
                job == "embark"
                and key ~= target
                and ctx.site[ctx.index[target]].hull
                and row.family == "people"
            then
                s.frames[ctx.index[key]] = { 0, 0.84, 1, 0, ctx.index[target] }
            end
            place.staff = worker
            message = "watch relieved / " .. worker
        elseif job == "power" or job == "reserve" then
            s.blue.power = true
            if job == "reserve" then
                s.blue.reserve = math.max(s.blue.reserve or 0, 150)
            end
            place.power = true
            message = "grid feed live"
        elseif job == "guard" or job == "support" then
            s.watchposts[target] = key
            place.cover = job == "support" and 2 or 1
            message = "yard watch / " .. key
        elseif job == "crossing" or job == "track" or job == "switch" or job == "drain" then
            place.closed = false
            place.route = job
            place.opened = s.tick
            if job == "crossing" then
                s.crossing = true
            end
            message = job .. " route open"
        elseif job == "berth" or job == "dock" or job == "guide" then
            place.slot = { crew = key, until_tick = s.tick + 180 }
            message = "approach slot until " .. (s.tick + 180)
        else
            return
        end
        w.receipts[key] = { job = job, target = target, tick = s.tick, message = message }
        return message
    end
    return M
end
