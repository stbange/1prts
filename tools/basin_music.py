"""Synthesize the original Basin shift motif and two quiet industrial room tones."""
import math
import struct
import wave
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "assets" / "sounds"
RATE = 16000

def render(name, seconds, sample):
    length=int(RATE*seconds)
    frames=bytearray()
    for i in range(length):
        t=i/RATE
        edge=min(1, i/(RATE*.04), (length-1-i)/(RATE*.04))
        frames.extend(struct.pack("<h",int(max(-1,min(1,sample(t)))*edge*17000)))
    with wave.open(str(OUT/(name+".wav")),"wb") as wav:
        wav.setparams((1,2,RATE,length,"NONE","not compressed"))
        wav.writeframes(frames)

def music(t):
    tones=(50,57,60,None,55,62,57,None,48,55,58,None,53,60,55,None)
    step=int(t/.5)%len(tones)
    note=tones[step]
    phase=t%.5
    bass=math.sin(math.tau*(73.416 if step<8 else 65.406)*t)*.16
    if note is None:return bass
    f=440*2**((note-69)/12)
    env=min(1,phase/.015)*math.exp(-phase*7)
    return bass+env*(.37*math.sin(math.tau*f*t)+.08*math.sin(math.tau*2*f*t))

if __name__=="__main__":
    render("shift-motif",8,music)
    render("yard-air",4,lambda t: .23*math.sin(math.tau*67*t)*(.5+.5*math.cos(math.tau*t/4))+.04*math.sin(math.tau*299*t))
    render("engine-room",4,lambda t: .37*math.sin(math.tau*48*t)+.16*math.sin(math.tau*96*t)*(1+.4*math.sin(math.tau*3*t)))
