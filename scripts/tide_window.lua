-- All time values are elapsed simulation ticks. The published harbor timetable uses seconds.
return function(ctx)
    local H=30
    local rules={hz=H,hold_ticks=720*H,deadline=720*H}
    rules.actions={{"hoist","Commission quay hoist / 6"},{"ramp","Fit extended ramp / 6"},
        {"load","Load cores by crawler"},{"quay","Sail to public quay"},{"slip","Sail to gravel slip"},
        {"install","Assign relay technicians"},{"cancel","Cancel fitting reservation"},
        {"hold","Hold ferry at home"},{"report","Request pilot report"}}
    local function radio(s,text) s.report=text;s.report_at=s.tick end
    function rules.new()
        return {tick=0,result="active",blue={alloy=8,queues={},power=true},red={queues={}},frames={},
            stage="loading",cargo=0,installed=0,elapsed=0,reserved=0,report_at=0,
            report="Depot watch. Two cores ashore. Choose hoist, ramp or crawler loading.",
            objective="Load two cores and choose a landing",clock=rules.deadline}
    end
    function rules.command(s,id)
        if s.result~="active" then return false end
        if id=="hoist" or id=="ramp" then
            if s.gear or s.loading or s.stage~="loading" or s.blue.alloy<6 then
                radio(s,"Fitters unavailable or alloy committed.");return false
            end
            s.gear=id;s.reserved=6;s.blue.alloy=s.blue.alloy-6;s.loading=s.tick
            radio(s,"Depot fitters reserved. Six alloy committed.")
        elseif id=="cancel" then
            if s.reserved==0 then radio(s,"No unstarted fitting reservation.");return false end
            s.blue.alloy=s.blue.alloy+s.reserved;s.reserved=0;s.gear=nil;s.loading=nil
            if s.destination=="slip" then s.destination=nil end
            radio(s,"Fitters released. Alloy returned.")
        elseif id=="load" then
            if s.loading or s.stage~="loading" then return false end
            s.loading=s.tick;radio(s,"Crawler crew loading both cores.")
        elseif id=="quay" or id=="slip" then
            if id=="slip" and s.gear~="ramp" then radio(s,"Gravel slip requires the extended ramp.");return false end
            if s.destination==id or s.stage=="discharge" or s.stage=="island" then return false end
            if s.stage=="crossing" or s.stage=="berth" then
                local frame=s.frames[ctx.index.kestrel] or {0,0,0,0}
                local remaining=math.max(24*H,(s.sailing or 135*H)-(s.tick-s.since))
                s.origin={frame[1],frame[3],frame[4]};s.since=s.tick;s.sailing=remaining;s.stage="crossing"
            end
            s.destination=id;radio(s,"Pilot acknowledges "..id.." landing.")
        elseif id=="hold" then
            if s.stage~="loading" or not s.destination then return false end
            s.destination=nil;radio(s,"Pilot holding at home.")
        elseif id=="install" then
            if s.technicians then return false end
            s.technicians=true;radio(s,"Relay technicians assigned to both cores.")
        elseif id=="report" then radio(s,"Pilot: "..s.stage..". Cores aboard "..s.cargo..".")
        else return false end
        return true
    end
    local function frame(s,key,x,y,z,yaw,parent)
        s.frames[ctx.index[key]]={x,y,z,yaw or 0,parent and ctx.index[parent] or 0}
    end
    function rules.step(s)
        if s.result~="active" then return end
        s.tick=s.tick+1;s.clock=rules.deadline-s.tick
        if s.clock<=0 then s.result="defeat";radio(s,"High tide. The island relay is offline.");return end
        local now=s.tick
        local rival=math.max(0,math.min(1,(now/H-180)/120,(540-now/H)/60))
        frame(s,"rival-ferry",78-78*rival,0,-156-74*rival,math.pi)
        if s.stage=="loading" and s.loading then
            s.reserved=0
            local duration=s.gear=="hoist" and 95 or 160
            local t=(now-s.loading)/H
            s.cargo=t>=duration and 2 or (t>=duration/2 and 1 or 0)
            if s.cargo<2 then
                local trip=math.sin(math.pi*math.min(1,t/duration)*4)
                frame(s,"mole",21,0,55-trip*12)
            elseif s.destination then
                s.stage="embark";s.since=now;s.objective="Ride the crossing or direct it from shore"
                radio(s,"Both cores secured. Crawler rolling onto the deck.")
            end
        elseif s.stage=="embark" then
            local t=math.min(1,(now-s.since)/(25*H))
            frame(s,"mole",21*(1-t),.84*t,55-39*t)
            if t==1 then
                frame(s,"mole",0,.84,16,0,"kestrel");s.stage="crossing";s.since=now
                radio(s,"Kestrel underway. Vehicle lashings checked.")
            end
        elseif s.stage=="crossing" or s.stage=="berth" then
            local t=math.min(1,(now-s.since)/(s.sailing or 135*H))
            local origin=s.origin or {0,0,0}
            local lateral=s.destination=="slip" and 55 or 0
            frame(s,"kestrel",origin[1]*(1-t)+lateral*t+8*math.sin(t*math.pi),0,origin[2]*(1-t)-230*t,origin[3]*(1-t)+.14*math.sin(t*2*math.pi))
            if t==1 then
                s.stage="berth"
                local free=s.destination=="slip" or now+30*H<=300*H or now>=480*H
                if free then
                    s.stage="discharge";s.since=now;s.landing=s.destination
                    radio(s,"Berth clear. Pilot lowering the ramp.")
                elseif not s.warned then
                    s.warned=true;radio(s,"Quay reserved to 08:00. Hold or redirect a fitted ferry to the slip.")
                end
            end
        elseif s.stage=="discharge" then
            local t=math.min(1,(now-s.since)/(30*H))
            local x=s.landing=="slip" and 55 or 0
            frame(s,"mole",x+17*t,.84*(1-t),-214+38*t)
            if t==1 then
                s.stage="island";s.since=now;s.cargo=0;s.objective="Assign relay technicians to install both cores"
                radio(s,"Both cores ashore. Relay crew standing by.");ctx.emit("berth",s,s.landing)
            end
        elseif s.stage=="island" then
            if not s.technicians then s.since=now;return end
            local length=(s.landing=="slip" and 225 or 150)*H
            local t=math.min(1,(now-s.since)/length)
            local x=s.landing=="slip" and 72 or 17
            frame(s,"mole",x+(-30-x)*t,0,-176-97*t)
            s.installed=math.floor(t*2)
            if t==1 then s.result="victory";radio(s,"Island relay online. Both cores installed.") end
        end
        s.progress=s.installed
    end
    return rules
end
