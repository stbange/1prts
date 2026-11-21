-- Each row is a dispatchable job: order, label, crew, seconds, prerequisites, destination and bay.
local plans = {
    ["compact-bank"] = {
        limit = 330,
        goal = "restore",
        brief = "Tow the Slip Shed and reopen the north mooring.",
        jobs = {
            { "sound", "Sound the shallow lane", "reed", 12, nil, { 10, 0, -74 } },
            { "embark", "Embark Channel Section", "channel-section", 6, nil, { 0, 0.84, 1 }, "wake" },
            { "cross", "Cross the marked shallows", "wake", 32, "sound embark", { 11, 0, -92 } },
            { "land", "Land the Channel Section", "channel-section", 6, "cross", { 16, 0, -104 } },
            { "repair", "Begin repair at Slip Shed", "shed", 26 },
            { "tow", "Tow shed to north mooring", "hinge", 38, "sound", { 28, 0, -105 } },
            { "patrol", "Dispatch a patrol sortie", "crossbill", 35, nil, { 35, 27, -120 }, "flight" },
            { "restore", "Restore harbor watch", "fitters", 16, "land repair tow", { 26, 0, -101 } },
        },
    },
    westbound = {
        limit = 265,
        goal = "freight",
        brief = "Reopen West Exchange and bring in the winter freight.",
        jobs = {
            { "survey", "Inspect the frostbound line", "whistle", 16, nil, { 2, 0, -66 } },
            { "heater", "Fit Points Heater / 4", "roundhouse", 12, nil, nil, nil, 4 },
            { "reserve", "Charge gallery reserve", "gallery", 8 },
            { "track", "Replace damaged track", "sleeper", 24, "survey", { 0, 0, -100 }, "workshop" },
            { "switch", "Set the exchange points", "points", 24, "track", { 2, 0, -156 }, "workshop" },
            { "guard", "Hold the exchange siding", "anvil", 20, nil, { 6, 0, -150 } },
            { "freight", "Dispatch winter freight", "shunter", 40, "switch guard", { 0, 0, -153 } },
            { "power", "Switch workshop grid", "dynamo", 0 },
        },
    },
    ["white-mile"] = {
        limit = 255,
        goal = "assay-load",
        brief = "Recover the last sample from the flooded field camp.",
        jobs = {
            { "scout", "Mark control points", "chain-crew", 12, nil, { 3, 0, -92 } },
            { "drain", "Drain the camp entrance", "sump", 26, "scout", { 26, 0, -67 } },
            { "shoes", "Fit Wide Shoes / 4", "shoe", 12, nil, nil, nil, 4 },
            { "chart", "Relay the survey bearing", "prism", 18, "scout", { -13, 0, -92 } },
            { "sample", "Collect the assay load", "barrow", 33, "drain chart", { 4, 0, -101 } },
            { "analyze", "Analyze the sample afloat", "glint", 20, "sample", { -23, 0, -112 } },
            { "assay-load", "Return load to assay office", "barrow", 32, "analyze", { -23, 0, -128 } },
        },
    },
    ["night-mail"] = {
        limit = 310,
        goal = "mail-3",
        brief = "Share the Flight Shed and deliver three bags before dawn.",
        jobs = {
            { "board", "Install Night Board / 4", "tower", 13, nil, nil, nil, 4 },
            { "beacon", "Set approach beacon", "gimbal", 12, nil, { 19, 0, -129 } },
            { "patrol", "Launch observation patrol", "crossbill", 32, "beacon", { 44, 33, -135 }, "apron" },
            { "clear", "Clear the landing office", "ground-watch", 12, "beacon", { 14, 0, -137 } },
            { "mail-1", "Fly bag 1 by courier", "tern", 26, "clear", { 8, 0, -152 }, "apron" },
            { "mail-2", "Fly bag 2 by tiltrotor", "kite", 34, "clear", { -19, 0, -153 }, "apron" },
            { "escort", "Bring observation crew home", "rook", 20, "patrol", { 25, 0, -45 }, "apron" },
            {
                "mail-3",
                "Land the last mailbag",
                "kite",
                32,
                "mail-1 mail-2 escort",
                { 13, 0, -158 },
                "apron",
            },
        },
    },
    ["high-water"] = {
        limit = 360,
        goal = "traffic",
        brief = "Deliver a pressure module and restore Crown Station traffic control.",
        jobs = {
            { "module", "Load the pressure module", "dray", 18, nil, { -2, 0, 45 } },
            { "collar", "Commission Second Collar / 4", "cradle", 22, nil, nil, nil, 4 },
            { "service", "Service the occupied collar", "cleat", 55, nil, { 10, 84, -270 }, "collar" },
            { "lift", "Launch orbital tender", "lark", 60, "module", { 0, 65, -175 } },
            { "supply", "Run surface power service", "spaceport", 30 },
            { "dock", "Dock at Crown Station", "lark", 22, "lift", { 0, 84, -226.35 }, "collar" },
            { "transfer", "Transfer pressure module", "spindle", 25, "dock", { 6, 84, -315 } },
            { "watch", "Deliver the station watch", "loom", 24, "dock", { 0, 84, -277 } },
            { "repair", "Seal the transfer joint", "rivet", 16, "transfer", { 8, 84, -279 } },
            {
                "traffic",
                "Reopen station traffic",
                "collar-watch",
                12,
                "watch repair supply",
                { 0, 84, -300 },
            },
        },
    },
}
local function report(s, txt)
    s.report = txt
    s.report_at = s.tick
