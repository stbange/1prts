-- A marker binds a crew and a surveyed location to an ordinary operation order.
local points = {
    ["gate-six"] = {
        { "west-pad", "fitters", "build:motor@west-pad" },
        { "east-pad", "fitters", "build:watch@east-pad" },
        { "bank-pad", "fitters", "build:dynamo@bank-pad" },
        { "common-lot", "mole", "common" },
    },
    ["tide-window"] = {
        { "public-quay", "kestrel", "quay", { -8, 0, -168 }, "Public quay" },
        { "gravel-slip", "kestrel", "slip", { 55, 0, -170 }, "Gravel slip" },
    },
    westbound = { { "exchange-switch", "points", "switch", { 2, 0, -156 }, "Exchange points" } },
    ["high-water"] = { { "crown-collar", "lark", "dock", { 0, 84, -267 }, "Crown docking collar" } },
}
return function(ctx, custom)
    local actions = {}
    for _, entry in ipairs(custom or points[ctx.sector] or {}) do
        local key, actor, order, pos = table.unpack(entry)
        assert(ctx.index[actor] and type(order) == "string", "unknown marker crew or order")
        if pos then
            assert(not ctx.index[key], "duplicate marker ID")
            local row = {
                "survey-pad",
                pos[1],
                pos[2],
                pos[3],
                0,
                key = key,
                name = entry[5] or key,
                type = "marker",
                family = "marker",
                owner = "neutral",
            }
            ctx.site[#ctx.site + 1] = row
            ctx.index[key] = #ctx.site
        end
        assert(ctx.index[key], "missing dispatch marker")
        actions[actor .. "@" .. key] = order
    end
    return actions
end
