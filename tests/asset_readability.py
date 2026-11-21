"""Compare the editable tank silhouettes from a raised three-quarter viewpoint in grayscale."""
from pathlib import Path
import math
import struct
import sys
import wave

assets = Path(sys.argv[1])

def silhouette(model):
    data=(assets/"models"/(model+".mesh")).read_bytes()
    magic,count=struct.unpack_from("<4sI",data)
    assert magic==b"ORRA" and len(data)==8+count*28
    pixels=set()
    for start in range(0,count,3):
        pts=[]
        for i in range(start,start+3):
            x,y,z=struct.unpack_from("<fff",data,8+i*28)
            lateral=.866*x-.5*z
            vertical=.94*y-.342*(.5*x+.866*z)
            pts.append((lateral*4,vertical*4))
        def edge(a,b,p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        for row in range(math.floor(min(p[1] for p in pts)),math.ceil(max(p[1] for p in pts))):
            for col in range(math.floor(min(p[0] for p in pts)),math.ceil(max(p[0] for p in pts))):
                middle=(col+.5,row+.5)
                edges=[edge(pts[i-1],pts[i],middle) for i in range(3)]
                if min(edges)>=0 or max(edges)<=0:
                    pixels.add((col,row))
    return pixels

lintel,wake=map(silhouette,("lintel","wake"))
difference=len(lintel^wake)/len(lintel|wake)
assert difference>.10, difference
print(f"Lintel and Wake differ on {difference:.1%} of occupied silhouette pixels.")
tracks=[]
for name in ("yard-air","engine-room","shift-motif"):
    with wave.open(str(assets/"sounds"/(name+".wav")),"rb") as clip:
        assert clip.getnframes()>=clip.getframerate()
        tracks.append(clip.readframes(clip.getnframes()))
assert len(set(tracks))==3
