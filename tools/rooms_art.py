"""Generate the loose workstations, salvage markers and Crown lift in metres."""
from make_art import OUT, box, tube, beam, sign, write, rail, sprite
from campaign_art import plaque

def station(model, room, gear, width):
    w=min(2.6,width*.29)
    d=min(1.9,width*.23)
    box(0,0,0,w,.13,d,"tread")
    if gear in ("console","bench","scope"):
        box(0,.13,0,w*.85,.72,d*.72,"blue")
        box(0,.85,0,w,.1,d,"edge")
        if gear=="scope":
            tube((0,.94,0),(0,1.5,0),.09,"dark")
            tube((0,1.51,d*.31),(0,1.62,-d*.4),min(.23,w*.18),"teal")
        elif gear=="console":
            box(0,.97,-d*.24,w*.65,.53,.15,"black")
            box(0,1.06,-d*.24+.08,w*.51,.31,.02,"reflection")
    elif gear in ("pump","valve","capstan","dynamo"):
        tube((0,.12,0),(0,.79,0),w*.31,"copper",8)
        for y in (.26,.48,.73):tube((0,y,0),(0,y+.07,0),w*.35,"tread",8)
        tube((0,.79,0),(0,1.13,0),w*.1,"trim",8)
        if gear in ("pump","valve"):
            tube((-.3,.45,0),(-w*.45,.45,0),.14,"edge")
            box(0,1.13,0,w*.72,.08,.13,"paint")
        else:box(0,1.11,0,w*.81,.12,d*.62,"edge")
    elif gear=="hoist":
        for x in (-w*.4,w*.4):box(x,.1,0,.11,1.8,d*.27,"trim")
        box(0,1.9,0,w,.14,d*.34,"paint")
        tube((0,1.84,0),(0,.73,0),.04,"tread")
        box(0,.54,0,w*.63,.21,d*.63,"blue")
    else:
        if gear=="vat":tube((0,.13,0),(0,1.1,0),w*.32,"copper",8)
        else:box(0,.13,0,w*.86,.9,d*.86,"trim")
        box(0,1.08,0,w*.89,.12,d*.88,"edge")
    scale=min(.057,w*.9/(len(room)*6))
    box(0,.39,d*.50,w,.45,.06,"black")
    sign(-len(room)*3*scale,.70,d*.50+.04,scale,room.upper().replace("-"," "),"paint")
    write("work-"+model)

def main():
    fleet={r[0]:float(r[2]) for r in (line.split() for line in (OUT/"fleet.txt").read_text().splitlines() if not line.startswith("#"))}
    for line in (OUT/"rooms.txt").read_text().splitlines():
        if not line or line.startswith("#"):continue
        model,room,gear,_,_=line.split()
        station(model,room,gear,fleet[model])
    colors=dict(h="paint",f="skin",j="blue",b="tread",w="orange",p="shadow")
    sprite("operator-side",["  hhh  ","  hhhh ","  ffff ","  ffb  ","  jjj  ","  jwjj ","  jfff ","  ppp  ","  p p  ","  b bb "],colors,.188)
    sprite("operator-work",["  hhh   "," hhhhh  ","  fff   ","  fbf   "," jjjj   "," jwwjfff"," fjjj   ","  ppp   ","  p p   "," bb bb  "],colors,.188)
    for mark in ("survey-pad","plate-lot"):
        if mark=="survey-pad":
            for x in (-4,4):
                for z in (-4,4):
                    box(x,.03,z,1.6,.07,.22,"paint");box(x,.03,z,.22,.07,1.6,"paint")
        else:
            for i in range(4):
                box((i%2)*1.3,i*.13,0,2.4,.13,2,"tread")
        box(0,0,2.4,.17,1.45,.17,"trim")
        plaque(0,1.04,2.4,2.6,"PAD" if mark=="survey-pad" else "PLATE")
        write(mark)
    box(0,-.14,0,3.4,.14,3.5,"tread")
    for side in (-1,1):
        box(side*1.59,0,0,.12,1.1,3.4,"paint")
        box(side*1.59,1.1,0,.14,.12,3.4,"edge")
    write("lift-car")
    box(0,0,0,1.53,2.47,.13,"teal")
    box(0,.93,.08,1.36,.13,.1,"paint")
    write("lift-door")
    for x in (-1.82,1.82):
        for z in (-1.9,1.9):box(x,0,z,.17,9.5,.17,"tread")
    box(0,6,-3.3,5,.16,3.2,"edge")
    plaque(0,8.58,-4.72,4.6,"TRAFFIC")
    write("lift-guide")
    # The Shore Depot apron and stair stringers meet the physical gantry at 4 m.
    for i in range(16):box(-6,(i+1)*.25-.14,6-i*.52,2.4,.14,.55,"tread")
    box(-6,3.85,-9,4.1,.15,13.3,"edge")
    rail(-6,4,-15.5,4)
    for z in (5,-7,-15):beam((-8,0,z),(-8,4,z),.14,"trim")
    plaque(-6,5.3,-15.37,4.4,"DISPATCH")
    write("depot-gantry")
if __name__=="__main__":main()
