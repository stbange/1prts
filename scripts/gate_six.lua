-- All costs, queues, strategic decisions and victory rules live here.
local game = {}
game.hz, game.hold_ticks = 30, 1800
game.jobs = {
    mill = { cost = 4, duration = 120, draw = 2, bay = "fit", label = "Plate Mill" },
    watch = { cost = 3, duration = 90, draw = 1, bay = "fit", label = "Watch House", needs = "mill" },
    motor = { cost = 6, duration = 150, draw = 3, bay = "fit", label = "Motor Hall", needs = "mill" },
    section = { cost = 2, duration = 90, bay = "watch", label = "Line section", needs = "watch" },
    armor = { cost = 4, duration = 150, bay = "motor", label = "Armored crew", needs = "motor" },
}
game.actions = {
    { "harvest", "Cycle salvage hauler" },
    { "mill", "Commission Plate Mill" },
    { "watch", "Build Watch House" },
    { "motor", "Build Motor Hall" },
    { "section", "Muster line infantry" },
    { "armor", "Produce armor" },
    { "advance", "Send section to gate" },
    { "guard", "Send armor to yard" },
    { "power", "Switch factory power" },
    { "recall", "Withdraw field crews" },
}
local function side()
    return {
        alloy = 8,
        loads = 8,
        haul = 0,
        harvesting = false,
        stock = 0,
        draw = 0,
        power = true,
        mill = false,
        watch = false,
        motor = false,
        section = false,
        armor = false,
        queues = {},
        reserved = 0,
        serial = 0,
        staffed = true,
        reserve = 0,
        priority = "motor",
        rollout = 0,
        advance = false,
        guard = false,
        walk = 0,
        drive = 0,
        hold = 0,
        readiness = 180,
    }
end
function game.new()
    return {
        blue = side(),
        red = side(),
        tick = 0,
        result = "active",
        report = "Survey ready. Commission a Plate Mill.",
    }
end
local function report(s, a, msg)
    if a == s.blue then
        s.report = msg
    end
end
local function order(s, a, id)
    if s.result ~= "active" then
        return false
    end
    local job = game.jobs[id]
    if job then
        if game.army and a[id] and (id == "section" or id == "armor") then
            return game.army.queue(s, a, id)
        end
        if a[id] then
            report(s, a, "Already commissioned: " .. job.label)
            return false
        end
        if job.needs and not a[job.needs] then
            report(s, a, "Required first: " .. game.jobs[job.needs].label)
            return false
        end
        local bay = (a == s.red and "rival-" or "") .. job.bay
        if a.queues[job.bay] or game.army and s.army.bays[bay] then
            report(s, a, "Crew occupied. Wait for the current job.")
            return false
        end
        if a.alloy < job.cost then
            report(s, a, "More alloy needed. Dispatch the hauler.")
            return false
        end
        a.alloy = a.alloy - job.cost
        s.sequence = (s.sequence or 0) + 1
        a.serial = s.sequence
        a.reserved = a.reserved + job.cost
        a.queues[job.bay] = { id = id, left = job.duration, serial = a.serial, cost = job.cost }
        report(s, a, job.label .. " queued. Crew assigned.")
    elseif id == "cancel" then
        if a.pending then
            a.pending = nil
            report(s, a, "Handover canceled.")
            return true
        end
        local newest, bay
        for name, q in pairs(a.queues) do
            if not newest or q.serial > newest.serial then
                newest, bay = q, name
            end
        end
        if game.army and game.army.cancel(s, a, newest and newest.serial) then
            return true
        end
        if not newest then
            report(s, a, "No outstanding order.")
            return false
        end
        if not newest.started then
            a.alloy = a.alloy + newest.cost
            a.reserved = a.reserved - newest.cost
        end
        a.queues[bay] = nil
        report(
            s,
            a,
            newest.started and "Crew stood down. Used stock recorded." or "Crew released. Stock returned."
        )
    elseif id == "staff" then
        a.staffed = not a.staffed
        report(s, a, a.staffed and "Bay watch back on shift." or "Bay watch stood down. Work held.")
    elseif id == "reserve" then
        if a.reserve > 0 or not a.power or a.alloy < 2 then
            report(s, a, "Reserve unavailable.")
            return false
        end
        a.alloy = a.alloy - 2
        a.reserve = 150
        report(s, a, "Gallery charged for one vehicle shift.")
    elseif id == "priority" then
        a.priority = a.priority == "motor" and "watch" or "motor"
        report(s, a, "Reserve feed: " .. a.priority)
    elseif id == "harvest" then
        a.harvesting = not a.harvesting
        report(
            s,
            a,
            a.harvesting and "Mole dispatched to the steel yard." or "Mole holding its present load."
        )
    elseif id == "power" then
        a.power = not a.power
        report(s, a, a.power and "Factory grid restored." or "Factory grid isolated. Production paused.")
    elseif id == "advance" or id == "guard" then
        local unit = id == "advance" and "section" or "armor"
        if not a[unit] then
            report(s, a, "Crew unavailable. Produce the unit first.")
            return false
        end
        if a[id] then
            report(s, a, "Crew already has that field order.")
            return false
        end
        a[id] = true
        report(
            s,
            a,
            id == "advance" and "Gate Section moving to the switch room." or "Lintel moving to the gate yard."
        )
    elseif id == "recall" then
        if not a.advance and not a.guard then
            report(s, a, "Field crews are already home.")
            return false
        end
        a.advance, a.guard = false, false
        report(s, a, "Field crews returning to the yard.")
    else
        return false
    end
    return true
