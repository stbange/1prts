-- Run against the same mission module the C engine loads. All clocks are simulation ticks.
local root=assert(arg[1],"mission directory")
local rules=assert(loadfile(root.."/scripts/runtime.lua"))(root,"gate-six").gate_six
local episodes = tonumber(arg[2]) or 20
local first_seed = tonumber(arg[3]) or 42
local trace = arg[4] and assert(io.open(arg[4], "w"))
local hz = rules.hz
local function row(seed, bot, tick, event, ...)
    io.write(table.concat({seed, bot, tick, event, ...}, ","), "\n")
end
local function rng(seed)
    return function(lo, hi)
        seed = (48271 * seed) % 2147483647
        return lo + math.floor((hi-lo+1) * seed / 2147483647)
    end
end
local function offer(s, id)
    local b, job = s.blue, rules.jobs[id]
    if not job then
        if id == "advance" then return b.section and not b.advance end
        if id == "guard" then return b.armor and not b.guard end
        if id == "recall" then return b.advance or b.guard end
        return true
    end
    return not b[id] and (not job.needs or b[job.needs]) and not b.queues[job.bay] and b.alloy >= job.cost
end
local infantry = {"mill","harvest","watch","section","advance","motor","armor","guard"}
local armor = {"harvest","mill","motor","armor","guard","watch","section","advance"}
local function choose(s, plan)
    for _,id in ipairs(plan) do
        if id == "harvest" then
            if not s.blue.harvesting then return id end
        elseif offer(s,id) then
            return id
        end
    end
end
local function run(seed, bot)
    local random = rng(seed)
    -- Burn the first correlated low bits before choosing a route.
    for _=1,3 do random(0,1000) end
    local s = rules.new()
    local plan = infantry
    if bot == "human" and random(1,2)==1 then plan=armor end
    if bot == "infantry" then plan={"mill","harvest","watch","section","advance"} end
    local check_at = bot=="human" and random(0,8*hz) or 0
    local pending, due, input_count = nil, 0, 0
    local progress, first_claim = 0, false
    local function dispatch(id)
        local available={}
        if trace and bot=="human" and seed==first_seed then
            for _,a in ipairs(rules.actions) do if offer(s,a[1]) then available[#available+1]=a[1] end end
        end
        local before_advance, before_guard = s.blue.advance, s.blue.guard
        local accepted=rules.command(s,id)
        local changed=accepted and not ((id=="advance" and before_advance) or (id=="guard" and before_guard)
                            or (id=="recall" and not before_advance and not before_guard))
        row(seed,bot,s.tick,"input",id,accepted and 1 or 0,changed and 1 or 0)
        if #available>0 then trace:write("order,",s.tick,",",id,",",table.concat(available,"|"),"\n") end
        input_count=input_count+1
    end
    while s.result=="active" and s.tick<240*hz do
        if pending and s.tick>=due then dispatch(pending); pending=nil end
        if not pending and s.tick>=check_at then
            if bot=="novice" or bot=="spam" then
                dispatch(bot=="spam" and "advance" or rules.actions[random(1,#rules.actions)][1])
                check_at=s.tick+random(8,15)
            elseif bot=="human" then
                pending=choose(s,plan)
                due=s.tick+math.floor((200+random(-35,35))*hz/1000+.5)
                -- Glance at the slate during a field visit, then explore or watch the crews.
                check_at=due+random(2*hz,7*hz)
            else
                local id=choose(s,plan)
                if id then dispatch(id) end
                check_at=s.tick+1
            end
        end
        local b,r=s.blue,s.red
        local old_alloy,old_hold,old_report=b.alloy,b.hold,s.report
        local old_flags=(b.mill and 1 or 0)+(b.watch and 1 or 0)+(b.motor and 1 or 0)
                        +(b.section and 1 or 0)+(b.armor and 1 or 0)
        local warned,approach=s.relief_at,r.walk
        rules.step(s)
        if s.relief_at and not warned then row(seed,bot,s.tick,"warning",s.relief_at) end
        if approach==0 and r.walk>0 then row(seed,bot,s.tick,"approach") end
        if r.walk==300 and approach<300 then row(seed,bot,s.tick,"enemy_room") end
        local flags=(b.mill and 1 or 0)+(b.watch and 1 or 0)+(b.motor and 1 or 0)
                   +(b.section and 1 or 0)+(b.armor and 1 or 0)
        if b.alloy>old_alloy or flags>old_flags or (b.hold>0 and b.hold%hz==0) then
            row(seed,bot,s.tick,"reward",b.alloy-old_alloy)
        end
        if not first_claim and b.hold>0 then first_claim=true; row(seed,bot,s.tick,"claim") end
        if old_hold>0 and b.hold==0 then row(seed,bot,s.tick,"reset",old_hold) end
        progress=math.max(progress,b.hold)
        if s.tick%hz==0 then
            row(seed,bot,s.tick,"second",r.walk/300,r.drive/210,r.hold/rules.hold_ticks,input_count,b.hold)
            input_count=0
        end
        if trace and bot=="human" and seed==first_seed then
            trace:write(table.concat({"tick",s.tick,b.alloy,b.stock,b.walk,b.drive,b.hold,r.walk,r.drive,r.hold,
                s.report~=old_report and 1 or 0},","),"\n")
        end
    end
    row(seed,bot,s.tick,"end",s.result,progress/rules.hold_ticks)
end
for i=0,episodes-1 do
    for _,bot in ipairs({"novice","human","oracle","spam","infantry"}) do run(first_seed+i,bot) end
end
-- Human-timing probes. Land a legal follow-on just before a fitting bay or a prerequisite opens.
for _,id in ipairs({"watch","section"}) do
    for early=1,3 do
        local s=rules.new()
        assert(rules.command(s,"mill"))
        -- A second crew needs alloy from a normal delivery.
        if id=="section" then
            assert(rules.command(s,"harvest"))
            for _=1,300 do rules.step(s) end
            assert(rules.command(s,"watch"))
        end
        local duration=id=="watch" and 120 or 90
        for _=1,duration-early do rules.step(s) end
        local at=s.tick
        rules.command(s,id)
        for _=1,early+1 do rules.step(s) end
        local bay=rules.jobs[id].bay
        row(first_seed,"timing",at,"buffer",id,early,s.blue.queues[bay] and s.blue.queues[bay].id==id and 1 or 0)
    end
end
if trace then trace:close() end
