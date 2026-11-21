"""Build the original, metre-scale Orra meshes and loose radio cues. Standard library only."""
import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets"
(OUT / "models").mkdir(parents=True, exist_ok=True)
(OUT / "sounds").mkdir(exist_ok=True)
rng = random.Random(614)
palette = dict(blue="315962", teal="397783", light="62919a", white="d5c9a7", orange="cb773f",
    trim="97543a", copper="a87551", dark="293c40", shadow="3b4847", tire="303736",
    tread="52605c", glass="284c58", reflection="76aeb0", lamp="f4c475", black="192a30",
    cement="9b9b85", wall="b5ae91", edge="d0ba97", dirt="8d8067", soil="a59a79",
    patch="877e66", road="696f65", paint="d8bd83", grass="757c5a", leaf="646f53",
    sea="658a85", wave="9fae95", plum="704c65", rose="a27883", skin="c39b77")
glyph = {s[0]: bytes.fromhex(s.split()[1]) for s in (OUT/"font.txt").read_text().splitlines()}
vertices = bytearray()

def rgb(c):
    return tuple(bytes.fromhex(palette.get(c,c)))

def tri(a,b,c,col):
    ab=[b[i]-a[i] for i in range(3)]; ac=[c[i]-a[i] for i in range(3)]
    n=[ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]]
    mag=max(0.000001,math.sqrt(sum(x*x for x in n))); n=[x/mag for x in n]
    for p in (a,b,c): vertices.extend(struct.pack("<6f4B",*p,*n,*rgb(col),255))

def quad(a,b,c,d,col):
    tri(a,b,c,col); tri(a,c,d,col)

def box(x,y,z,w,h,d,col):
    a,b=x-w/2,x+w/2; l,r=z-d/2,z+d/2; t=y+h
    quad((a,y,r),(b,y,r),(b,t,r),(a,t,r),col)
    quad((b,y,l),(a,y,l),(a,t,l),(b,t,l),col)
    quad((b,y,r),(b,y,l),(b,t,l),(b,t,r),col)
    quad((a,y,l),(a,y,r),(a,t,r),(a,t,l),col)
    quad((a,t,r),(b,t,r),(b,t,l),(a,t,l),col)
    quad((a,y,l),(b,y,l),(b,y,r),(a,y,r),col)

def tube(a,b,r,col,sides=8,end=None):
    v=[b[i]-a[i] for i in range(3)]
    m=math.sqrt(sum(i*i for i in v)); v=[i/m for i in v]
    e=(0,1,0) if abs(v[1])<0.9 else (1,0,0)
    def cross(p,q): return (p[1]*q[2]-p[2]*q[1],p[2]*q[0]-p[0]*q[2],p[0]*q[1]-p[1]*q[0])
    u=cross(v,e); m=math.sqrt(sum(i*i for i in u)); u=[i/m for i in u]; w=cross(v,u)
    def ring(p,radius,k):
        theta=2*math.pi*k/sides
        return tuple(p[i]+radius*(u[i]*math.cos(theta)+w[i]*math.sin(theta)) for i in range(3))
    for k in range(sides):
        a0,a1=ring(a,r,k),ring(a,r,k+1)
        b0,b1=ring(b,end if end is not None else r,k),ring(b,end if end is not None else r,k+1)
        quad(a0,a1,b1,b0,col); tri(a,a1,a0,col); tri(b,b0,b1,col)

def sign(x,y,z,s,word,col):
    for i,ch in enumerate(word):
        for j,bits in enumerate(glyph.get(ch,b'')):
            for k in range(5):
                if bits & (16>>k):
                    a=x+(i*6+k)*s; b=y-j*s
                    quad((a,b-s,z),(a+s,b-s,z),(a+s,b,z),(a,b,z),col)

def rail(x,y,z,w):
    for h in (.55,1.02): box(x,y+h,z,w,.065,.075,"paint")
    for i in range(int(w/1.5)+1):
        box(x-w/2+i*1.5,y,z,.085,1.08,.085,"shadow")

def beam(a,b,r=.10,col="shadow"):
    tube(a,b,r,col,4)

