#pragma once

#include "opencv2\opencv.hpp"
#include "opencv2\world.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\core\cvstd.hpp"

#include "NuiApi.h"
#include "NuiImageCamera.h"
#include "NuiSensor.h"
#include "NuiSkeleton.h"

#include "Globals.h"

int** getDepthImage(HANDLE h, IplImage* depth, int width, int height) {

	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);

	int** returnArray = new int*[width];
	for (int i = 0; i < width; i++) {
		returnArray[i] = new int[height];
	}

	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch == 0)
	{
		return returnArray;
	}
	USHORT * pBuff = (USHORT*)LockedRect.pBits;

	int minVal = 100000;
	int maxVal = -10000;

	const int MIN_DIST = 800;
	const int MAX_DIST = 3000;

	double range = MAX_DIST - MIN_DIST;
	double scale = range / 254.0;

	int channelCount = depth->nChannels;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int index = y * width + x;
			unsigned short pixelVal = pBuff[index] >> 3;
			int grayVal = (pixelVal - MIN_DIST) / scale + 1;

			if (pixelVal <= MIN_DIST)
				grayVal = 0;
			else if (pixelVal >= MAX_DIST)
				grayVal = 255;

			buf[channelCount * index] = buf[channelCount * index + 1] = buf[channelCount * index + 2] = grayVal;

			if (pixelVal > MIN_DIST && pixelVal < MAX_DIST) {
				returnArray[x][y] = pixelVal;
				if (pixelVal < minVal)
					minVal = pixelVal;
				if (pixelVal > maxVal)
					maxVal = pixelVal;
			}
			else if (pixelVal >= MAX_DIST) {
				returnArray[x][y] = MAX_DIST;
			}
			else if (pixelVal <= MIN_DIST) {
				returnArray[x][y] = MIN_DIST;
			}
		}
	}

	cvSetData(depth, buf, width * CHANNEL);
	cvShowImage("depth image", depth);
	NuiImageStreamReleaseFrame(h, pImageFrame);

	depthLockedRect = LockedRect;

	return returnArray;
}