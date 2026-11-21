local api = ...
api.unit({
    id = "quay-courier",
    name = "Wire Courier",
    role = "freight",
    family = "truck",
    owner = "weir",
    w = 3.2,
    h = 3.5,
    d = 12,
    asset = "courier.mesh",
    interior = "porter",
    work = "cargo",
})
api.sector({
    id = "wire-yard",
    owner = "weir",
    terrain = "railbed",
    units = "quay-courier fitters kestrel",
    buildings = "depot stores",
    positions = { kestrel = { 80, 0, -80 }, ["quay-courier"] = { 0, 0, 0 }, depot = { -30, 0, -12 }, stores = {
        12,
        0,
        -74,
    } },
    spawn = { 0, 0, 9, 0 },
    routes = { road = { { 0, 8 }, { 0, -26 }, { 8, -54 }, { 12, -66 } } },
})
local function shift(target)
    return function(ctx)
        return {
            hz = 30,
            hold_ticks = 5400,
            actions = { { "waybill", "Stamp freight receipt", "quay-courier" } },
            new = function()
                return {
                    tick = 0,
                    result = "active",
                    blue = { alloy = 8, queues = {} },
                    red = { queues = {} },
                    objective = "Dispatch courier freight to " .. target,
                    report = "The quay is open. Use the courier's freight assignment.",
                }
            end,
            command = function(s, id)
                if
                    id ~= "waybill"
                    or not s.works.receipts["quay-courier"]
                    or s.works.receipts["quay-courier"].target ~= target
                then
                    s.report = "No delivery receipt from " .. target .. "."
                    return false
                end
                s.result = "victory"
                s.report = "Signed cargo received at " .. target .. "."
                return true
            end,
            step = function(s)
                s.tick = s.tick + 1
                if s.tick >= 5400 then
                    s.result = "defeat"
                    s.report = "Quay shift ended."
                end
            end,
        }
    end
end
api.job("gravel-call", "Land the fitted Kestrel at gravel", function(s, ctx)
    if ctx.sector ~= "wire-yard" or not s.inherited_ramp or s.gravel then
        return false
    end
    s.gravel = true
    s.frames[ctx.index.kestrel] = { 50, 0, -120, 0, 0, 1, 5 }
    s.report = "Kestrel extended ramp open at the gravel landing."
    return true
end)
api.link("high-water", "wire-quay")
api.mission({
    id = "wire-quay",
    name = "Wire Quay",
    sector = "wire-yard",
    owner = "weir",
    next = "wire-return",
    build = shift("stores"),
})
api.mission({ id = "wire-return", name = "Wire Quay / Return", sector = "wire-yard", owner = "weir", build = shift(
    "depot"
) })