end
-- One follow-on can cross the last three simulation ticks of a production transition.
-- Check each dependency and the price again when the crew actually takes the order.
local function finishing(a, q)
    return q and q.left <= 3 and (game.jobs[q.id].bay == "fit" or a.power)
end
function game.command(s, id)
    local a, job = s.blue, game.jobs[id]
    if s.result ~= "active" or not job or a[id] or a.alloy < job.cost then
        return order(s, a, id)
    end
    if a.pending and a.pending.id == id then
        report(s, a, "Dispatch already holds this order.")
        return false
    end
    local bay = a.queues[job.bay]
    local needs = job.needs and not a[job.needs]
    if bay or needs then
        local supplier = needs and a.queues[game.jobs[job.needs].bay]
        if
            a.pending
            or (bay and (bay.id == id or not finishing(a, bay)))
            or (needs and (not finishing(a, supplier) or supplier.id ~= job.needs))
        then
            return order(s, a, id)
        end
        a.pending = { id = id, expires = s.tick + 3 }
        report(s, a, "Dispatch holding " .. job.label .. " for the handover.")
        return true
    end
    return order(s, a, id)
end
function game.advise(s)
    local a = s.blue
    local advice = not a.mill and not a.queues.fit and "mill"
        or not a.harvesting and a.loads > 0 and "harvest"
        or not a.power and "power"
        or not a.staffed and "staff"
    if not advice then
        for _, id in ipairs({ "watch", "section", "motor", "armor" }) do
            local job = game.jobs[id]
            if
                not a[id]
                and not a.queues[job.bay]
                and (not job.needs or a[job.needs])
                and a.alloy >= job.cost
            then
                advice = id
                break
            end
        end
    end
    advice = advice or a.armor and not a.guard and "guard" or a.section and not a.advance and "advance" or nil
    s.advice = advice
    if advice then
        for i, action in ipairs(game.actions) do
            if action[1] == advice then
                s.objective = "[" .. i .. "] " .. action[2]
                return
            end
        end
    end
    for _, bay in ipairs({ "fit", "motor", "watch" }) do
        local q = a.queues[bay]
        if q then
            s.objective = game.jobs[q.id].label .. " / " .. math.ceil(q.left / 30) .. "s to handover"
            return
        end
    end
    s.objective = a.walk == 300 and "Hold Gate Six / keep the switch room clear"
        or a.advance and "Section moving / cover the gate yard"
        or "Await the next Mole delivery"
end
local function handover(s)
    local a = s.blue
    local pending = a.pending
    if not pending then
        return
    end
    local job = game.jobs[pending.id]
    if not a.queues[job.bay] and (not job.needs or a[job.needs]) then
        a.pending = nil
        order(s, a, pending.id)
    elseif s.tick >= pending.expires then
        a.pending = nil
        report(s, a, "Handover expired. Check crew and power.")
    end
end
local function shift(s, a)
    for _, bay in ipairs({ "fit", "watch", "motor" }) do
        local q = a.queues[bay]
        local site=(a==s.red and "rival-" or "")..bay
        local grid=not game.army and a.power or game.army and game.army.grid_on(s,a,site)
        if
            q
            and a.staffed
            and not a.away
            and (not s.service or not s.service[site])
            and (bay == "fit" or grid or (bay == a.priority and a.reserve > 0))
        then
            if not q.started then
                q.started = true
                a.reserved = a.reserved - (q.cost or 0)
            end
            if bay ~= "fit" and not grid then
                a.reserve = a.reserve - 1
            end
            q.left = q.left - 1
            if q.left == 0 then
                if bay ~= "fit" and not grid then
                    a.reserve = 0
                end
                local job = game.jobs[q.id]
                a[q.id] = true
                a.draw = a.draw + (job.draw or 0)
                a.queues[bay] = nil
                report(
                    s,
                    a,
                    q.id == "armor"
                            and (a == s.blue and "Lintel driver ready. Rear hatch at the Motor Hall exit." or "Wake driver ready at motor exit.")
                        or job.label .. " ready for orders."
                )
            end
        end
    end
    if game.army then
        game.army.haul(s, a)
    elseif a.harvesting and (a.loads > 0 or a.haul > 0) then
        if a.haul < 299 then
            a.haul = a.haul + 1
            if a.haul == 150 then
                a.loads = a.loads - 1
                a.stock = 1
            end
        elseif a.mill and a.power then
            a.haul, a.stock, a.alloy = 0, 0, a.alloy + 4
            report(s, a, "Mole to dispatch. Four alloy delivered.")
        end
    end
    a.walk = math.max(0, math.min(300, a.walk + (a.advance and 1 or -1)))
    if a.armor then
        a.rollout = math.min(120, a.rollout + 1)
    end
    a.drive = math.max(0, math.min(210, a.drive + (a.guard and 1 or -1)))
