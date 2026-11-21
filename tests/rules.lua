-- Count only first-party script lines reached by these isolated unit cases.
local covered = {}
local root = assert(data_root)
local function hook(_,line)
    local info=debug.getinfo(2,"S")
    local path=info.source:match("/scripts/(.+%.lua)$") or info.source:match("/(mods/.+%.lua)$")
    if path then covered[path]=covered[path] or {}; covered[path][line]=true end
end
debug.sethook(hook,"l")
local C=dofile(root.."/scripts/catalog.lua")
local site=dofile(root.."/scripts/site.lua")
assert(#site>10)
local sizes={units=39,buildings=30,factions=6,missions=6,upgrades=6}
for name,count in pairs(sizes) do
    assert(#C[name]==count,name)
    local seen={}
    for _,row in ipairs(C[name]) do assert(not seen[row.id] and #row.role>5);seen[row.id]=true end
    if C.theater[name] then
        assert(#C.theater[name]==18);local localset={}
        for _,id in ipairs(C.theater[name]) do assert(seen[id] and not localset[id]);localset[id]=true end
    end
end
local G=dofile(root.."/scripts/gate_six.lua")
local function step(s,n) for _=1,n do G.step(s) end end
local s=G.new()
assert(not G.command(s,"unknown"))
assert(not G.command(s,"motor") and not G.command(s,"advance") and not G.command(s,"guard"))
assert(G.command(s,"mill") and not G.command(s,"mill"));step(s,120)
assert(not G.command(s,"mill") and not G.command(s,"motor"))
assert(G.command(s,"harvest"));step(s,150);assert(s.blue.stock==1)
assert(G.command(s,"harvest"));step(s,35);assert(s.blue.haul==150)
G.command(s,"harvest");step(s,150);assert(s.blue.alloy==8 and s.blue.stock==0)
G.command(s,"watch");step(s,90);G.command(s,"section");G.command(s,"power")
step(s,10);assert(s.blue.queues.watch.left==90)
G.command(s,"power");step(s,90);assert(s.blue.section)
G.command(s,"advance");step(s,30);G.command(s,"recall");step(s,30);assert(s.blue.walk==0)
local b,r=s.blue,s.red
b.walk,b.advance,b.hold=300,true,1799;r.drive,r.guard=210,true
G.step(s);assert(b.hold==0)
b.drive,b.guard,r.readiness=210,true,1;G.step(s);assert(not r.guard)
b.walk,b.advance,r.walk,r.advance=300,true,300,true;r.drive,r.guard=0,false
G.step(s);assert(r.walk==0)
b.hold=1799;G.step(s);assert(s.result=="victory" and not G.command(s,"harvest"))
local done=s.tick;G.step(s);assert(s.tick==done)
local idle=G.new();step(idle,5700);assert(idle.result=="defeat")
local empty=G.new();G.command(empty,"harvest");step(empty,340);assert(empty.blue.stock==1 and empty.blue.alloy==8)
G.command(empty,"mill");step(empty,3000);assert(empty.blue.loads==0 and empty.blue.alloy==36)
-- Reserve a follow-on without double spending, replaying it or stealing the active crew.
local early=G.new()
assert(not G.command(early,"recall"))
assert(G.command(early,"mill"))
step(early,116)
assert(not G.command(early,"watch") and not early.blue.pending)
step(early,1)
assert(G.command(early,"watch") and not G.command(early,"watch"))
assert(not G.command(early,"mill") and early.blue.alloy==4)
step(early,3)
assert(early.blue.mill and early.blue.queues.fit.id=="watch" and early.blue.alloy==1)
assert(not early.blue.pending)
local prep=G.new()
G.command(prep,"mill"); G.command(prep,"harvest"); step(prep,300)
G.command(prep,"watch"); step(prep,87)
assert(G.command(prep,"section") and not G.command(prep,"motor"))
step(prep,3); assert(prep.blue.queues.watch.id=="section")
step(prep,90); assert(G.command(prep,"advance") and not G.command(prep,"advance"))
step(prep,121)
assert(prep.relief_at and prep.relief_at-prep.tick>=230 and prep.red.walk==0)
step(prep,300); assert(prep.red.walk>0)
-- A data pack can put a production prerequisite in a powered bay. Power loss expires its handover.
local jobs=G.jobs
jobs.armor.needs="section"
local power=G.new()
power.blue.watch=true; power.blue.motor=true; power.blue.alloy=20
assert(G.command(power,"section"));step(power,88)
assert(G.command(power,"power")); assert(not G.command(power,"armor"))
assert(G.command(power,"power")); assert(G.command(power,"armor"))
assert(G.command(power,"power")); step(power,3)
assert(not power.blue.pending and not power.blue.queues.motor)
assert(power.report:find("expired"))
jobs.armor.needs="motor"
local ending=G.new();ending.blue.advance=true;ending.blue.walk=300;ending.blue.hold=1799
ending.blue.pending={id="watch",expires=2}
G.step(ending);assert(ending.result=="victory" and not ending.blue.pending)
local here=debug.getinfo(1,"S").source:sub(2):match("^(.*[/\\\\])") or ""
assert(loadfile(here.."campaign_rules.lua"))(root)
debug.sethook()
-- luac -l lists even portions whose enclosing function never executes. The
-- separate coverage runner supplies that complete denominator for each file.
local dest=os.getenv("ORRA_LUA_HITS")
if dest then
    local out=assert(io.open(dest,"w"))
    for file,hits in pairs(covered) do for line in pairs(hits) do out:write(file," ",line,"\n") end end
    out:close()
end
print("Lua unit assertions passed")
