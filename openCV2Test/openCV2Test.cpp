// openCV2Test.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//
#include <vector>

#include "stdafx.h"

#include <Windows.h>
#include <iostream>

#include <math.h>

#include "NuiApi.h"
#include "NuiImageCamera.h"
#include "NuiSensor.h"
#include "NuiSkeleton.h"

#include "opencv2\opencv.hpp"
#include "opencv2\world.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\core\cvstd.hpp"
/*#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include<opencv2/objdetect/objdetect.hpp>
*/
#define COLOR_WIDTH 640    
#define COLOR_HEIGHT 480    
#define DEPTH_WIDTH 320    
#define DEPTH_HEIGHT 240    
#define SKELETON_WIDTH 640    
#define SKELETON_HEIGHT 480    
#define CHANNEL 3
using namespace std;
BYTE buf[DEPTH_WIDTH * DEPTH_HEIGHT * CHANNEL];

const float a = 0.00173667;
const float fovDepthX = 58.5;
const float fovDepthY = 46.6;
const float fovColorX = 62;
const float fovColorY = 48.6;

IplImage* color;
IplImage* depth;
int** depthImg;
NUI_LOCKED_RECT depthLockedRect;

#define PI 3.14159265358979323846

double GetRadianFromDegree(double angleInDegree) {
	return angleInDegree * PI / 180.0;
}

double GetDegreeFromRadian(double angleInRadian) {
	return angleInRadian * 180.0 / PI;
}

//return 2 angles, for horizontal (x) and vertical (y)
double* GetAngleFromColorIndex(int colorX, int colorY) {
	double coordMidX = COLOR_WIDTH / 2.0 - 0.5;
	double coordMidY = COLOR_HEIGHT / 2.0 - 0.5;
	double* returnArr = new double[2] {-100, -100};

	double width = tan(GetRadianFromDegree(fovColorX / 2.0)) * 2;
	double height = tan(GetRadianFromDegree(fovColorY / 2.0)) * 2;
	double widthStep = width / (COLOR_WIDTH - 1);
	double heightStep = height / (COLOR_HEIGHT - 1);

	double centeredX = colorX - coordMidX;
	double centeredY = colorY - coordMidY;

	double trueAngleX = GetDegreeFromRadian(atan(centeredX * widthStep));
	double trueAngleY = GetDegreeFromRadian(atan(centeredY * heightStep));

	cout << " for coords (" << colorX << ";" << colorY << ") the degrees are (" << trueAngleX << ";" << trueAngleY << ")" << endl;

	returnArr[0] = trueAngleX;
	returnArr[1] = trueAngleY;
	return returnArr;
}

double* Get3DCoordinates(double* angles, int** depthArr) {

	double* realWorldCoords = new double[3] {-1000, -1000, -1000};

	double colorAngleX = angles[0];
	double colorAngleY = angles[1];

	double width = tan(GetRadianFromDegree(fovDepthX / 2.0)) * 2;
	double height = tan(GetRadianFromDegree(fovDepthY / 2.0)) * 2;
	double widthStep = width / (DEPTH_WIDTH - 1);
	double heightStep = height / (DEPTH_HEIGHT - 1);

	double coordMidX = DEPTH_WIDTH / 2.0 - 0.5;
	double coordMidY = DEPTH_HEIGHT / 2.0 - 0.5;

	double distX = tan(GetRadianFromDegree(colorAngleX));
	double distY = tan(GetRadianFromDegree(colorAngleY));


	//calc index position in depth array
	int idxDepthX = (int)(distX / widthStep + coordMidX + 0.5);
	int idxDepthY = (int)(distY / heightStep + coordMidY + 0.5);

	//check range of index
	if (idxDepthX >= 0 && idxDepthX < DEPTH_WIDTH && idxDepthY >= 0 && idxDepthY < DEPTH_HEIGHT) {
		double depthValZ = depthArr[idxDepthX][idxDepthY];
		//TODO check correction!!!
		depthValZ *= 1.204;


		double realWorldZ = depthValZ / 10.0; //convert from mm to cm
		double realWorldX = tan(GetRadianFromDegree(colorAngleX)) * realWorldZ;
		double realWorldY = tan(GetRadianFromDegree(colorAngleY)) * realWorldZ;
		realWorldCoords[0] = realWorldX;
		realWorldCoords[1] = realWorldY;
		realWorldCoords[2] = realWorldZ;

		std::cout << "3D pos in cm: (" << realWorldX << ";" << realWorldY << ";" << realWorldZ << ")" << std::endl;

	}
	else {
		std::cout << "3D pos in cm: ERROR: out of FoV!!! " << std::endl;
		return realWorldCoords;
	}
		
}



