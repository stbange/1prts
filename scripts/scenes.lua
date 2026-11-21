local M={}
local homes={weir="WEIR ASSEMBLY",sounding="SOUNDING COMPACT",terrace="TERRACE RAILWAY",
             salt="SALT SURVEY",lantern="LANTERN PILOTS",meridian="MERIDIAN DOCK TRUST"}
local plans={
 ["gate-six"]={"weir","terrain", "porter sump tag dray plumb span tally reed hinge points barrow gimbal",
               "post depot pump stores infirmary signals slipway quarters siding mooring gallery shed"},
 ["compact-bank"]={"sounding","estuary","wake hinge reed channel-section crossbill fitters mole span",
                  "shed mooring slipway watch motor signals post"},
 ["tide-window"]={"weir","estuary","kestrel mole fitters tag porter reed","depot relay mooring customs slipway"},
 westbound={"terrace","railbed","shunter points anvil sleeper whistle dray tally fitters gate-section",
            "exchange roundhouse siding stores gallery survey post quarters dynamo"},
 ["white-mile"]={"salt","saltpan","barrow chain-crew glint prism shoe sump mole plumb tag",
                "assay pump infirmary relay stores signals mill"},
 ["night-mail"]={"lantern","airfield","kite tern rook crossbill gimbal gantry ground-watch porter",
                "flight tower beacon customs quarters depot"},
 ["high-water"]={"meridian","orbit","lark cleat spindle rivet loom collar-watch dray",
                "station spaceport cradle lock mooring beacon"}
}
local fixed={
 ["tide-window"]={kestrel={0,0,0},mole={21,0,55},depot={-33,0,73},relay={-30,0,-284},mooring={36,0,-264},customs={45,0,80}},
 westbound={shunter={0,0,-3},exchange={3,0,-165},roundhouse={-35,0,-25},siding={29,0,-80}},
 ["white-mile"]={barrow={0,0,-3},assay={-25,0,-136},pump={30,0,-63}},
 ["night-mail"]={kite={0,0,-10},flight={-43,0,-28},tower={46,0,-55},beacon={17,0,-165}},
 ["high-water"]={lark={0,0,0},spaceport={-45,0,65},station={0,84,-668},cradle={64,0,23}},
 ["compact-bank"]={wake={0,0,-3},shed={-25,0,-27},hinge={-27,0,5},mooring={28,0,-105}}
}
local spawn={["tide-window"]={0,0,50,0},westbound={0,0,13,0},["white-mile"]={0,0,13,0},
             ["night-mail"]={0,0,14,0},["high-water"]={1,0,45,0},["compact-bank"]={0,0,11,0}}
local facilities={hall=true,plant=true,works=true,tower=true,hangar=true,gate=true}
local people_models={fitters="worker",["gate-section"]="section",["channel-section"]="enemysection"}
function M.fleet(read)
    local fleet={}
    for line in read("assets/fleet.txt"):gmatch("[^\r\n]+") do
        if line:sub(1,1)~="#" then
            local id,f,w,h,d,owner,role=line:match("^(%S+)%s+(%S+)%s+([%d.]+)%s+([%d.]+)%s+([%d.]+)%s+(%S+)%s+(%S+)")
            assert(id,"invalid fleet definition: "..line)
            fleet[id]={family=f,w=tonumber(w),h=tonumber(h),d=tonumber(d),owner=owner,role=role}
        end
    end
    return fleet
end
function M.make(id,catalog,fleet,base_site)
    local plan=assert(plans[id],"unknown sector "..id)
    local gate=id=="gate-six"
    local scene=gate and base_site or {{plan[2],0,0,0,0}}
    local names={}
    for _,group in ipairs({catalog.units,catalog.buildings}) do
        for _,v in ipairs(group) do names[v.id]=v.name end
    end
    local index={}
    if gate then
        for i,key in ipairs({"terrain","gate","survey","dynamo","mill","watch","motor","mole","lintel","wake","gate-section","channel-section","fitters"}) do
            local row,def=scene[i],fleet[key]
            if def and key~="wake" and key~="channel-section" then
                row.key,row.role,row.name=key,def.role,names[key];index[key]=i
            end
        end
    end
    local function entity(key,unit,pos)
        local def=assert(fleet[unit],unit)
        assert(not index[key],"duplicate entity "..key)
        local model=def.model or people_models[unit] or unit
        local row={model,pos[1],pos[2],pos[3],pos[4] or 0}
        row.key,row.name,row.family,row.role=key,names[unit],def.family,def.role
        row.size={def.w,def.h,def.d};row.asset=def.asset
        row.hull=not facilities[def.family] and def.family~="people"
        row.station=def.family~="people"
        scene[#scene+1]=row; index[key]=#scene
        if row.station then
            for n,z in ipairs({-.29,.26}) do
                local worker={"worker",def.w*.23,row.hull and .84 or .08,def.d*z,0}
                worker.parent=#scene-(n-1);worker.key=key.."-"..n;worker.family="operator"
                worker.name=row.name..(n==1 and " / pilot" or " / watch")
                scene[#scene+1]=worker
            end
        end
    end
    local positions=fixed[id] or {}
    for group=3,4 do
        local j=0
        for unit in plan[group]:gmatch("%S+") do
            j=j+1
            local x=(group==3 and 1 or -1)*(gate and (83+((j-1)%2)*34) or (75+((j-1)%2)*37))
            local z=60-math.floor((j-1)/2)*37
            entity(unit,unit,positions[unit] or {x,0,z})
        end
    end
    if id=="tide-window" then entity("rival-ferry","kestrel",{78,0,-156}) end
    return {site=scene,index=index,add_entity=entity,terrain=plan[2],faction=homes[plan[1]],spawn=spawn[id] or {3,0,22,-.052}}
end
return M
