-- Save documents contain data only. Numeric scene rows are converted to stable
-- call signs at the file boundary so nested passengers survive scene rebuilding.
return function(ctx, rules, records, decode, mission, initial)
    local M = {}
    local function copy(t)
        local out = {}
        for k, v in pairs(t) do
            out[k] = v
        end
        return out
    end
    local function canonical(s)
        local state = copy(s)
        state.frames = {}
        for i, frame in pairs(s.frames) do
            local f = copy(frame)
            f[5] = f[5] and f[5] > 0 and ctx.site[f[5]].key or false
            state.frames[ctx.site[i].key] = f
        end
        return state
    end
    function M.save(s)
        return records.encode({ format = 1, mission = mission.id, packs = ctx.packages, state = canonical(s) })
    end
    function M.restore(text)
        local file = records.decode(text, decode)
        assert(file.format == 1 and file.mission == mission.id, "incompatible save or operation")
        assert(records.encode(file.packs) == records.encode(ctx.packages), "installed pack versions differ")
        local s = file.state
        assert(
            type(s) == "table"
                and math.type(s.tick) == "integer"
                and s.tick >= 0
                and (s.result == "active" or s.result == "victory" or s.result == "defeat"),
            "invalid mission clock"
        )
        assert(
            type(s.blue.queues) == "table"
                and type(s.red.queues) == "table"
                and type(s.tactical.orders) == "table"
                and type(s.campaign.history) == "table"
                and type(s.campaign.hulls) == "table"
                and type(s.freight.items) == "table"
                and type(s.works.receipts) == "table"
                and math.type(s.seed) == "integer"
                and math.type(s.random_state) == "integer"
                and type(s.frames) == "table",
            "incomplete saved state"
        )
        ctx.truncate(initial)
        for _, v in ipairs(s.spawned) do
            assert(not ctx.index[v.key], "duplicate saved instance")
            if v.unit == "ground" then
                assert(ctx.ground(v.key), "invalid saved point")
            else
                assert(ctx.fleet[v.unit] and type(v.pos) == "table" and #v.pos >= 3, "missing saved asset")
                ctx.add_entity(v.key, v.unit, v.pos)
            end
        end
        for _, row in ipairs(ctx.site) do
            local inst = s.instances[row.key]
            assert(
                inst and inst.type == row.type and inst.owner == row.owner,
                "missing or changed instance " .. row.key
            )
            for _, seat in ipairs(inst.crew) do
                assert(ctx.index[seat], "missing crew " .. seat)
            end
            assert(not inst.parent or ctx.index[inst.parent], "missing saved parent")
        end
        for key in pairs(s.instances) do
            assert(ctx.index[key], "unknown saved instance")
        end
        local frames = {}
        for key, frame in pairs(s.frames) do
            local i = assert(ctx.index[key], "missing frame")
            local f = copy(frame)
            for n = 1, 4 do
                assert(type(f[n]) == "number", "invalid frame pose")
            end
            f[5] = f[5] and assert(ctx.index[f[5]], "missing moving parent") or 0
            frames[i] = f
        end
        s.frames = frames
        for i in pairs(frames) do
            local seen = {}
            local node = i
            while node and node > 0 do
                assert(not seen[node], "cyclic moving frame")
                seen[node] = true
                node = frames[node] and frames[node][5] or ctx.site[node].parent
            end
        end
        for _, jobs in ipairs({ s.tactical.orders, s.tactical.posted, s.service, s.contacts }) do
            for actor, job in pairs(jobs) do
                assert(ctx.index[actor] and ctx.index[job.target], "missing order target")
            end
        end
        for _, work in pairs(s.jobs or {}) do
            assert(ctx.index[work.actor] and type(work.elapsed) == "number", "invalid queued crew")
        end
        for item, carrier in pairs(s.freight.items) do
            assert(
                ctx.index[item] and (carrier == "stores" or ctx.index[carrier]),
                "unknown cargo or receiver"
            )
        end
        if s.army then
            for _, job in pairs(s.army.bays) do
                assert(rules.jobs[job.id] and (not job.pad or ctx.index[job.pad]), "unknown production order")
            end
        end
        return s
    end
    local next_base = {
        ["gate-six"] = "tide-window",
        ["tide-window"] = "westbound",
        westbound = "white-mile",
        ["white-mile"] = "night-mail",
        ["night-mail"] = "high-water",
    }
    function M.new(s)
        s.seed, s.random_state = 614, 614
        s.campaign = { history = {}, hulls = {}, upgrades = {}, source = "" }
        s.entry_record = records.encode(s.campaign)
    end
    function M.finish(s)
        if s.result ~= "victory" then
            return
        end
        local ledger = s.campaign
        if ledger.history[mission.id] then
            return
        end
        ledger.history[mission.id] = { tick = s.tick, faction = ctx.owner }
        for _, key in ipairs({ "mole", "kestrel", "wake" }) do
            local inst = s.instances[key]
            if inst and inst.owner == ctx.owner and inst.status ~= "disabled" then
                ledger.hulls[ctx.owner .. "/" .. key] = {
                    key = key,
                    type = inst.type,
                    owner = inst.owner,
                    crew = copy(inst.veterans or inst.crew),
                    status = inst.status,
                }
            end
        end
        if s.gear then
            ledger.upgrades[s.gear == "hoist" and "weir/depot" or "weir/kestrel"] = s.gear
        end
        for _, gear in ipairs({ "heater", "shoes", "board", "collar" }) do
            if s.done and s.done[gear] then
                ledger.upgrades[ctx.owner .. "/" .. gear] = gear
            end
        end
    end
    function M.ledger(s)
        M.finish(s)
        return records.encode(s.campaign)
    end
    function M.entry(s)
        return s.entry_record or records.encode({ history = {}, hulls = {}, upgrades = {}, source = "" })
    end
    function M.enter(s, text)
        local ledger = records.decode(text, decode)
        assert(
            type(ledger.history) == "table"
                and type(ledger.upgrades) == "table"
                and type(ledger.hulls) == "table",
            "invalid campaign record"
        )
        s.entry_record, s.campaign = text, ledger
        local function transfer(key)
            local item = ledger.hulls[ctx.owner .. "/" .. key]
            local inst = s.instances[key]
            if not item or not inst or item.type ~= inst.type or item.owner ~= ctx.owner then
                return
            end
            -- A familiar hull can have different operator row numbers at this port.
            -- Keep the source watch's identities as the names on its assigned seats.
            inst.veterans = copy(item.crew)
            for i, seat in ipairs(inst.crew) do
                s.instances[seat].person = item.crew[i] or seat
            end
            inst.status = item.status
            s.report = key .. " and its watch have reported from the previous operation."
        end
        transfer("mole")
        transfer("kestrel")
        if mission.id == "night-mail" then
            s.linked_hoist = ledger.upgrades["weir/depot"] == "hoist"
        end
        if ctx.index.kestrel and ledger.upgrades[ctx.owner .. "/kestrel"] == "ramp" then
            s.inherited_ramp = true
        end
        return true
    end
    function M.next(s)
        return s.result == "victory" and (mission.next or next_base[mission.id]) or ""
    end
    return M
end
