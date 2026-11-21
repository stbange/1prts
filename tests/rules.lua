-- Count only first-party script lines reached by these isolated unit cases.
local covered = {}
local root = assert(data_root)
local function hook(_,line)
    local info=debug.getinfo(2,"S")
    local path=info.source:match("/scripts/(.+%.lua)$")
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
