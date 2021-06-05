


import argparse
import queue
import roypy
import sys
import time
import numpy as np

from image_functions import DataListener, DepthDataTransform

from roypy_sample_utils import CameraOpener, add_camera_opener_options

import glove
import video



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

#            if count % 50 == 0:
#                print(item.shape)
#                print(item)
            transform.transform(item)
            sumtime += time.time() - start

#            if count % 50 == 0:
#                print(values.shape)
#                print(values)

            if count == 100:
                print(sumtime)
                break

            count += 1

            #paint(values)
            #ESC = 27
            #if  ESC == cv2.waitKey(1):
            #    break
            


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
    gloveThread = glove.GloveThread()
    videoThread = video.VideoThread()

    q = queue.Queue()
    dataListener.addQueue(q)

    cam.registerDataListener(dataListener)
    cam.startCapture()

    transform = DepthDataTransform()
    transform.addGreyscaleFrameListener(videoThread)
    transform.add3by3Listener(gloveThread)
    
    videoThread.start()
    gloveThread.start()

    process_evt_q(q, transform)
    print("onNewData sumtime")
    print(dataListener.sumtime)

    print("frame: ", transform.sum_time_frame)
    print("histo: ", transform.sum_time_histo)

    


if (__name__ == "__main__"):
    main()