end
return function(id, ctx)
    local plan = assert(plans[id])
    local rules = { hz = 30, hold_ticks = plan.limit * 30, deadline = plan.limit * 30, actions = {} }
    local by_id = {}
    for _, job in ipairs(plan.jobs) do
        by_id[job[1]] = job
        rules.actions[#rules.actions + 1] = { job[1], job[2], job[3] }
    end
    for _, cmd in ipairs({ { "cancel", "Cancel latest crew order" }, { "report", "Request shift report" } }) do
        rules.actions[#rules.actions + 1] = cmd
    end
    if ctx.freight and ctx.freight.checkpoint then
        rules.actions[#rules.actions + 1] =
            { "passage", "Close or open " .. ctx.freight.checkpoint, ctx.freight.checkpoint }
    end
    if id == "night-mail" then
        rules.actions[#rules.actions + 1] = { "handoff", "Load bag two at Shore Depot", "depot" }
    end
    local function advise(s)
        s.advice = nil
        local active
        for _, job in ipairs(plan.jobs) do
            local key = job[1]
            if s.jobs[key] then
                active = active or job
            end
            if
                not (
                    s.done[key]
                    or s.jobs[key]
                    or job[8]
                    or key == "reserve"
                    or key == "power"
                    or id == "compact-bank" and key == "patrol"
                )
            then
                local ready = true
                for req in (job[5] or ""):gmatch("%S+") do
                    if not s.done[req] then
                        ready = false
                        break
                    end
                end
                if ready then
                    s.advice = key
                    s.objective = job[2]
                    return
                end
            end
        end
        if active then
            s.objective = active[3] .. " on shift / " .. active[2]
        end
    end
    function rules.new()
        local s = {
            blue = { alloy = 8, queues = {}, power = true },
            red = { queues = {} },
            tick = 0,
            clock = rules.deadline,
            progress = 0,
            result = "active",
            report = plan.brief,
            objective = plan.brief,
            report_at = 0,
            jobs = {},
            done = {},
            frames = {},
            reserved = 0,
            since = {},
            serial = 0,
        }
        advise(s)
        return s
    end
    function rules.command(s, order)
        if s.result ~= "active" then
            return false
        end
        if order == "passage" then
            return ctx.freight.control(s)
        end
        if order == "report" then
            report(s, s.objective)
            return true
        end
        if order == "cancel" then
            local selected
            for _, work in pairs(s.jobs) do
                if not selected or work.serial > selected.serial then
                    selected = work
                end
            end
            if not selected then
                return false
            end
            if not selected.started then
                s.blue.alloy = s.blue.alloy + selected.cost
                s.reserved = s.reserved - selected.cost
            end
            if s.priority == selected.id then
                s.priority = nil
                s.reserve = false
            end
            s.jobs[selected.id] = nil
            advise(s)
            report(s, "Dispatch canceled " .. selected.id .. ".")
            return true
        end
        if order == "handoff" and id == "night-mail" then
            if s.handoff or s.done["mail-2"] then
                return false
            end
            s.handoff = s.tick + (s.linked_hoist and 5 or 20) * 30
            report(s, "Shore Depot loading the second bag.")
            return true
        end
        local job = by_id[order]
        if not job then
            return false
        end
        if order == "power" then
            s.blue.power = not s.blue.power
            report(s, s.blue.power and "Workshop grid restored." or "Workshop on reserve.")
            return true
        end
        if s.done[order] or s.jobs[order] then
            report(s, "That crew already has the order.")
            return false
        end
        for req in (job[5] or ""):gmatch("%S+") do
            if not s.done[req] then
                report(s, "Required first: " .. by_id[req][2])
                return false
            end
        end
        local cost = job[8] or 0
        if cost > s.blue.alloy then
            report(s, "Insufficient available alloy.")
            return false
        end
        if s.service and s.service[job[3]] or s.tactical and s.tactical.orders[job[3]] then
            report(s, "Crew is on a field shift.")
            return false
        end
        for _, other in pairs(s.jobs) do
            if other.actor == job[3] then
                report(s, "Crew already on assignment.")
                return false
            end
        end
        local seconds = job[4]
        if order == "mail-2" and s.handoff and s.tick >= s.handoff then
            seconds = seconds - 9
        end
        if
            s.done.heater and order == "switch"
            or s.done.shoes and (order == "sample" or order == "assay-load")
        then
            seconds = math.ceil(seconds * 0.55)
        end
        local index = ctx.index[job[3]]
        local loc = s.frames[index] or ctx.site[index]
        local start = s.frames[index] and { loc[1], loc[2], loc[3] } or { loc[2], loc[3], loc[4] }
        if loc[5] and s.frames[index] and loc[5] > 0 then
            local carrier = s.frames[loc[5]] or ctx.site[loc[5]]
            local offset = s.frames[loc[5]] and 0 or 1
            local yaw = carrier[4 + offset] or 0
            start = {
                carrier[1 + offset] + math.cos(yaw) * loc[1] - math.sin(yaw) * loc[3],
                carrier[2 + offset] + loc[2],
                carrier[3 + offset] + math.sin(yaw) * loc[1] + math.cos(yaw) * loc[3],
            }
        end
        s.serial = s.serial + 1
        s.blue.alloy = s.blue.alloy - cost
        s.reserved = s.reserved + cost
        s.jobs[order] = {
            id = order,
            actor = job[3],
            serial = s.serial,
            elapsed = 0,
            length = seconds * 30,
            cost = cost,
            start = start,
        }
        advise(s)
        report(s, job[3] .. " acknowledges: " .. job[2])
        return true
    end
    function rules.step(s)
        if s.result ~= "active" then
            return
        end
        s.tick = s.tick + 1
        s.clock = rules.deadline - s.tick
        if s.clock <= 0 then
            s.result = "defeat"
            report(s, "Shift window closed. " .. plan.brief)
            return
        end
        -- A shared bay serves the earliest confirmed order. Installations open a second slot.
        local owners = {}
        for key, work in pairs(s.jobs) do
            local job = by_id[key]
            local bay = job[7]
            if bay and not (bay == "apron" and s.done.board or bay == "collar" and s.done.collar) then
                owners[bay] = math.min(owners[bay] or math.huge, work.serial)
            end
        end
        local finished = false
        for _, job in ipairs(plan.jobs) do
            local key, actor, bay = job[1], job[3], job[7]
            local work = s.jobs[key]
            if work then
                local paused = ctx.freight and ctx.freight.waits(s, key)
                    or (owners[bay] and owners[bay] ~= work.serial)
                    or (key == "repair" and s.jobs.tow)
                if bay == "workshop" and not s.blue.power then
                    if s.reserve and not s.priority then
                        s.priority = key
                    end
                    paused = paused or s.priority ~= key
                end
                if not paused then
                    if not work.started then
                        work.started = true
                        if ctx.freight then
                            ctx.freight.take(s, key)
                        end
                        s.reserved = s.reserved - work.cost
                    end
                    work.elapsed = work.elapsed + 1
                    local t = math.min(1, work.elapsed / work.length)
                    if job[6] then
                        local dest, origin = job[6], work.start
                        local x = origin[1] + (dest[1] - origin[1]) * t
                        local y = origin[2] + (dest[2] - origin[2]) * t
                        local z = origin[3] + (dest[3] - origin[3]) * t
                        if actor == "kite" or actor == "tern" or actor == "rook" or actor == "crossbill" then
                            y = y + 28 * math.sin(math.pi * t)
                        end
                        s.frames[ctx.index[actor]] = { x, y, z, 0, 0 }
                        if key == "tow" then
                            s.frames[ctx.index.shed] = { x + 2, y, z - 32, 0, 0 }
                        end
                        if key == "cross" then
                            s.frames[ctx.index["channel-section"]] = { 0, 0.84, 1, 0, ctx.index.wake }
                        end
                    end
                    if t == 1 and (not ctx.freight or ctx.freight.deliver(s, key)) then
                        finished = true
                        s.jobs[key] = nil
                        s.done[key] = true
                        s.since[key] = s.tick
                        s.progress = s.progress + 1
                        if key == "embark" then
                            s.frames[ctx.index["channel-section"]] = { 0, 0.84, 1, 0, ctx.index.wake }
                        end
                        if key == "reserve" then
                            s.reserve = true
                        end
                        if s.priority == key then
                            s.priority = nil
                            s.reserve = false
                        end
                        report(s, actor .. " reports " .. key .. " complete.")
                        ctx.emit("complete", s, key)
                        if key == plan.goal and (not ctx.freight or ctx.freight.goal(s)) then
                            s.result = "victory"
                            s.objective = job[2] .. " complete"
                        end
                    end
                end
            end
        end
        if finished and s.result == "active" then
            advise(s)
        end
    end
    return rules
end
