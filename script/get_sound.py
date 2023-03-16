#script connecting to a lsl stream called Kistler 
#pull the data from the stream and play it as a sound

import pylsl
import numpy as np
import pyaudio
import math
import time


# get the stream
streams = pylsl.resolve_stream('name', 'Kistler')
inlet = pylsl.stream_inlet(streams[0])

fs = 12500

# set up the sound
p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paFloat32,
                channels=1,
                rate=fs,
                output=True)

# get the data and play it
ts =0
while True:
    chunk, timestamp = inlet.pull_chunk(1024)
    if(timestamp):
        print(timestamp[0]-ts)
        
        if timestamp[0] < ts:
            print('timestamp error')
            ts = timestamp[0]
        audio = np.array(chunk, dtype=np.float32).T
        audio = audio[0,:].copy(order='C'
        volume = 0.5  # range [0.0, 1.0]
        audio = (volume * audio).tobytes()
        stream.write(audio)
    #print(timestamp)
