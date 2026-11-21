-- Players see only the slate, the objective text, acknowledgments and the mission clock.
local root = assert(arg[1])
local runs = tonumber(arg[2]) or 12
local runtime = assert(loadfile(root .. "/scripts/runtime.lua"))
local routes = {
    ["tide-window"] = "hoist quay install",
    ["compact-bank"] = "sound embark repair tow cross land restore",
    westbound = "survey heater guard track switch freight",
    ["white-mile"] = "scout shoes drain chart sample analyze assay-load",
    ["night-mail"] = "board beacon clear patrol mail-1 mail-2 escort mail-3",
    ["high-water"] = "module collar supply service lift dock transfer watch repair traffic",
}
print("mission,persona,seed,outcome,seconds,accepted,rejected,work")
for _, id in ipairs({ "tide-window", "compact-bank", "westbound", "white-mile", "night-mail", "high-water" }) do
    local rules = runtime(root, id).gate_six
    for _, persona in ipairs({ "newcomer", "dispatcher", "planner" }) do
        for seed = 1, runs do
            local random = seed * 7919
            local function die(n)
                random = (random * 48271) % 2147483647
                return 1 + math.floor(random * n / 2147483647)
            end
            local s, accepted, rejected, when = rules.new(), 0, 0, 0
            local issued = {}
            while s.result == "active" do
                if s.tick >= when then
                    local order
                    if persona == "planner" then
                        local pattern = routes[id]
                        if id == "tide-window" and seed % 3 == 0 then
                            pattern = "ramp slip install"
                        elseif id == "tide-window" and seed % 3 == 1 then
                            pattern = "load quay install"
                        end
                        for key in pattern:gmatch("%S+") do
                            if not issued[key] then
                                if rules.command(s, key) then
                                    issued[key] = true
                                    accepted = accepted + 1
                                else
                                    rejected = rejected + 1
                                end
                            end
                        end
                    else
                        if die(100) <= (persona == "dispatcher" and 86 or 42) then
                            order = s.advice
                            if not order then
                                for _, a in ipairs(rules.actions) do
                                    if a[2] == s.objective then
                                        order = a[1]
                                    end
                                end
                            end
                        end
                        order = order or rules.actions[die(#rules.actions)][1]
                        if rules.command(s, order) then
                            accepted = accepted + 1
                        else
                            rejected = rejected + 1
                        end
                    end
                    when = s.tick
                        + (
                            persona == "newcomer" and 90 + die(240)
                            or persona == "dispatcher" and 60 + die(120)
                            or 75
                        )
                end
                rules.step(s)
            end
            print(
                table.concat(
                    { id, persona, seed, s.result, s.tick / 30, accepted, rejected, s.progress or 0 },
                    ","
                )
            )
        end
    end
end