std::vector<int*> get_seed_coordinates2(double* target_color_max, double* target_color_min, int* target_color, IplImage* color) {

	std::vector<int*> cont;
	int* best_pos = new int[2]{ 0, 0 };
	//long int min_error = 255 * 255 * 3;
	int i= 0;
	double red_sum = 0.0;
	double blue_sum = 0.0;
	double green_sum = 0.0;
	

	for (int x = 0; x < color->width; x++) {
		for (int y = 0; y < color->height; y++) {
			CvScalar color_pxl = cvGet2D(color, y, x);
			uint8_t green = uint8_t(color_pxl.val[0]),
				blue = uint8_t(color_pxl.val[1]),
				red = uint8_t(color_pxl.val[2]),
				c4 = uint8_t(color_pxl.val[3]);
			//target_color = RBG
			if(abs(blue-target_color[1])) //findbest color 

			if (red >= target_color_min[0] && red <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				blue >= target_color_min[2] && blue <= target_color_max[2]) {
				int x2 = x*1.333;
				int *a = new int[2]{ x2, y };
				cont.push_back(a);
				i++;
				red_sum += red;
				green_sum += green;
				blue_sum += blue;
				//std::cout << "( R:" << red << ", G: " << green << ", B: " << blue << " ) found";
			}
			
			
			/*if(x == x_mid && y == y_mid){
					std::cout << "Middle-Color: ( R:" <<red << ", G: " << green << ", B: " << blue << std::endl;
			}*/
		
		}
	}
	

	//std::cout << "Points found: " << i << endl;

	return cont;
}


IplImage* findColorAndMark(int* rgb_target, IplImage* color, std::string s = "unknown") {
	double toleranceFactor = 0.1;
	double range = toleranceFactor * 255;
	double* rgb_min = new double[3]{ max(0.0, rgb_target[0] - range), max(0.0, rgb_target[1] - range), max(0.0, rgb_target[2] - range) };
	double* rgb_max = new double[3]{ min(255.0, rgb_target[0] + range), min(255.0, rgb_target[1] + range), min(255.0, rgb_target[2] + range) };
	cv::Point textPos(0, 0);

	std::vector<int*> result = get_seed_coordinates2(rgb_max, rgb_min, rgb_target, color);
	cv::Mat output_frame(cv::cvarrToMat(color));
	for (auto a : result) {

		cv::Point *target = new cv::Point(int(0.5 + a[0] * 0.75), a[1]);
		cvCircle(color, *target, 2, cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));
		if (textPos.x < target->x &&textPos.y < target->y) {
			textPos.x = target->x + 2;
			textPos.y = target->y + 2;
		}
		delete target;
	}

	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

	cvPutText(color, s.c_str(), textPos, &font, cv::Scalar(0.0, 0.0, 0.0));
	//std::cout << result[0][1] << " " << result[0][0] << std::endl;
	//std::cout << result.size() << std::endl;

	for (auto e : result) {
		delete e;
	}
	cvCircle(color, cv::Point(320, 240), 10, cv::Scalar(0, 255, 0));
	delete rgb_max;
	delete rgb_min;

	

	return color;

}

IplImage* DrawCircleAtMiddle(IplImage* color) {
	cvCircle(color, cv::Point(320, 240), 10, cv::Scalar(0, 255, 0));
	return color;
}

