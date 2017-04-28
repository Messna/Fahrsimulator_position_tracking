// openCV2Test.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//
#include <vector>

#include "stdafx.h"

#include <Windows.h>
#include <iostream>

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
#define COLOR_HIGHT 480    
#define DEPTH_WIDTH 320    
#define DEPTH_HIGHT 240    
#define SKELETON_WIDTH 640    
#define SKELETON_HIGHT 480    
#define CHANNEL 3
using namespace std;
BYTE buf[DEPTH_WIDTH * DEPTH_HIGHT * CHANNEL];

const float a = 0.00173667;
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
	

	std::cout << "Points found: " << i << endl;

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
	std::cout << "G: "<< (int)rgb << " B: " <<(int)cg << " R: " << (int)cb << " " << (int)c4 << std::endl;
	//color =	DrawCircleAtMiddle(color);

	/*****************Find different colors and mark them on image*******************/
	int* rgb_target;
	IplImage* tmp_color = nullptr;
	rgb_target = new int[3]{ 190, 70, 60 };
	color = findColorAndMark(rgb_target, color, "Red");
	//delete color;
	//color = tmp_color;
	delete[] rgb_target;

	
	rgb_target = new int[3]{ 74, 94, 154 };
	color = findColorAndMark(rgb_target, color, "Green");
	//delete color;
	//color = tmp_color;
	delete[] rgb_target;
	
	rgb_target = new int[3]{ 52, 111, 65 };
	color = findColorAndMark(rgb_target, color, "Blue");
	//delete color;
	//color = tmp_color;
	delete[] rgb_target;
	
	/*****************Find different colors and mark them on image*******************/
	
	cvShowImage("color image", color);
	
	NuiImageStreamReleaseFrame(h, pImageFrame);
	return 0;
}

int drawDepth(HANDLE h, IplImage* depth) {
	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame(h, 0, &pImageFrame);
	if (FAILED(hr))
	{
		cout << "Get Image Frame Failed" << endl;
		return -1;
	}
	//  temp1 = depth;
	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect(0, &LockedRect, NULL, 0);
	if (LockedRect.Pitch != 0)
	{
		USHORT * pBuff = (USHORT*)LockedRect.pBits;
		for (int i = 0; i < DEPTH_WIDTH * DEPTH_HIGHT; i++)
		{
			BYTE index = pBuff[i] & 0x07;
			USHORT realDepth = (pBuff[i] & 0xFFF8) >> 3;
			BYTE scale = 255 - (BYTE)(256 * realDepth / 0x0fff);
			buf[CHANNEL * i] = buf[CHANNEL * i + 1] = buf[CHANNEL * i + 2] = 0;
			switch (index)
			{
			case 0:
				buf[CHANNEL * i] = scale / 2;
				buf[CHANNEL * i + 1] = scale / 2;
				buf[CHANNEL * i + 2] = scale / 2;
				break;
			case 1:
				buf[CHANNEL * i] = scale;
				break;
			case 2:
				buf[CHANNEL * i + 1] = scale;
				break;
			case 3:
				buf[CHANNEL * i + 2] = scale;
				break;
			case 4:
				buf[CHANNEL * i] = scale;
				buf[CHANNEL * i + 1] = scale;
				break;
			case 5:
				buf[CHANNEL * i] = scale;
				buf[CHANNEL * i + 2] = scale;
				break;
			case 6:
				buf[CHANNEL * i + 1] = scale;
				buf[CHANNEL * i + 2] = scale;
				break;
			case 7:
				buf[CHANNEL * i] = 255 - scale / 2;
				buf[CHANNEL * i + 1] = 255 - scale / 2;
				buf[CHANNEL * i + 2] = 255 - scale / 2;
				break;
			}
		}
		cvSetData(depth, buf, DEPTH_WIDTH * CHANNEL);
	}
	NuiImageStreamReleaseFrame(h, pImageFrame);
	cvShowImage("depth image", depth);
	
	return 0;
}

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
					pt[j].y = (int)(fy * SKELETON_HIGHT + 0.5f);
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

int calcRealX(int x, int z) {
	return (x - 320) * a * z;
}

int calcRealY(int y, int z) {
	return (y - 320) * a * z;
}

static void onMouse(int event, int x, int y, int f, void*) {
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	cvPutText(color, s.c_str(), textPos, &font, cv::Scalar(0.0, 0.0, 0.0));
	std::cout << x << ", " << y << " - " << calcRealX(x, 50) << ", " << calcRealY(y, 50) << std::endl;
}

int main(int argc, char * argv[]) {
	

	IplImage* color = cvCreateImageHeader(cvSize(COLOR_WIDTH, COLOR_HIGHT), IPL_DEPTH_8U, 4);

	//IplImage* depth = cvCreateImageHeader(cvSize(DEPTH_WIDTH, DEPTH_HIGHT), IPL_DEPTH_8U, CHANNEL);

	//IplImage* skeleton = cvCreateImage(cvSize(SKELETON_WIDTH, SKELETON_HIGHT), IPL_DEPTH_8U, CHANNEL);

	cvNamedWindow("color image", CV_WINDOW_AUTOSIZE);

	//cvNamedWindow("depth image", CV_WINDOW_AUTOSIZE);

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
		delete color->imageData;
//		WaitForSingleObject(h3, INFINITE);
//		drawDepth(h4, depth);
		//WaitForSingleObject(h5, INFINITE);
		//drawSkeleton(skeleton);

		//exit
		int c = cvWaitKey(1);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

//	cvReleaseImageHeader(&depth);
	cvReleaseImageHeader(&color);
	
	//cvReleaseImage(&skeleton);
//	cvDestroyWindow("depth image");
	cvDestroyWindow("color image");
	//cvDestroyWindow("skeleton image");

	NuiShutdown();

	return 0;

	//cv::VideoCapture cap('0'); // open the default camera
	//if (!cap.isOpened())  // check if we succeeded
	//	return -1;

	//cv::Mat edges;
	//cv::namedWindow("edges", 1);
	//for (;;)
	//{
	//	cv::Mat frame;
	//	cap >> frame; // get a new frame from camera
	//	cvtColor(frame, edges, CV_BGR2HSV);
	//	GaussianBlur(edges, edges, cv::Size(7, 7), 1.5, 1.5);
	//	Canny(edges, edges, 0, 30, 3);
	//	imshow("edges", edges);
	//	std::cout << "Output" << std::endl;
	//	//if (cv::waitKey(300) >= 0) break;
	//}
	//std::cin.get();
	//return 0;
}


