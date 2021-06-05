# This file implements glove stuff.
import threading
import queue

class Glove:
    def vibrate(self, arr):
        
        """ expects a 3x3 array of values 0..255 and does something with
        it. In the final use case, this is vibrating the motors, but a glove
        can also be virtual for debugging or visualizing output."""

        print(arr)

class GloveThread(threading.Thread):
    def __init__(self, glove = Glove()):
        super().__init__()
        self.glove = glove
        self.q = queue.Queue()

    def putFrame(self, frame):
        self.q.put(frame)

    def run(self):
        while True:
            try:
                frame = self.q.get(True, 1)
            except queue.Empty:
                break

            self.glove.vibrate(frame)
                