int drawColor(HANDLE h, IplImage* color) {
	
	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);
	if (FAILED(hr))
	{
		cout << "Get Image Frame Failed" << endl;
		return -1;
	}
	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;

	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{
		BYTE * pBuffer = (BYTE*)LockedRect.pBits;
		cvSetData(color, pBuffer, LockedRect.Pitch);
	
	}

	CvScalar color_pxl = cvGet2D(color, 240, 320);
	
	uint8_t rgb = uint8_t(color_pxl.val[0]),
		cg = uint8_t(color_pxl.val[1]),
		cb = uint8_t(color_pxl.val[2]),
		c4 = uint8_t(color_pxl.val[3]);
	//std::cout << "G: "<< (int)rgb << " B: " <<(int)cg << " R: " << (int)cb << " " << (int)c4 << std::endl;

	/*****************Find different colors and mark them on image*******************/
	int* rgb_target;
	rgb_target = new int[3]{ 190, 70, 60 };
	color = findColorAndMark(rgb_target, color, "Red");
	delete[] rgb_target;

	
	rgb_target = new int[3]{ 74, 94, 154 };
	color = findColorAndMark(rgb_target, color, "Green");
	delete[] rgb_target;
	
	rgb_target = new int[3]{ 52, 111, 65 };
	color = findColorAndMark(rgb_target, color, "Blue");
	delete[] rgb_target;
	
	/*****************Find different colors and mark them on image*******************/
	
	cvShowImage("color image", color);
	
	NuiImageStreamReleaseFrame(h, pImageFrame);
	return 0;
}

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

	//std::cout << "DEPTH img w=" << depth->width << " h=" << depth->height << " #channels=" << depth->nChannels << std::endl;

	int minVal = 100000;
	int maxVal = -10000;
	double sum = 0.0;
	int count = 0;

	const int MIN_DIST = 6400;
	const int MAX_DIST = 31800;

	double range = MAX_DIST - MIN_DIST;
	double scale = range / 254.0;

	int channelCount = depth->nChannels;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int index = y * width + x;
			unsigned short pixelVal = pBuff[index];
			int grayVal = (pixelVal - MIN_DIST) / scale + 1;

			if (pixelVal <= MIN_DIST) {
				grayVal = 0;
			}
			else if (pixelVal >=  MAX_DIST) {
				grayVal = 255;
			}

			buf[channelCount * index] = buf[channelCount * index + 1] = buf[channelCount * index + 2] = grayVal;


			if (pixelVal > MIN_DIST && pixelVal < MAX_DIST) {
				returnArray[x][y] = pixelVal;
				sum += pixelVal;
				if (pixelVal < minVal) {
					minVal = pixelVal;
				}
				if (pixelVal > maxVal) {
					maxVal = pixelVal;
				}
				count++;
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
	NuiImageStreamReleaseFrame(h, pImageFrame);
	cvShowImage("depth image", depth);

	depthLockedRect = LockedRect;

	sum /= (count);
	//std::cout << "# valid pixels = " << count << " avg = " << sum << " min=" << minVal << " maX=" << maxVal << std::endl;

	return returnArray;
}


