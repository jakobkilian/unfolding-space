


import argparse
import cv2
import queue
import roypy
import sys
import time
from image_functions import DataListener, DepthDataTransform

from roypy_sample_utils import CameraOpener, add_camera_opener_options

def paint(data):
    # data at this point is [x X y X 0..255]
    cv2.imshow("Data", data)


def process_evt_q(q, transform):
    count = 0
    sumtime = 0
    while True:
        try:
            if len(q.queue) == 0:
                item = q.get(True, 1)
            else:
                for _ in range(0,len(q.queue)):
                    item = q.get(True,1)
        except queue.Empty:
            # indicates a timeout
            break
        else:
            #print(type(item))
            #print(item)
            start = time.time()
            values = transform.transform(item)
            sumtime += time.time() - start
            count += 1

            if count % 50 == 0:
                print(values)

            if count == 100:
                print(sumtime)
                break

            paint(values)
            ESC = 27
            if  ESC == cv2.waitKey(1):
                break
            


def main():
    parser = argparse.ArgumentParser(usage = __doc__)
    add_camera_opener_options(parser)
    options = parser.parse_args()

    opener = CameraOpener(options)

    try:
        cam = opener.open_camera ()
    except:
        print("could not open Camera Interface")
        sys.exit(1)

    try:
        # retrieve the interface that is available for recordings
        replay = cam.asReplay()
        print ("Using a recording")
        print ("Framecount : ", replay.frameCount())
        print ("File version : ", replay.getFileVersion())
    except SystemError:
        print ("Using a live camera")

    dataListener = DataListener()

    q = queue.Queue()
    dataListener.addQueue(q)

    cam.registerDataListener(dataListener)
    cam.startCapture()

    transform = DepthDataTransform()

    process_evt_q(q, transform)
    print("DL sumtime")
    print(dataListener.sumtime)

    


if (__name__ == "__main__"):
    main()