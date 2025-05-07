import wave
import random
import struct
import time

from pvrecorder import PvRecorder

loop = 1
text = ["Whats_my_position", "Whats_my_fuel_level"]
recorder = PvRecorder(device_index=-1, frame_length=512)

for index, device in enumerate(PvRecorder.get_available_devices()):
  print(f"[{index}] {device}")
input()

while True:
  loop = loop + 1

  for i in range(0, 2):
    recording = []

    print(f'Press enter and say: {text[i]}, then again')
    input()

    try:
      recorder.start()

      while True:
        frame = recorder.read()
        recording.extend(frame)

    except KeyboardInterrupt:
      recorder.stop()

      print("Creating wave file")
      with wave.open(f'./data/{text[i]}/recording_{time.time()}.wav', 'w') as f:
        f.setparams((1, 2, 16000, 512, "NONE", "NONE"))
        f.writeframes(struct.pack("h" * len(recording), *recording))