end
local function rival(s)
    local a = s.red
    if game.army and (a.loads<=2 or s.tactical and next(s.tactical.intel)) and s.army.lots["common-lot"] > 0 and a.haul == 0 then
        game.army.lot(s, a, "common-lot")
    end
    if not a.harvesting then
        order(s, a, "harvest")
    end
    if game.tactics then
        game.tactics.reinforcement(s)
    end
    -- A staffed opening normally favors the switch crew. A reported armored
    -- incursion makes the motor hall and its own relief convoy more urgent.
    local pressure = false
    for _, contact in pairs(s.tactical and s.tactical.intel or {}) do
        if s.tick - contact.tick <= 240 then
            pressure = true
        end
    end
    local weights = pressure and { mill = 100, motor = 90, armor = 80, watch = 60, section = 50 }
        or { mill = 100, watch = 90, section = 80, motor = 60, armor = 50 }
    local best, score
    for _, id in ipairs({ "mill", "watch", "section", "motor", "armor" }) do
        local job = game.jobs[id]
        if
            not a[id]
            and (not job.needs or a[job.needs])
            and not a.queues[job.bay]
            and a.staffed
            and a.alloy >= job.cost
            and (not score or weights[id] > score)
        then
            best, score = id, weights[id]
        end
    end
    if best then
        order(s, a, best)
    end
    if a.section and s.relief_at and s.tick >= s.relief_at then
        order(s, a, "advance")
    end
    if a.armor and a.readiness > 0 and s.tick >= 3600 then
        order(s, a, "guard")
    end
end
function game.step(s)
    if s.result ~= "active" then
        return
    end
    s.tick = s.tick + 1
    if not s.relief_at and (s.blue.walk >= 120 or s.tick >= 2760) then
        s.relief_at = math.min(3000, s.tick + 240)
        report(s, s.blue, "Lookouts report Compact relief forming at the north road.")
    end
    if s.tick % 60 == 0 then
        rival(s)
    end
    if game.army then game.army.grid(s,s.blue);game.army.grid(s,s.red) end
    shift(s, s.blue)
    shift(s, s.red)
    if game.army then
        game.army.tick(s)
    end
    local b, r = s.blue, s.red
    if b.drive == 210 and r.drive == 210 then
        r.readiness = r.readiness - 1
        if r.readiness == 0 then
            r.guard = false
            report(s, b, "Rival carrier withdrawing from the yard.")
        end
    end
    -- A friendly armored crew can clear an unsupported opposing section.
    if b.drive == 210 and r.walk == 300 and r.drive < 210 then
        r.advance = false
        r.walk = 0
    end
    if r.drive == 210 and b.walk == 300 and b.drive < 210 then
        b.advance = false
        b.walk = 0
    end
    local forces = game.tactics and game.tactics.yard(s)
    local function claim(a, other)
        local own = (a == b and "home" or "rival")
        local foe = (a == b and "rival" or "home")
        local taking, blocked = a.walk == 300, other.walk == 300 or other.drive == 210
        if forces then
            taking, blocked = forces[own].occupy, forces[foe].occupy or forces[foe].guard
        end
        if taking and not blocked then
            a.hold = a.hold + 1
        else
            a.hold = 0
        end
    end
    local before = b.hold
    claim(b, r)
    claim(r, b)
    if before > 0 and b.hold == 0 then
        report(s, b, "Switch room hold interrupted. Lookouts report a contested yard.")
    end
    if b.hold >= game.hold_ticks then
        s.result = "victory"
        s.report = "Gate Six secured. The road is open."
    end
    if r.hold >= game.hold_ticks then
        s.result = "defeat"
        s.report = "Compact watch controls Gate Six."
    end
    if s.result == "active" then
        handover(s)
    else
        b.pending = nil
    end
end
return game