//int drawDepth(HANDLE h, IplImage* depth) {
//	const NUI_IMAGE_FRAME * pImageFrame = NULL;
//	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);
//	if (FAILED(hr))
//	{
//		cout << "Get Image Frame Failed" << endl;
//		return -1;
//	}
//	//  temp1 = depth;
//	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
//	NUI_LOCKED_RECT LockedRect;
//	pTexture->LockRect(0, &LockedRect, NULL, 0);
//	if (LockedRect.Pitch != 0)
//	{
//		USHORT * pBuff = (USHORT*)LockedRect.pBits;
//		for (int i = 0; i < DEPTH_WIDTH * DEPTH_HIGHT; i++)
//		{
//			BYTE index = pBuff[i] & 0x07;
//			USHORT realDepth = (pBuff[i] & 0xFFF8) >> 3;
//			BYTE scale = 255 - (BYTE)(256 * realDepth / 0x0fff);
//
//			buf[CHANNEL * i] = buf[CHANNEL * i + 1] = buf[CHANNEL * i + 2] = 0;
//			switch (index)
//			{
//			case 0:
//				buf[CHANNEL * i] = scale / 2;
//				buf[CHANNEL * i + 1] = scale / 2;
//				buf[CHANNEL * i + 2] = scale / 2;
//				break;
//			case 1:
//				buf[CHANNEL * i] = scale;
//				break;
//			case 2:
//				buf[CHANNEL * i + 1] = scale;
//				break;
//			case 3:
//				buf[CHANNEL * i + 2] = scale;
//				break;
//			case 4:
//				buf[CHANNEL * i] = scale;
//				buf[CHANNEL * i + 1] = scale;
//				break;
//			case 5:
//				buf[CHANNEL * i] = scale;
//				buf[CHANNEL * i + 2] = scale;
//				break;
//			case 6:
//				buf[CHANNEL * i + 1] = scale;
//				buf[CHANNEL * i + 2] = scale;
//				break;
//			case 7:
//				buf[CHANNEL * i] = 255 - scale / 2;
//				buf[CHANNEL * i + 1] = 255 - scale / 2;
//				buf[CHANNEL * i + 2] = 255 - scale / 2;
//				break;
//			}
//		}
//		cvSetData(depth, buf, DEPTH_WIDTH * CHANNEL);
//	}
//	NuiImageStreamReleaseFrame(h, pImageFrame);
//	cvShowImage("depth image", depth);
//	
//	depthLockedRect = LockedRect;
//	return 0;
//}

int drawSkeleton(IplImage* skeleton) {
	NUI_SKELETON_FRAME SkeletonFrame;
	CvPoint pt[20];
	HRESULT hr = NuiSkeletonGetNextFrame(0, &SkeletonFrame);
	bool bFoundSkeleton = false;
	for (int i = 0; i < NUI_SKELETON_COUNT; i++)
	{
		if (SkeletonFrame.SkeletonData[i].eTrackingState
			== NUI_SKELETON_TRACKED)
		{
			bFoundSkeleton = true;
		}
	}
	// Has skeletons!
	//
	if (bFoundSkeleton)
	{
		NuiTransformSmooth(&SkeletonFrame, NULL);
		memset(skeleton->imageData, 0, skeleton->imageSize);
		for (int i = 0; i < NUI_SKELETON_COUNT; i++)
		{
			if (SkeletonFrame.SkeletonData[i].eTrackingState
				== NUI_SKELETON_TRACKED)
			{
				for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)
				{
					float fx, fy;
					NuiTransformSkeletonToDepthImage(
						SkeletonFrame.SkeletonData[i].SkeletonPositions[j],
						&fx, &fy);
					pt[j].x = (int)(fx * SKELETON_WIDTH + 0.5f);
					pt[j].y = (int)(fy * SKELETON_HEIGHT + 0.5f);
					cvCircle(skeleton, pt[j], 5, CV_RGB(255, 0, 0), -1);
				}

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HEAD],
					pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],
					pt[NUI_SKELETON_POSITION_SPINE], CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_SPINE],
					pt[NUI_SKELETON_POSITION_HIP_CENTER],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HAND_RIGHT],
					pt[NUI_SKELETON_POSITION_WRIST_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_WRIST_RIGHT],
					pt[NUI_SKELETON_POSITION_ELBOW_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_ELBOW_RIGHT],
					pt[NUI_SKELETON_POSITION_SHOULDER_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_SHOULDER_RIGHT],
					pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],
					pt[NUI_SKELETON_POSITION_SHOULDER_LEFT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_SHOULDER_LEFT],
					pt[NUI_SKELETON_POSITION_ELBOW_LEFT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_ELBOW_LEFT],
					pt[NUI_SKELETON_POSITION_WRIST_LEFT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_WRIST_LEFT],
					pt[NUI_SKELETON_POSITION_HAND_LEFT], CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HIP_CENTER],
					pt[NUI_SKELETON_POSITION_HIP_RIGHT], CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HIP_RIGHT],
					pt[NUI_SKELETON_POSITION_KNEE_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_KNEE_RIGHT],
					pt[NUI_SKELETON_POSITION_ANKLE_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_ANKLE_RIGHT],
					pt[NUI_SKELETON_POSITION_FOOT_RIGHT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HIP_CENTER],
					pt[NUI_SKELETON_POSITION_HIP_LEFT], CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_HIP_LEFT],
					pt[NUI_SKELETON_POSITION_KNEE_LEFT], CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_KNEE_LEFT],
					pt[NUI_SKELETON_POSITION_ANKLE_LEFT],
					CV_RGB(0, 255, 0));

				cvLine(skeleton, pt[NUI_SKELETON_POSITION_ANKLE_LEFT],
					pt[NUI_SKELETON_POSITION_FOOT_LEFT], CV_RGB(0, 255, 0));
			}
		}
	}
	cvShowImage("skeleton image", skeleton);
	return 0;
}

