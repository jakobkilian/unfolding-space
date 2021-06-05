
# This file implements glove stuff.
import threading
import queue
import cv2
import numpy as np

class Video:
    def showFrame(self, frame):
        """ """
        cv2.imshow("Frame", frame.clip(0,255).astype(np.ubyte))
        ESC = 27
        if ESC == cv2.waitKey(1):
            return False
        return True

class VideoThread(threading.Thread):
    def __init__(self, video = Video()):
        super().__init__()
        self.video = video
        self.q = queue.Queue()

    def putFrame(self, frame):
        self.q.put(frame)

    def run(self):
        while True:
            try:
                frame = self.q.get(True, 1)
            except queue.Empty:
                break

            if not self.video.showFrame(frame):
                break
                