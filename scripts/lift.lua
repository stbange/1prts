-- Crown's two landing interlocks run on the same fixed clock as the dock crew.
return function(ctx)
    local M = {}
    local function frame(s)
        local l = s.lift
        local car = ctx.index["traffic-lift"]
        s.frames[car] = { -4, 0.84 + l.height, 326, 0, ctx.index.station }
        for level = 0, 1 do
            for side = 1, 2 do
                local gap = l.height == level * 6 and not l.moving and 1.9 or 0.76
                local i = ctx.index["lift-door-" .. level .. "-" .. side]
                s.frames[i] = { -4 + (2 * side - 3) * gap, 0.84 + 6 * level, 327.88, 0, ctx.index.station }
            end
        end
    end
    function M.new(s)
        if ctx.index["traffic-lift"] then
            s.lift = { height = 0, target = 0, moving = false }
            frame(s)
        end
    end
    function M.call(s, level)
        local lift = s.lift
        if not lift or lift.moving or not (level == 0 or level == 1) or lift.height == level * 6 then
            return false
        end
        lift.moving = true
        lift.target = level * 6
        s.report = "Crown lift / doors closing."
        frame(s)
        return true
    end
    function M.tick(s)
        local lift = s.lift
        if not lift or not lift.moving then
            return
        end
        local delta = lift.target - lift.height
        lift.height = lift.height + math.max(-0.05, math.min(0.05, delta))
        if math.abs(lift.target - lift.height) < 0.00001 then
            lift.height = lift.target
            lift.moving = false
            s.report = lift.height == 0 and "Crown lift / collar deck." or "Crown lift / traffic control."
        end
        frame(s)
    end
    return M
end
