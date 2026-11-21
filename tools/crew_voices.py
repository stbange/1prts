"""Regenerate the original radio readings using the system Flite and its CMU US voices."""
from ctypes import CDLL, c_char_p, c_float, c_void_p
from pathlib import Path
import math
import struct
import tempfile
import wave

sounds = Path(__file__).resolve().parents[1] / "assets" / "sounds"
engine = CDLL("libflite.so.1")
engine.flite_init()
engine.flite_text_to_speech.argtypes = [c_char_p, c_void_p, c_char_p]
engine.flite_text_to_speech.restype = c_float
with wave.open(str(sounds / "ack.wav")) as reference:
    output_rate = reference.getframerate()

lines = [
    ("weir", "rms", "Assembly watch. Order received."),
    ("sounding", "slt", "Compact harbor. We have your order."),
    ("terrace", "awb", "Exchange dispatch. Crew on the line."),
    ("salt", "slt", "Field survey. We read you."),
    ("lantern", "rms", "Flight watch. Logged and ready."),
    ("meridian", "awb", "Collar control. Order received."),
]
for name, voice, line in lines:
    library = CDLL("libflite_cmu_us_" + voice + ".so.1")
    register = getattr(library, "register_cmu_us_" + voice)
    register.restype = c_void_p
    with tempfile.TemporaryDirectory() as directory:
        raw = Path(directory) / "voice.wav"
        engine.flite_text_to_speech(line.encode(), register(None), bytes(raw))
        with wave.open(str(raw)) as source:
            rate = source.getframerate()
            count = source.getnframes()
            samples = struct.unpack("<" + "h" * count, source.readframes(count))
        filtered = []
        low = high = 0
        for i in range(int(count * output_rate / rate)):
            sample = samples[min(count - 1, int(i * rate / output_rate))]
            low += .10 * (sample - low)
            high += .62 * (sample - low - high)
            filtered.append(int(10500 * math.tanh(high / 10000)))
        with wave.open(str(sounds / (name + ".wav")), "wb") as target:
            target.setparams((1, 2, output_rate, 0, "NONE", ""))
            target.writeframes(struct.pack("<" + "h" * len(filtered), *filtered))
