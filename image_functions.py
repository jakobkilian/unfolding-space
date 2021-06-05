#!/usr/bin/python3


import cv2
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
        numpyPoints = data.npoints()
        start = time.time()
        np_z_conf_points = numpyPoints[:,:,(RYL_DEPTH_Z_IDX,RYL_DEPTH_CONFIDENCE_IDX)]
        for q in self.queueList:
            q.put(np_z_conf_points)
        self.sumtime += time.time() - start


class DepthDataTransform():
    """Transforms a X x Y sized DepthData object into a AxA Motor Image"""

    def __init__(self, minConfidence=10, maxDepth=1.5, numTiles=3):
        """
            numTiles : the number of tiles in the resulting motor image
            maxDepth : maximum z value of depth point to consider, everything greater is out of range and will be set to 255
            minConfidence : minimum confidence for a point to be considered
        """

        self.numTiles = numTiles
        self.maxDepth = maxDepth
        self.minConfidence = minConfidence

    def transform(self, np_z_confidence):
        
        # shape of the input array is y,x,(z,confidence)
        height = np_z_confidence.shape[0]
        width  = np_z_confidence.shape[1]
        tileWidth = (width / self.numTiles) + 1
        tileHeight = (height / self.numTiles) + 1

        tiles = np.zeros( self.numTiles * self.numTiles).reshape(-1, self.numTiles)

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

        return values
        # astype(np.ubyte)
        # np.histogram(test2[0:3,0:3], bins=range(100,150))
#        for x_tile in range(0,self.numTiles):
#            for y_tile in range(0,self.numTiles):
#                #print("here")