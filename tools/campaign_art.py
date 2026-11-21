"""Build metre-scale campaign scenery and inhabited vehicles from the loose fleet register."""
import math
import random
from make_art import OUT, box, quad, tube, beam, sign, write, facility, sprite, rail

colors=dict(weir="blue",sounding="plum",terrace="trim",salt="teal",lantern="orange",meridian="light",shared="blue",neutral="wall")
original=set("mole lintel wake fitters gate-section channel-section survey dynamo mill watch motor gate".split())
def plaque(x,y,z,w,label):
    scale=min(.14,(w-.4)/max(1,len(label)*6))
    box(x,y,z,w,1.38,.14,"black")
    sign(x-len(label)*3*scale,y+1.09,z+.09,scale,label,"paint")

def console(x,y,z):
    box(x,y,z,1.3,.95,.67,"dark")
    box(x,y+.96,z,1.38,.12,.72,"edge")
    box(x,y+1.08,z-.18,1.12,.57,.1,"glass")
    box(x,y+1.18,z-.117,.95,.33,.03,"reflection")

def craft(name,f,w,h,d,col):
    half=w/2
    tall=f in {"ferry","pontoon","tender","station","tug"}
    floor=.84
    box(0,floor-.16,0,w,.16,d,"shadow")
    # Stern ramp connects the road to the main working deck.
    for j in range(8):
        box(0,j*.102-.08,d*.5+2.1-j*.30,min(4.3,w*.76),.16,.37,"tread")
    for side in (-1,1):
        x=side*(half-.19)
        box(x,.04,0,.38,floor,d,col)
        box(x,floor,0,.21,1.02,d*.96,col)
        window=max(2.25,min(4.2,h*.44))
        box(x,floor+window,0,.26,.38,d*.96,"edge")
        for j in range(max(3,int(d/6))):
            z=-d*.46+j*d*.91/(max(3,int(d/6))-1)
            box(x,floor,z,.25,window,.25,col)
        box(x,floor+window+.38,-d*.17,.32,.50,d*.63,col)
        if tall:
            rail(x,floor+.03,d*.32,min(2,w*.2))
        if f in {"rail","tracked","crawler","truck"}:
            box(side*(half+.08),.16,0,.42,.66,d*.77,"tire")
            for j in range(5):
                z=(j-2)*d*.15
                tube((x,.57,z),(x+side*.47,.57,z),.53,"shadow",8)
        if f in {"launch","ferry","pontoon"}:
            quad((x,0,-d*.5),(0,.2,-d*.5-3),(0,1.95,-d*.5-3),(x,floor+1.06,-d*.5),col)
    box(0,floor,-d*.49,w,1.12,.25,col)
    box(0,floor+2.75,-d*.49,w,.25,.25,"edge")
    canopy=max(2.72,min(5.2,h*.56))
    box(0,floor+canopy,-d*.25,w+.4,.22,d*.47,col)
    for x in (-w*.28,w*.28):
        console(x,floor,-d*.37)
        box(x,floor+.02,d*.18,min(1.15,w*.18),.65,min(3,d*.17),"trim")
    for z,label in [(-d*.30,"BRIDGE"),(d*.13,"SERVICE")]:
        box(-half+.35,floor+2.23,z,.40,.22,1.8,"lamp")
        plaque(-w*.27,floor+2.82,z-.48,min(w*.43,4.5),label)
    if w>12:
        plaque(-w*.32,1.7,d*.49,min(w*.32,6),name.upper())
    else:
        plaque(-w*.24,canopy+.08,d*.33,min(w*.6,4.8),name.upper())
    for side in (-1,1):
        for mark in range(3):
            box(side*(half+.014),1.28,d*.34-mark*.49,.09,.15+.13*mark,.2,"paint")
    if f=="ferry":
        for side in (-1,1):
            box(side*3.2,.6,d*.49,.25,3.1,.25,"dark")
            box(side*3.2,3.59,d*.49,.8,.44,.29,"lamp")
        plaque(3.8,3.27,d*.49,2.5,"OPEN")
        for side in (-1,1):
            x=side*w*.39
            box(x,1.1,-d*.26,w*.18,5.7,d*.24,col)
            box(x,6.8,-d*.26,w*.21,.29,d*.25,"edge")
            for j in range(4):
                box(x,4.9,-d*.35+j*2.3,w*.185,1.22,1.1,"glass")
        box(0,7.04,-d*.3,w*.91,2.8,7,col)
        for j in range(5):
            box((j-2)*2.8,7.7,-d*.3+3.53,2.35,1.32,.1,"glass")
        box(0,9.86,-d*.3,w*.96,.35,8.1,"edge")
        tube((0,10,-d*.3),(0,14.5,-d*.3),.13,"dark")
        box(0,13,-d*.3,4,.16,.26,"trim")
    if f in {"aircraft","tiltrotor","tender","tug"}:
        height=floor+1.5
        for side in (-1,1):
            span=(w*1.4 if f in {"aircraft","tiltrotor"} else w*.62)
            quad((side*half,height,-d*.21),(side*(half+span),height+.38,d*.12),
                 (side*(half+span),height+.38,d*.22),(side*half,height,d*.15),col)
            pod=side*(half+span*.84)
            tube((pod,height,d*.22),(pod,height,-d*.06),max(.46,w*.105),"tread",10)
            if f=="tiltrotor":
                box(pod,height+1,d*.06,w*1.6,.14,.52,"black")
                box(pod,height+1,d*.06,.52,.14,w*1.6,"black")
            box(side*w*.35,canopy*.65,d*.42,.3,h*.47,d*.17,col)
    if f in {"ferry","station"}:
        bridge=min(5.8,h*.47);steps=22
        for j in range(steps):
            box(-w*.29,floor+bridge*(j+1)/steps-.17,d*.08-j*.64,2.5,.17,.69,"tread")
        box(0,floor+bridge-.19,-d*.22,w*.85,.19,d*.36,"shadow")
        for x in (-w*.39,w*.39):box(x,floor+bridge,-d*.24,.23,1.06,d*.32,"paint")
        console(0,floor+bridge,-d*.35)
        if f=="ferry":
            for x in (-w*.42,w*.42):
                tube((x,1,-d*.1),(x,12,-d*.1),1.15,"trim",10)
                tube((x,10,-d*.1),(x,11.2,-d*.1),1.19,"paint",10)
            for z in (-d*.4,d*.4):
                beam((-half,1.5,z),(-half,10,z),.13,"edge")
        else:
            for section,label in enumerate(("COLLAR CONTROL","PRESSURE LOCK","CREW QUARTERS","TRANSFER HALL","WATCH OFFICE")):
                center=d*(.42-section*.21)
                box(0,5.8,center,w*.80,.42,39,"light")
                for side in (-1,1):
                    x=side*w*.39
                    box(x,.84,center,.38,1.14,38,col)
                    box(x,4.48,center,.36,1.36,38,col)
                    for pane in range(4):box(x,1.98,center-14+pane*9,.35,2.5,.4,"edge")
                    box(side*w*.26,.84,center+18.9,w*.28,4.96,.35,col)
                    box(side*w*.26,3.52,center+19.12,w*.18,.28,.06,"lamp")
                    console(side*w*.30,.84,center+4)
                plaque(-w*.23,3.94,center+19.12,5.9,label)
                box(0,5.45,center,2,.10,34,"lamp")
            for j in range(25):
                z=-d*.48+j*d*.96/24
                for side in (-1,1):
                    x=side*half
                    beam((x,1,z),(x,10,z+16),.43,"trim")
                    box(x+side*9,4,z,17,.36,10,"blue")
                    box(x+side*9,4.38,z,16,.11,.35,"lamp")
                if j%4==0:plaque(0,4,z,9,"COLLAR "+str(j//4+1))
    write(name)

def landscape(name):
    r=random.Random(name)
    orbit=name=="orbit"; water=name=="estuary"; flat=name=="saltpan"
    base="glass" if orbit else "sea" if water else "edge" if flat else "grass"
    box(0,-1.17,-70,1500,.7,1500,base)
    if water:
        for z in (99,-308):
            box(0,-.5,z,720,.48,138,"cement")
            for x in (-40,0,55,106):
                box(x,-.51,z+(1 if z<0 else -1)*49,16,.51,58,"road")
                for offset in (-6,6):box(x+offset,0,z,1.2,1.5,1.2,"tread")
        for _ in range(350):
            box(r.uniform(-260,260),-.449,r.uniform(-260,20),r.uniform(2,12),.012,.15,"wave")
    else:
        box(0,-.34,-40,280,.34,360,"shadow" if orbit else "cement" if name=="airfield" else "soil" if not flat else "white")
    if name in {"railbed","airfield","orbit"}:
        for x in (-4,4) if name=="railbed" else (-14,14):
            box(x,-.01,-50,.27,.09,270,"tread" if name=="railbed" else "paint")
        for j in range(43):
            z=84-j*6
            if name=="railbed":
                box(0,-.02,z,10,.08,.5,"copper")
            else:box(0,.02,z,.37,.04,2.8,"white")
    if water:
        for lane in (0,55):
            box(lane+6,-.37,-216,29,.37,90,"road")
            for step in range(7):
                box(lane-6,.01,-176-step*12,1,.05,3,"paint")
        for x,z in [(-25,44),(41,-215)]:
            for foot in (-1,1):
                box(x+foot*3,0,z,1.4,15,1.5,"trim")
                beam((x+foot*3,1,z),(x-foot*3,12,z),.22,"edge")
            beam((x-4,14,z),(x+12,17,z),.65,"orange")
            box(x,12,z,5,3.2,3.3,"glass");box(x+12,9,z,.16,7.8,.16,"tread")
        for x,z in [(-25,59),(-32,64),(27,-245),(35,-250)]:
            box(x,0,z,5,2.3,3.1,"copper");box(x,2.3,z,4.8,.14,3.2,"paint")
    if name=="airfield":
        for z in range(-180,94,12):
            for x in (-23,23):
                box(x,.06,z,.56,.31,.58,"teal");box(x,.38,z,.4,.19,.37,"lamp")
    if name=="railbed":
        for z in range(-165,89,24):
            box(-11,0,z,.34,8,.34,"trim");box(-7,7.7,z,8.4,.28,.31,"tread")
            beam((-11,5.3,z),(-7,7.7,z),.11,"dark")
            beam((-3,8,z),(-3,8,z+24),.08,"dark")
    if not orbit:
        for j in range(80):
            x=(155+r.random()*250)*r.choice((-1,1));z=r.uniform(-390,190)
            h=r.uniform(8,46)
            tube((x,-1,z),(x+8,h,z),r.uniform(16,39),"cement" if flat else "wall",5,1)
    for j in range(11):
        x=((-1)**j)*(44+4*(j%3));z=80-j*22
        box(x,0,z,.29,7,.29,"dark");box(x,7,z,1.6,.28,.62,"lamp")
    write(name)

def main():
    tube((0,.10,0),(0,1.18,0),.58,"orange",10)
    tube((0,.23,0),(0,.38,0),.60,"paint",10)
    box(0,1.18,0,.8,.13,.43,"edge")
    plaque(0,.24,.55,1.03,"CORE");write("core")
    sprite("spray",["      w   ","  w   ww  ","   w ww w "," w   www  ","  ww ww   "],dict(w="wave"),.18)
    for name in ("estuary","railbed","saltpan","airfield","orbit"):landscape(name)
    for line in (OUT/"fleet.txt").read_text().splitlines():
        if not line or line.startswith("#"):continue
        name,f,w,h,d,owner,role=line.split()
        if name in original:continue
        w,h,d=map(float,(w,h,d));col=colors[owner]
        if f in {"hall","plant","works","hangar","tower"}:
            facility(name,name.upper(),role.upper(),w,h,d,col)
        elif f=="people":
            rows=["  hhh  "," hhhhh ","  fff  ","  fbf  "," cjjjc "," jjwjj "," fjwjf ","  ppp  ","  p p  ","  b b  "]
            sprite(name,rows,dict(h=col,f="skin",c="edge",j=col,b="black",w="paint",p="shadow"),.188)
        else:craft(name,f,w,h,d,col)
    target=OUT.parent/"mods"/"long-watch";target.mkdir(parents=True,exist_ok=True)
    for name,dest in [("lintel","oxide"),("kestrel","harbor")]:
        repaint(name,target/(dest+".mesh"))
    # The add-on acknowledgment is a shorter, falling version of the dispatch signal.
    import wave
    with wave.open(str(OUT/"sounds"/"blocked.wav")) as source:
        with wave.open(str(target/"radio.wav"),"wb") as result:
            result.setparams(source.getparams());result.writeframes(source.readframes(source.getnframes()//2))

def repaint(name,target):
    data=bytearray((OUT/"models"/(name+".mesh")).read_bytes())
    for offset in range(8+24,len(data),28):
        red,green,blue=data[offset:offset+3]
        if blue>red*1.15 and green>red*1.15:data[offset:offset+3]=bytes((144,82,48))
    target.write_bytes(data)
if __name__=="__main__":main()