//double* getPoint3DFromDepthCoordinates(double x, double y, double z) {
//	double pixelPerAngle = 58.5 / 320;
//	double angleRange = x - 58.5 / 2 + pixelPerAngle * x;
//	
//}

double calcRealX(int x, double z) {
	return x - 160 + (x - 160) * a * z;
}

double calcRealY(int y, double z) {
	return y - 120 + (y - 120) * a * z;
}

static void onMouse(int event, int x, int y, int f, void*) {
	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	//cvPutText(color, "test", CvPoint(100, 100), &font, cv::Scalar(0.0, 0.0, 0.0));	
	int depthX = x / 2;
	int depthY = y / 2;

	double depthVal = depthImg[depthX][depthY] / 100.0;

	cout << depthX << ", " << depthY << " , Real: Z:" << depthVal << ", X:" << calcRealX(depthX, depthVal) << ", Y:" << calcRealY(depthY, depthVal) << endl;

		double* colorAngleArr = GetAngleFromColorIndex(x, y);
		double* rdWorldPos = Get3DCoordinates(colorAngleArr, depthImg);
}

int main(int argc, char * argv[]) {
	

	color = cvCreateImageHeader(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 4);

	depth = cvCreateImageHeader(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), IPL_DEPTH_8U, CHANNEL);

	//IplImage* skeleton = cvCreateImage(cvSize(SKELETON_WIDTH, SKELETON_HIGHT), IPL_DEPTH_8U, CHANNEL);

	cvNamedWindow("color image", CV_WINDOW_AUTOSIZE);

	cvNamedWindow("depth image", CV_WINDOW_AUTOSIZE);

	cv::setMouseCallback("color image", onMouse);

	//cvNamedWindow("skeleton image", CV_WINDOW_AUTOSIZE);

	HRESULT hr = NuiInitialize(
		NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX
		| NUI_INITIALIZE_FLAG_USES_COLOR
		/*| NUI_INITIALIZE_FLAG_USES_SKELETON*/);

	if (hr != S_OK)
	{
		cout << "NuiInitialize failed" << endl;
		return hr;
	}

	HANDLE h1 = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE h2 = NULL;
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480,
		0, 2, h1, &h2);

	if (FAILED(hr))
	{
		cout << "Could not open image stream video" << endl;
		return hr;
	}

	HANDLE h3 = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE h4 = NULL;
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,
		NUI_IMAGE_RESOLUTION_320x240, 0, 2, h3, &h4);
	if (FAILED(hr))
	{
		cout << "Could not open depth stream video" << endl;
		return hr;
	}

	HANDLE h5 = CreateEvent(NULL, TRUE, FALSE, NULL);
	hr = NuiSkeletonTrackingEnable(h5, 0);
	if (FAILED(hr))
	{
		cout << "Could not open skeleton stream video" << endl;
		return hr;
	}

	while (1)
	{
		WaitForSingleObject(h1, INFINITE);
		drawColor(h2, color);
		WaitForSingleObject(h3, INFINITE);
		depthImg = getDepthImage(h4, depth, 320, 240);
		//WaitForSingleObject(h5, INFINITE);
		//drawSkeleton(skeleton);

		//exit
		int c = cvWaitKey(1);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

	cvReleaseImageHeader(&depth);
	cvReleaseImageHeader(&color);
	
	//cvReleaseImage(&skeleton);
	cvDestroyWindow("depth image");
	cvDestroyWindow("color image");
	//cvDestroyWindow("skeleton image");

	NuiShutdown();

	return 0;
}


