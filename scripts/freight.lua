-- Cargo changes parent frames at the receiving crew's handover.
local manifests = {
    westbound = { freight = { "winter-freight", "shunter", "exchange" } },
    ["white-mile"] = {
        sample = { "salt-sample", "barrow", "barrow" },
        ["assay-load"] = { "salt-sample", "barrow", "assay" },
    },
    ["night-mail"] = {
        ["mail-1"] = { "bag-one", "tern", "beacon" },
        ["mail-2"] = { "bag-two", "kite", "beacon" },
        ["mail-3"] = { "bag-three", "kite", "beacon" },
    },
    ["high-water"] = {
        module = { "pressure-module", "dray", "lark" },
        transfer = { "pressure-module", "spindle", "station" },
    },
}
local blockers = {
    westbound = "siding",
    ["white-mile"] = "pump",
    ["night-mail"] = "flight",
    ["high-water"] = "cradle",
    ["compact-bank"] = "mooring",
}
return function(ctx)
    local M = {}
    local entries = manifests[ctx.sector] or {}
    local checkpoint = blockers[ctx.sector]
    function M.new(s)
        s.freight = { items = {}, closed = false, deliveries = {} }
        for _, item in pairs(entries) do
            s.freight.items[item[1]] = s.freight.items[item[1]] or "stores"
        end
    end
    function M.control(s)
        if not checkpoint then
            return false
        end
        s.freight.closed = not s.freight.closed
        s.report = checkpoint .. (s.freight.closed and " closed to traffic." or " reopened to traffic.")
        return true
    end
    function M.waits(s, key)
        return s.freight.closed and (entries[key] or key == "cross" or key == "tow" or key == "dock")
    end
    local function attach(s, item, parent, slot)
        s.frames[ctx.index[item]] = { slot or 0, 0.88, 1, 0, ctx.index[parent] }
        s.instances[item].parent = parent
        s.freight.items[item] = parent
    end
    function M.take(s, key)
        local entry = entries[key]
        if entry then
            attach(s, entry[1], entry[2])
        end
    end
    -- Unload at the actual receiving door or loading apron, in the receiver's frame.
    local berths = {
        freight = { -3, 0, 12 },
        ["assay-load"] = { 2, 0, 8 },
        ["mail-1"] = { -9, 0, 13 },
        ["mail-2"] = { -36, 0, 12 },
        ["mail-3"] = { -4, 0, 7 },
        module = { -2, 0, 45 },
        transfer = { 6, 0, 353 },
    }
    function M.deliver(s, key)
        local entry = entries[key]
        if not entry then
            return true
        end
        if s.freight.items[entry[1]] ~= entry[2] then
            return false
        end
        local p, port, berth = ctx.position(s, entry[2]), ctx.position(s, entry[3]), berths[key]
        if berth then
            local dx, dy, dz = p[1] - port[1] - berth[1], p[2] - port[2] - berth[2], p[3] - port[3] - berth[3]
            if dx * dx + dy * dy + dz * dz > 25 then
                s.report = entry[2] .. " waiting for " .. entry[3] .. " to meet the handover."
                return false
            end
        end
        attach(s, entry[1], entry[3], -(#key % 3))
        s.freight.deliveries[key] = entry[1]
        return true
    end
    function M.goal(s)
        for key, entry in pairs(entries) do
            if s.freight.deliveries[key] ~= entry[1] then
                return false
            end
        end
        if ctx.sector == "compact-bank" then
            local crew = ctx.position(s, "channel-section")
            local shed = ctx.position(s, "shed")
            return crew[3] < -95 and shed[3] < -95 and #s.instances.shed.crew > 0
        end
        return true
    end
    M.checkpoint = checkpoint
    return M
end
