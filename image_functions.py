#!/usr/bin/python3


import numpy as np
import roypy
import time

RYL_DEPTH_X_IDX = 0
RYL_DEPTH_Y_IDX = 1
RYL_DEPTH_Z_IDX = 2
RYL_DEPTH_NOISE_IDX = 3
RYL_DEPTH_GRAY_IDX = 4
RYL_DEPTH_CONFIDENCE_IDX = 5

class DataListener(roypy.IDepthDataListener):

    """ Receives data from libroyale and stuffs it into a bunch of
    queues. It's the resposibility of whoever assembled everything with 
    addQueue to make sure the data gets consumed."""
    
    def __init__(self):
        super(DataListener, self).__init__()
        self.queueList = []
        self.sumtime = 0
    
    def addQueue(self, q):
        self.queueList.append(q)

    def onNewData(self, data):
        start = time.time()
        numpyPoints = data.npoints()
        np_z_conf_points = numpyPoints[:,:,(RYL_DEPTH_Z_IDX,RYL_DEPTH_CONFIDENCE_IDX)]
        for q in self.queueList:
            q.put(np_z_conf_points)
        self.sumtime += time.time() - start


class DepthDataTransform():
    """Transforms a X x Y sized DepthData object into a AxA Motor Image"""

    def __init__(self, minConfidence=10, maxDepth=2.5, numTiles=3):
        """
            numTiles : the number of tiles in the resulting motor image
            maxDepth : maximum z value of depth point to consider, everything greater is out of range and will be set to 255
            minConfidence : minimum confidence for a point to be considered
        """

        self.numTiles = numTiles
        self.maxDepth = maxDepth
        self.minConfidence = minConfidence

        self.greyscaleFrameListener = []
        self.threebythreeListener = []

        self.sum_time_frame = 0
        self.sum_time_histo = 0

    def add3by3Listener( self, listener ):
        self.threebythreeListener.append(listener)

    def addGreyscaleFrameListener ( self, listener ):
        self.greyscaleFrameListener.append(listener)

    def transform(self, np_z_confidence):
        
        # TIMING
        startFrame = time.time()

        # shape of the input array is y,x,(z,confidence)
        height = np_z_confidence.shape[0]
        width  = np_z_confidence.shape[1]


        # This was:
        #
        # for x, y
        # if valid
        # (curPoint.z <= maxDepth
        #                 ? (unsigned char)(curPoint.z / maxDepth * 255.0f)
        #                 : 255)

        values     = (np_z_confidence[:,:,0] / self.maxDepth) * 255
        confident  = np_z_confidence[:,:,1] > self.minConfidence # True / False Array
        values[~confident] = 255 # set all the value we are not confident about to 255

        byte_values = values.clip(0,255).astype(np.ubyte)
        for listener in self.greyscaleFrameListener:
            listener.putFrame(byte_values)

        # TIMING
        self.sum_time_frame += time.time() - startFrame
        startHisto = time.time()

        tileWidth  = int(width / self.numTiles) + 1
        tileHeight = int(height / self.numTiles) + 1
        tiles = np.zeros( self.numTiles * self.numTiles).reshape(-1, self.numTiles)

        # astype(np.ubyte)
        # np.histogram(test2[0:3,0:3], bins=range(100,150))
        for x_tile in range(0,self.numTiles):
            for y_tile in range(0,self.numTiles):
                x = x_tile * tileWidth
                y = y_tile * tileWidth
                #print(x, y)
                #histogram = np.histogram(byte_values[x:x+tileWidth,y:y+tileHeight], bins=range(0,257)) # 257 : both range and histo bins are open ranges ...
                histogram = np.histogram(byte_values[x:x+tileWidth,y:y+tileHeight], bins=5) # 257 : both range and histo bins are open ranges ...
                #tiles[x_tile, y_tile] = histogram[1][0]
                sum = 0
                for i, count in enumerate(histogram[0]):
                    sum += count

#const int minObjSizeThresh = 90;  // the min number of pixels, an object must have
#                                  // (smaller objects might be noise)
                    THRESHOLD = 90
                    if sum > THRESHOLD:
                        #tiles[x_tile, y_tile] = i
                        tiles[x_tile, y_tile] = int(histogram[1][i])
                        #print(histogram[1])
                        break



        for listener in self.threebythreeListener:
            listener.putFrame(tiles)

        self.sum_time_histo += time.time() - startHisto