-- All costs, queues, strategic decisions and victory rules live here.
local game = {}
game.hz, game.hold_ticks = 30, 1800
game.jobs = {
    mill    = {cost=4, duration=120, draw=2, bay="fit", label="Plate Mill"},
    watch   = {cost=3, duration=90, draw=1, bay="fit", label="Watch House", needs="mill"},
    motor   = {cost=6, duration=150, draw=3, bay="fit", label="Motor Hall", needs="mill"},
    section = {cost=2, duration=90, bay="watch", label="Line section", needs="watch"},
    armor   = {cost=4, duration=150, bay="motor", label="Armored crew", needs="motor"}
}
game.actions = {
    {"harvest","Cycle salvage hauler"}, {"mill","Commission Plate Mill"}, {"watch","Build Watch House"},
    {"motor","Build Motor Hall"}, {"section","Muster line infantry"}, {"armor","Produce armor"},
    {"advance","Send section to gate"}, {"guard","Send armor to yard"}, {"power","Switch factory power"},
    {"recall","Withdraw field crews"}
}
local function side()
    return {alloy=8, loads=8, haul=0, harvesting=false, stock=0, draw=0, power=true,
            mill=false, watch=false, motor=false, section=false, armor=false, queues={},
            rollout=0, advance=false, guard=false, walk=0, drive=0, hold=0, readiness=180}
end
function game.new()
    return {blue=side(), red=side(), tick=0, result="active", report="Survey ready. Commission a Plate Mill."}
end
local function report(s, a, msg)
    if a == s.blue then s.report = msg end
end
local function order(s,a,id)
    if s.result ~= "active" then return false end
    local job=game.jobs[id]
    if job then
        if a[id] then report(s,a,"Already commissioned: "..job.label) return false end
        if job.needs and not a[job.needs] then report(s,a,"Required first: "..game.jobs[job.needs].label) return false end
        if a.queues[job.bay] then report(s,a,"Crew occupied. Wait for the current job.") return false end
        if a.alloy < job.cost then report(s,a,"More alloy needed. Dispatch the hauler.") return false end
        a.alloy=a.alloy-job.cost
        a.queues[job.bay]={id=id,left=job.duration}
        report(s,a,job.label.." queued. Crew assigned.")
    elseif id=="harvest" then
        a.harvesting=not a.harvesting
        report(s,a,a.harvesting and "Mole dispatched to the steel yard." or "Mole holding its present load.")
    elseif id=="power" then
        a.power=not a.power
        report(s,a,a.power and "Factory grid restored." or "Factory grid isolated. Production paused.")
    elseif id=="advance" or id=="guard" then
        local unit=id=="advance" and "section" or "armor"
        if not a[unit] then report(s,a,"Crew unavailable. Produce the unit first.") return false end
        a[id]=true
        report(s,a,id=="advance" and "Gate Section moving to the switch room." or "Lintel moving to the gate yard.")
    elseif id=="recall" then
        a.advance,a.guard=false,false
        report(s,a,"Field crews returning to the yard.")
    else return false end
    return true
end
function game.command(s,id) return order(s,s.blue,id) end
local function shift(s,a)
    for _,bay in ipairs({"fit","watch","motor"}) do
        local q=a.queues[bay]
        if q and (bay=="fit" or a.power) then
            q.left=q.left-1
            if q.left==0 then
                local job=game.jobs[q.id]
                a[q.id]=true
                a.draw=a.draw+(job.draw or 0)
                a.queues[bay]=nil
                report(s,a,job.label.." ready for orders.")
            end
        end
    end
    if a.harvesting and (a.loads>0 or a.haul>0) then
        if a.haul<299 then
            a.haul=a.haul+1
            if a.haul==150 then a.loads=a.loads-1; a.stock=1 end
        elseif a.mill and a.power then
            a.haul,a.stock,a.alloy=0,0,a.alloy+4
            report(s,a,"Mole to dispatch. Four alloy delivered.")
        end
    end
    a.walk=math.max(0,math.min(300,a.walk+(a.advance and 1 or -1)))
    if a.armor then a.rollout=math.min(120,a.rollout+1) end
    a.drive=math.max(0,math.min(210,a.drive+(a.guard and 1 or -1)))
end
local function rival(s)
    local a=s.red
    if not a.harvesting then order(s,a,"harvest") end
    if not a.mill then order(s,a,"mill")
    elseif not a.watch then order(s,a,"watch")
    elseif not a.section then order(s,a,"section")
    elseif not a.motor then order(s,a,"motor")
    elseif not a.armor then order(s,a,"armor") end
    if a.section and s.tick>=3000 then order(s,a,"advance") end
    if a.armor and a.readiness>0 and s.tick>=3600 then order(s,a,"guard") end
end
function game.step(s)
    if s.result~="active" then return end
    s.tick=s.tick+1
    if s.tick%60==0 then rival(s) end
    shift(s,s.blue); shift(s,s.red)
    local b,r=s.blue,s.red
    if b.drive==210 and r.drive==210 then
        r.readiness=r.readiness-1
        if r.readiness==0 then r.guard=false; report(s,b,"Rival carrier withdrawing from the yard.") end
    end
    -- A friendly armored crew can clear an unsupported opposing section.
    if b.drive==210 and r.walk==300 and r.drive<210 then r.advance=false; r.walk=0 end
    if r.drive==210 and b.walk==300 and b.drive<210 then b.advance=false; b.walk=0 end
    local function claim(a,other)
        if a.walk==300 and other.walk<300 and other.drive<210 then a.hold=a.hold+1 else a.hold=0 end
    end
    claim(b,r); claim(r,b)
    if b.hold>=game.hold_ticks then s.result="victory"; s.report="Gate Six secured. The road is open." end
    if r.hold>=game.hold_ticks then s.result="defeat"; s.report="Compact watch controls Gate Six." end
end
return game