def write(name):
    (OUT/"models"/(name+".mesh")).write_bytes(b"ORRA"+struct.pack("<I",len(vertices)//28)+vertices)
    vertices.clear()

def ground():
    box(0,-2,0,550,2,540,"soil")
    box(0,.005,-18,18,.025,154,"road")
    for z in range(-90,64,6):
        box(.1,.04,z,.18,.012,2,"paint")
    for x in (-9.05,9.05):
        box(x,.038,-18,.16,.014,154,"paint")
        for z in range(-88,62,7): box(x+(1 if x>0 else -1)*.55,.032,z,.35,.12,.65,"edge")
    for i in range(420):
        x,z=rng.uniform(-112,105),rng.uniform(-130,60)
        if abs(x)<9: continue
        r=rng.uniform(.3,3.5); y=.024
        tri((x,y,z),(x+r,y,z+.16*r),(x+.4*r,y,z+r*.5),rng.choice(["dirt","patch","grass"]))
    for side in (-1,1):
        xs=[side*(5+rng.randrange(4)) for _ in range(14)]
        for i,x in enumerate(xs):
            z=8+i*.28
            quad((x,.06,z),(x+side*1.8,.06,z-.4),(x+side*1.9,.06,z-.2),(x,.06,z+.2),"paint")
    # Survey pads, a sunken drain and two lines of kerb paint on the home apron.
    for x,z,w,d in ((-26,-47,19,14),(33,-51,16,12),(-29,25,17,23)):
        box(x,.015,z,w,.028,d,"cement")
        for dx in (-1,1):
            for dz in (-1,1):
                box(x+dx*(w/2-.9),.048,z+dz*d/2,1.8,.016,.18,"orange")
                box(x+dx*w/2,.048,z+dz*(d/2-.9),.18,.016,1.8,"orange")
    for z in range(-55,28,3):
        box(10.1,.032,z,.42,.02,2.62,"shadow")
        for dz in (-.8,0,.8):box(10.1,.058,z+dz,.45,.02,.11,"tread")
    # Raised pipeline on saddles separates the service apron from the road.
    for x,z,length in ((-16,-23,16),(20,1,19)):
        tube((x,.98,z-length/2),(x,.98,z+length/2),.36,"copper",8)
        for dz in range(-length//2+1,length//2,3):
            box(x,0,z+dz,.99,.68,.35,"cement")
            tube((x,.98,z+dz),(x,.98,z+dz+.15),.43,"dark",8)
    # Open water and low horizontal glints on the approach bank.
    box(48,.08,-78,51,.07,58,"sea")
    for i in range(110):
        box(rng.uniform(24,71),.16,rng.uniform(-105,-52),rng.uniform(.3,3.5),.01,.10,"wave")
    # Three concentric, fully three-dimensional ridgelines.
    for layer,(radius,base,amplitude) in enumerate(((232,32,49),(180,17,39),(138,10,19))):
        count=65; rings=[]
        for i in range(count):
            angle=i*2*math.pi/count
            height=base+amplitude*(.22+.46*(.5+.5*math.sin(4*angle+.7*layer))+.32*rng.random())
            rings.append((radius*math.cos(angle),height,radius*math.sin(angle)))
        colors=(("9a9a89","aa9e87","9a9181"),("85897a","94937c","a39a81"),("737e6b","899079","9a997d"))[layer]
        for i,p in enumerate(rings):
            q=rings[(i+1)%count]; a=(p[0]*.83,-.4,p[2]*.83); b=(q[0]*.83,-.4,q[2]*.83)
            mid=((p[0]+q[0])*.48,(p[1]+q[1])*.29,(p[2]+q[2])*.48)
            tri(a,mid,p,rng.choice(colors));tri(p,mid,q,rng.choice(colors));tri(q,mid,b,rng.choice(colors));tri(a,b,mid,rng.choice(colors))
    # Lattice towers, with the catenaries visibly sagging between them.
    for side in (-1,1):
        x=side*49
        for j,z in enumerate((30,-14,-58,-102)):
            h=15+(j%2)*2
            for dx in (-1,1):
                for dz in (-1,1): beam((x+dx*2.1,0,z+dz*1.4),(x+dx*.45,h,z+dz*.4),.16)
                for lev in (1,5,9):
                    beam((x-1.8+lev*.08,lev,z+dx),(x+1.8-(lev+4)*.08,lev+4,z+dx),.10)
                    beam((x+1.8-lev*.08,lev,z+dx),(x-1.8+(lev+4)*.08,lev+4,z+dx),.10)
            box(x,h,z,8,.35,.4,"shadow");box(x,h-2,z,6,.27,.35,"shadow")
            if j<3:
                for dx in (-3.55,3.55):
                    for k in range(6):
                        def pt(t): return (x+dx,h+((1 if j%2==0 else -1)*2)*t-2.8*math.sin(math.pi*t),z-44*t)
                        beam(pt(k/6),pt((k+1)/6),.052,"black")
    for i in range(55):
        x,z=rng.uniform(-98,96),rng.uniform(-121,55)
        if abs(x)<43: continue
        h=rng.uniform(1.5,4.8)
        tube((x,0,z),(x,h*.65,z),.13,"copper",5)
        tube((x,h*.28,z),(x,h,z),h*.28,"leaf",6,.03)
    for x,z in ((-13,14),(14,-7),(-14,-24),(15,-41)):
        box(x,0,z,.32,2.1,.32,"cement");box(x,1.4,z,.37,.53,.37,"orange");box(x,2.11,z,.38,.12,.38,"lamp")
    for i in range(14):
        x=-38+(i%5)*2;z=-27-(i//5)*2.7
        box(x,0,z,1.8,1.48,2.4,"copper" if i%3 else "blue")
        for dz in (-.75,.75):box(x,1.48,z+dz,1.85,.07,.18,"dark")
    # The survey stair rises from the road to a staffed observation landing.
    cx=-19.5
    for j in range(20):
        z=20-(j+.5)*.65; h=(j+1)*.29
        box(cx,h-.13,z,4.4,.13,.65,"tread");box(cx,h,z+.27,4.4,.025,.085,"paint")
    for dx in (-2.18,2.18):
        beam((cx+dx,1.0,20),(cx+dx,6.85,7),.075,"paint")
        beam((cx+dx,.05,20),(cx+dx,5.85,7),.15,"dark")
        for j in (0,5,10,15,19):
            box(cx+dx,(j+1)*.29,20-(j+.5)*.65,.09,.98,.09,"dark")
    box(-19.5,5.58,3.5,7.8,.22,7,"tread")
    for x in (-23.2,-15.8):
        for z in (.12,6.8):box(x,0,z,.27,5.58,.27,"shadow")
        beam((x,.3,.12),(x,5.5,6.8),.12,"tread")
        for z in (0,2.3,4.6,7):box(x,5.8,z,.09,1.1,.09,"dark")
        beam((x,6.82,0),(x,6.82,7),.065,"paint")
    rail(-19.5,5.8,.13,7.7)
    write("terrain")

def facility(name,title,tag,w,h,d,col):
    # Two jambs leave a full-height opening, and the interior floor connects to the apron.
    box(0,-.03,0,w,.08,d,"shadow")
    box(0,.04,-d/2,w,h,.38,col)
    for x in (-w/2,w/2): box(x,.04,0,.40,h,d,col)
    door=5.6
    for side in (-1,1):
        a=(w-door)/2; x=side*(w/2-a/2)
        box(x,.04,d/2,a,h,.45,col);box(x,.15,d/2+.25,a,.75,.10,"cement")
        for j in range(max(1,int(a/.8))):box(side*(door/2+.35+j*.8),1,d/2+.25,.10,h-1,.16,"light" if col=="blue" else "edge")
    box(0,h-.15,0,w+1,.42,d+1,"dark")
    box(0,4.6,d/2,w,h-4.6,.5,col)
    box(0,4.5,d/2+.5,6.15,.40,1.55,"orange")
    for x in (-3.05,3.05):box(x,0,d/2+.20,.26,4.6,.42,"trim")
    box(0,h-2.05,d/2+.34,w*.87,1.6,.18,"dark")
    s=min((w*.8)/(6*len(title)),.145)
    sign(-len(title)*3*s,h-.98,d/2+.45,s,title,"white")
    # Upper monitor windows face the street, with independent mullions.
    box(-w*.13,h+.22,-d*.10,w*.62,2.1,d*.54,"blue")
    box(-w*.13,h+.78,d*.17+.03,w*.58,.90,.15,"glass")
    for i in range(8):
        box(-w*.41+i*w*.08,h+.73,d*.17+.12,.115,1.02,.08,"cement")
    box(-w*.13,h+2.3,-d*.10,w*.65,.27,d*.57,"edge")
    for i in range(4):
        box(w*.37,h+.27,(-.35+i*.21)*d,2.2,.55,1.3,"tread")
    # Side louvers, conduit elbows and stacked industrial access panels.
    for side in (-1,1):
        z0=d*.14
        box(side*(w/2+.24),1.35,z0,.16,2.25,4.5,"dark")
        for jj in range(7):box(side*(w/2+.35),1.48+jj*.275,z0,.20,.085,4.22,"tread")
        tube((side*w*.42,1.02,d/2+.38),(side*w*.42,h-2.2,d/2+.38),.12,"copper",8)
        for jj in range(3):box(side*w*.42,1.3+jj*1.15,d/2+.4,.34,.105,.28,"dark")
    # Machines, staffed desk and interior run lights.
    for x in (-w*.33,w*.33):
        box(x,.05,-1,2.1,.92,4.5,"tread");box(x,.98,-1,2.15,.14,4.55,"edge")
        box(x,1.12,-2,1.5,.85,.18,"dark");box(x,1.23,-1.89,1.2,.52,.035,"reflection")
        box(x,h-.31,0,.21,.13,d*.75,"lamp")
    box(0,1.6,-d/2+.22,4,1.25,.15,"dark")
    sign(-1.55,2.55,-d/2+.31,.085,tag,"paint")
    if name=="survey":
        # Full-size enamel schematic and instruments inside the dispatch room.
        box(0,1.15,-d/2+.33,7.8,2.8,.13,"black")
        sign(-3.56,3.63,-d/2+.413,.095,"SECTOR 06","paint")
        for j in range(6):box(-3.2+j*1.25,1.34,-d/2+.409,.025,1.65,.015,"tread")
        for j in range(4):box(0,1.43+j*.43,-d/2+.41,7.16,.027,.018,"tread")
        pts=[(-3,1.62),(-1.74,1.62),(-1.1,2.24),(.55,2.24),(1.13,2.72),(2.9,2.72)]
        for a,b in zip(pts,pts[1:]):beam((a[0],a[1],-d/2+.45),(b[0],b[1],-d/2+.45),.045,"reflection")
        for x,yy in pts:box(x,yy-.065,-d/2+.47,.14,.14,.03,"lamp")
        for j in range(4):
            x=2.55+j*.44
            box(x,.12,-d/2+1.12,.38,1.15,.72,"blue")
            box(x,1.27,-d/2+1.12,.32,.09,.46,"edge")
        box(-2,.1,-d/2+1.8,2.6,.95,.7,"tread");box(-2,1.07,-d/2+1.8,2.72,.12,.83,"copper")
    rail(-w*.35,.1,d/2+1.1,w*.23)
    if name in ("dynamo","mill"):
        for i in range(3):
            x=(i-1)*3.4; z=-d*.34; base=h+1
            tube((x,base,z),(x,base+6+i*1.1,z),.97,"cement",10,.72)
            tube((x,base+4,z),(x,base+5,z),.86,"orange",10,.83)
            tube((x,base+6+i*1.1,z),(x,base+6.45+i*1.1,z),.85,"dark",10)
    if name=="survey":
        x=-w*.34
        box(x,h+.24,0,4.4,3.6,4,"wall")
        box(x,h+2.3,2.02,3.9,1.05,.16,"glass")
        box(x,h+3.85,0,5,.28,4.8,"dark")
        beam((x,h+4,0),(x,h+10,0),.085,"dark")
        for j in range(3):box(x,h+7+j*.65,0,2.5-j*.5,.065,.075,"dark")
    write(name)

def floodgate():
    for side in (-1,1):
        x=side*13
        box(x,0,0,8.5,18.5,9,"cement")
        for dx in (-3.7,3.7):box(x+dx,0,.15,1.4,22.3,10,"wall")
        box(x,4.5,4.72,6.7,12.5,.45,"blue")
        for i in range(6):box(x,5+i*1.92,5.02,6.65,.22,.25,"light")
        box(x,18.4,0,9.4,1,10,"edge")
        box(x,19.4,0,6.3,3.8,7.4,"blue");box(x,20.2,3.76,5.8,1.25,.18,"glass")
        for j in (-1.8,0,1.8):box(x+j,20.1,3.9,.15,1.45,.18,"wall")
        box(x,23.2,0,7.6,.44,8.4,"dark")
        box(x,15,5.08,6.2,3.1,.12,"dark");sign(x-1.92,17.67,5.16,.32,"06","white")
        tube((x+side*2,23.7,0),(x+side*2,27,0),.10,"dark");box(x+side*2,26.7,0,.52,.55,.52,"orange")
    box(0,15.5,0,24,3.4,5.3,"blue");box(0,15.42,2.71,24,.44,.35,"orange")
    for i in range(8):
        x=-11+i*3; beam((x,16,2.7),(x+2.8,18.6,2.7),.18,"light")
    rail(0,18.9,2.7,24);box(0,19,-.7,16,.45,1.3,"tread")
    # The center entrance is walkable right through to the switch room.
    box(0,0,-4,16,7.3,.4,"teal")
    for side in (-1,1):
        box(side*6.3,0,1.1,3.7,6.2,5,"teal")
        for j in range(4):box(side*6.3,1.1*j+.45,3.65,3.6,.15,.16,"light")
    box(0,5.0,2.6,17,2.1,2.5,"dark");sign(-3.9,6.65,3.88,.147,"GATE SIX","white")
    for x in (-3.8,3.8):box(x,3.8,3.95,.4,.65,.16,"lamp")
    rail(-6,.1,7,4);rail(6,.1,7,4)
    # Pillbox windows, maintenance stairs and yard identity boards.
    for side in (-1,1):
        x=side*13
        for j in range(5):
            box(x+side*3.7,1+j*.95,6-j*.20,1,.18,1.35,"edge")
        for j in range(3):
            box(x-2+j*2.0,10.5,5.14,1.16,.65,.19,"glass")
            box(x-2+j*2.0,11.2,5.12,1.32,.12,.32,"edge")
        for j in range(7):
            box(x-2.8+j*.88,3.9,4.75,.4,.3,.28,"orange")
    box(-3.7,.1,-1,2,1,2,"tread");box(-3.7,1.1,-1.7,1.5,.7,.2,"reflection")
    write("gate")

def vehicle(name,col,scale,armor):
    w,l,h=3.5*scale,8*scale,2.0*scale
    # Split hull and real centre aisle, open entrance at the rear (+Z).
    box(0,.5,0,w,.32,l,"shadow")
    for side in (-1,1):
        box(side*w*.47,.7,0,w*.15,h*.82,l*.91,col)
        box(side*w*.63,.35,0,w*.31,.95*scale,l,"tire")
        for j in range(8):
            z=-l*.41+j*l*.117
            tube((side*w*.64,.78*scale,z),(side*w*.82,.78*scale,z),.53*scale,"tread",9)
            tube((side*w*.81,.78*scale,z),(side*w*.84,.78*scale,z),.26*scale,"shadow",8)
        box(side*w*.63,1.38*scale,0,w*.36,.23*scale,l*1.02,col)
        for j in range(6):
            z=(j-2.5)*l*.145
            box(side*w*.66,1.64*scale,z,w*.35,.23*scale,l*.132,"light" if col!="plum" else "rose")
        for j in range(11):box(side*w*.65,.19*scale,(-.45+j*.09)*l,w*.36,.10*scale,.18*scale,"tread")
    box(0,.75,-l*.43,w*.85,h*.86,l*.13,col)
    box(0,h*.92,-l*.16,w*.82,.24,l*.58,col)
    box(0,h*.99,-l*.22,w*.72,.86*scale,l*.29,col)
    box(0,h*1.09,-l*.08,w*.62,.32*scale,.055,"glass")
    box(0,h*1.03,-l*.06,w*.68,.10,.12,"paint")
    for x in (-w*.33,w*.33):box(x,1.02,-l*.50,.32,.25,.13,"lamp")
    if armor:
        tube((0,h+1,-l*.23),(0,h+1,-l*.85),.18*scale,"dark",8)
        box(0,h+1,-l*.71,.44*scale,.34*scale,l*.30,col)
        tube((-.49,h+1.01,-l*.24),(-.49,h+1.43,-l*.24),.56*scale,col,8)
    else:
        for side in (-1,1):
            box(side*w*.4,1.55,l*.19,.25*scale,1.33*scale,l*.41,"copper")
            for j in range(4):box(side*w*.45,1.7,l*(.03+j*.1),.19*scale,1.2*scale,.16*scale,"paint")
    box(0,.72,l*.38,w*.53,.14,l*.21,"tread")
    for j in range(3):box(0,.57-j*.17,l*(.46+j*.043),w*.46,.12,l*.052,"edge")
    # The crew shares a radio desk and a bank of analog instruments at the front of the aisle.
    box(0,.81,-l*.055,w*.61,.61,.58,"dark")
    box(0,1.44,-l*.055,w*.62,.11,.69,"tread")
    for i in (-1,0,1):
        x=i*w*.16
        box(x,1.55,-l*.065,w*.12,.35,.12,"black")
        box(x,1.59,-l*.065+.068,w*.095,.20,.02,"reflection")
        box(x-.02,1.66,-l*.065+.083,.033,.13,.015,"paint")
    for side in (-1,1):
        box(side*w*.43,1.0,l*.20,.15,.30,l*.35,"tread")
        tube((side*w*.40,1.7,l*.07),(side*w*.40,1.7,l*.37),.055,"paint",8)
    if not armor and col=="orange":
        # The stripped salvage boom gives the Mole a recognizable asymmetrical profile.
        x=-w*.23
        tube((x,h*.86,-l*.27),(x,h+3.3,-l*.40),.38,"trim",6)
        beam((x+.3,h*.93,-l*.25),(x+.3,h+3.0,-l*.40),.16,"edge")
        beam((x,h+3.23,-l*.4),(x,h+3.65,-l*.70),.35,"orange")
        beam((x,h+3.9,-l*.4),(x,h+4.2,-l*.70),.14,"shadow")
        beam((x,h+3.68,-l*.70),(x,h+1.55,-l*.70),.055,"black")
        box(x,h+1.28,-l*.70,.82,.30,.77,"dark")
    if armor:
        box(w*.34,h+.11,l*.07,.85,.80,.9,"blue")
        box(w*.34,h+.34,l*.07+.46,.68,.27,.05,"glass")
        sign(-w*.38,h+.54,-l*.08+.075,.045,"W 17","white")
    box(-w*.28,.86,l*.16,.56,.56,.8,"dark")
    beam((w*.27,h+.68,0),(w*.27,h+2.7,0),.042,"black")
    write(name)

def sprite(name,rows,colors,px):
    for j,line in enumerate(rows):
        for i,ch in enumerate(line):
            if ch not in colors: continue
            x=(i-len(line)*.5)*px; y=(len(rows)-j-1)*px
            quad((x,y,0),(x+px,y,0),(x+px,y+px,0),(x,y+px,0),colors[ch])
    write(name)

def build_original():
    ground(); floodgate()
    for args in (("survey","SURVEY HALL","DISPATCH",18,7.1,13,"blue"),("dynamo","DYNAMO 02","GRID 02",16,8.2,13,"wall"),
                 ("mill","PLATE MILL","BATCH 04",17,8.6,12,"blue"),("watch","WATCH HOUSE","MUSTER",14,6.6,10,"wall"),
                 ("motor","MOTOR HALL","LINE 01",21,8.2,14,"blue")):
        facility(*args)
    vehicle("mole","orange",2.13,False);vehicle("lintel","blue",1.04,True);vehicle("wake","plum",1.73,False)
    person=[
        "       hhhh       ","     hhhhhhhh     ","    hhHHHHHHhh    ","    hhhhhhhhhh    ",
        "    bbbhhhhbbbb   ","     fFffFff      ","     fbbffbb      ","     fFfffFf      ",
        "      fffcf       ","      cfffc       ","    jjjccjjjj     ","   jjjwjjwwjjj    ",
        "   jjjwwwwjjrj    ","   jjwwjjwwjrj    ","   jsjjjjjjjsj    ","   csjbwwbjjsf    ",
        "   fsjbbbbjjff    ","     ppppppp      ","     pppbbpp      ","     ppb  pp      ",
        "     ppp  pp      ","     bpp  pb      ","     ppb  pp      ","     bbb  bbb     ","    bbbb  bbbb    "]
    for name,jacket,hat in (("worker","orange","lamp"),("section","teal","light"),("enemysection","plum","rose")):
        colors=dict(h=hat,H="edge",f="skin",F="white",c="copper",j=jacket,w="paint",s="skin",
                    p="shadow",b="black",r="tread")
        if name=="enemysection":
            colors.update(h="f0ce91", H="fff2ba", j="362234", w="ffe8a8", p="20262a", b="161b20")
        sprite(name,person,colors,.077)
    for name,hz in (("ack",482),("blocked",185)):
        with wave.open(str(OUT/"sounds"/(name+".wav")),"wb") as wav:
            wav.setparams((1,2,22050,0,"NONE","not compressed"))
            signal=[int(4500*math.exp(-i/780)*math.sin(i*hz*2*math.pi/22050)) for i in range(3300)]
            wav.writeframes(struct.pack("<"+"h"*len(signal),*signal))

if __name__ == "__main__":
    build_original()
