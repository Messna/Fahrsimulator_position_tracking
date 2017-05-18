#include <vector>
#include <map>
#include <utility>

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

#define COLOR_WIDTH 640    
#define COLOR_HEIGHT 480    
#define DEPTH_WIDTH 320    
#define DEPTH_HEIGHT 240      
#define CHANNEL 3
using namespace std;
BYTE buf[DEPTH_WIDTH * DEPTH_HEIGHT * CHANNEL];

const float a = 0.00173667;
const float fovDepthX = 58.5;
const float fovDepthY = 46.6;
const float fovColorX = 62;
const float fovColorY = 48.6;
const double generalTolerance = 0.05;

int clickedX = 1;
int clickedY = 1;
double* clickedPoint1 = nullptr;
double* clickedPoint2 = nullptr;
bool point1 = false;

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

	//cout << " for coords (" << colorX << ";" << colorY << ") the degrees are (" << trueAngleX << ";" << trueAngleY << ")" << endl;

	returnArr[0] = trueAngleX;
	returnArr[1] = trueAngleY;
	return returnArr;
}

double* Get3DCoordinates(double* angles, int** depthArr) {

	double* realWorldCoords = new double[5] {-1000, -1000, -1000, 1, 1};

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

		double realWorldZ = depthValZ / 10.0; //convert from mm to cm
		double realWorldX = tan(GetRadianFromDegree(colorAngleX)) * realWorldZ;
		double realWorldY = tan(GetRadianFromDegree(colorAngleY)) * realWorldZ;
		realWorldCoords[0] = realWorldX;
		realWorldCoords[1] = realWorldY;
		realWorldCoords[2] = realWorldZ;

		//std::cout << "3D pos in cm: (" << realWorldX << ";" << realWorldY << ";" << realWorldZ << ")" << std::endl;
	}
	else {
		//std::cout << "3D pos in cm: ERROR: out of FoV!!! " << std::endl;
	}
	return realWorldCoords;
}

double GetLength(double* p1, double* p2) {
	double l1 = (p2[0] - p1[0]) * (p2[0] - p1[0]);
	double l2 = (p2[1] - p1[1]) * (p2[1] - p1[1]);
	double l3 = (p2[2] - p1[2]) * (p2[2] - p1[2]);
	return sqrt(l1 + l2 + l3);
}

bool has_target_color(double* target_color_max, double* target_color_min, CvScalar& color_pxl) {
	
	uint8_t green = uint8_t(color_pxl.val[0]),
		blue = uint8_t(color_pxl.val[1]),
		red = uint8_t(color_pxl.val[2]),
		c4 = uint8_t(color_pxl.val[3]);
	//target_color = RBG

	if (red >= target_color_min[0] && red <= target_color_max[0] &&
		green >= target_color_min[1] && green <= target_color_max[1] &&
		blue >= target_color_min[2] && blue <= target_color_max[2]) {
		return true;
	}
	
	return false;
}

void findNeighbors(int x, int y, double* target_color_max, 
					double* target_color_min, 
					std::map<std::pair<int, int>, bool>& stack) {
	
	if ((stack.find(std::make_pair(x, y)) == stack.end()) && 
			has_target_color(target_color_max, target_color_min, cvGet2D(color, y, x/1.333))) {
		stack[(std::make_pair(x, y))] = true;
		
		if (x > 0) {
			findNeighbors(x - 1, y, target_color_max, target_color_min, stack);
		}
		if (y > 0) {
			findNeighbors(x, y-1, target_color_max, target_color_min, stack);
		}
		if (x < color->width) {
			findNeighbors(x + 1, y, target_color_max, target_color_min, stack);
		}
		if (y < color->height) {
			findNeighbors(x, y + 1, target_color_max, target_color_min, stack);
		}
	}
}

void region_growing(int* start, double* target_color_max, double* target_color_min) {
	std::map<std::pair<int, int>, bool> stack;
	
	try
	{
		findNeighbors(start[0], start[1], target_color_max, target_color_min, stack);

	}
	catch (const std::exception &e)
	{
		std::cout << "Exception at findNeighbors-call" << std::endl;
	}
	long int sum_x = 0;
	long int sum_y = 0;
	if (stack.size() != 0) {
		for (auto a : stack) {
			sum_x += a.first.first;
			sum_y += a.first.second;
		}
		start[0] = sum_x / stack.size();
		start[1] = sum_y / stack.size();
	}
}
std::vector<int*> get_seed_coordinates2(double* target_color_max, double* target_color_min, int* target_color) {
	std::vector<int*> cont;
	int* best_pos = new int[2]{ 0, 0 };
	long int min_error = 255 * 255 * 255;
	int i = 0;
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

			if (red >= target_color_min[0] && red <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				blue >= target_color_min[2] && blue <= target_color_max[2]) {

				int x2 = x*1.333;
				int *a = new int[2]{ x2, y };
				cont.push_back(a);
				if (abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]) < min_error) {
					min_error = abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]);
					best_pos[0] = x2;
					best_pos[1] = y;
				}

				i++;
				//red_sum += red;
				//green_sum += green;
				//blue_sum += blue;
				//std::cout << "( R:" << red << ", G: " << green << ", B: " << blue << " ) found";
			}


			/*if(x == x_mid && y == y_mid){
			std::cout << "Middle-Color: ( R:" <<red << ", G: " << green << ", B: " << blue << std::endl;
			}*/

		}
	}

	//region_growing(best_pos, target_color_max, target_color_min, color);

	//std::cout << "Points found: " << i << endl;

	return cont;

}
int* get_seed_coordinates3(double* target_color_max, double* target_color_min, int* target_color) {


	int* best_pos = new int[2]{ 0, 0 };
	long int min_error = 255 * 255 * 255;
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
			
			if (red >= target_color_min[0] && red <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				blue >= target_color_min[2] && blue <= target_color_max[2]) {

				int x2 = x*1.333;

				if (abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]) < min_error) {
					min_error = abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]);
					best_pos[0] = x2;
					best_pos[1] = y;
				}
			}
		}
	}
	
	region_growing(best_pos, target_color_max, target_color_min);

	return best_pos;
}



void findColorAndMark(int* rgb_target, std::string s = "unknown", double toleranceFactor = generalTolerance) {
	
	double range = toleranceFactor * 255;
	double* rgb_min = new double[3]{ max(0.0, rgb_target[0] - range), max(0.0, rgb_target[1] - range), max(0.0, rgb_target[2] - range) };
	double* rgb_max = new double[3]{ min(255.0, rgb_target[0] + range), min(255.0, rgb_target[1] + range), min(255.0, rgb_target[2] + range) };
	cv::Point textPos(0, 0);

	
/*	
	std::vector<int*> result = get_seed_coordinates2(rgb_max, rgb_min, rgb_target, color);
	cv::Mat output_frame(cv::cvarrToMat(color));
	for (auto a : result) {

		cv::Point *target = new cv::Point(int(0.5 + a[0] * 0.75), a[1]);
		
		cvCircle(color, *target, 2, cv::Scalar(169, 169, 169));
		//cvCircle(color, *target, 2, cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));
		if (textPos.x < target->x &&textPos.y < target->y) {
			textPos.x = target->x + 2;
			textPos.y = target->y + 2;
		}
		delete target;
	}
	for (auto e : result) {
		delete e;
	}
	*/
	
	int * a = get_seed_coordinates3(rgb_max, rgb_min, rgb_target);
	cv::Mat output_frame(cv::cvarrToMat(color));


	cv::Point *target = new cv::Point(int(0.5 + a[0] * 0.75), a[1]);
	
	cvCircle(color, *target, 1, cv::Scalar(0, 0, 0));
	double* angle = GetAngleFromColorIndex(a[0], a[1]);
	double* realcoord = Get3DCoordinates(angle, depthImg);
	//cvCircle(color, *target, 2, cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));
	if (textPos.x < target->x &&textPos.y < target->y) {
		textPos.x = target->x + 2;
		textPos.y = target->y + 2;
	}
	delete target;
	


	

	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	std::ostringstream os;
	os << realcoord[2];
	std::string str = os.str();
	std::string outputStr = s.append(" ").append(str);
	//std::cout << "X: " << realcoord[0] << " Y: " << realcoord[1] << " Z: " << realcoord[2] << std::endl;
	cvPutText(color, outputStr.c_str(), textPos, &font, cv::Scalar(0.0, 0.0, 0.0));
	//std::cout << result[0][1] << " " << result[0][0] << std::endl;
	//std::cout << result.size() << std::endl;

	cvCircle(color, cv::Point(320, 240), 3, cv::Scalar(0, 255, 0));
	delete rgb_max;
	delete rgb_min;

	delete[] a;
}

IplImage* DrawCircleAtMiddle() {
	cvCircle(color, cv::Point(320, 240), 5, cv::Scalar(0, 255, 0));
	return color;
}

int drawColor(HANDLE h) {
	
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

	/*CvScalar color_pxl = cvGet2D(color, 240, 320);
	
	uint8_t rgb = uint8_t(color_pxl.val[0]),
		cg = uint8_t(color_pxl.val[1]),
		cb = uint8_t(color_pxl.val[2]),
		c4 = uint8_t(color_pxl.val[3]);
	std::cout << "G: "<< (int)rgb << " B: " <<(int)cg << " R: " << (int)cb << " " << (int)c4 << std::endl;
	*/
	/*****************Find different colors and mark them on image*******************/
	//Color-Format = RBG
	int* rgb_target;

	IplImage* tmp_color = nullptr;
	//rgb_target = new int[3]{ 140, 38, 31 };

	rgb_target = new int[3]{ 190, 75, 82 };

	findColorAndMark(rgb_target, "Red");
	delete[] rgb_target;


	//rgb_target = new int[3]{ 68, 115, 112 };
	rgb_target = new int[3]{ 90, 170, 170 };
	findColorAndMark(rgb_target, "Green");
	delete[] rgb_target;
	

	//rgb_target = new int[3]{ 70, 120, 70 };
	rgb_target = new int[3]{ 86, 133, 88 };
	findColorAndMark(rgb_target, "Blue");
	delete[] rgb_target;

	rgb_target = new int[3]{ 125, 25, 84 };
	findColorAndMark(rgb_target, "Yellow");
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
			else if (pixelVal >=  MAX_DIST)
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

static void onMouse(int event, int x, int y, int f, void*) {
	double* colorAngleArr = GetAngleFromColorIndex(x, y);
	double* rdWorldPos = Get3DCoordinates(colorAngleArr, depthImg);
	
	std::ostringstream os;
	os << rdWorldPos[2];
	std::string str = os.str();
	std::cout << "Mouse-Event: Depth: " << rdWorldPos[2] << std::endl;
	cv::Point textPos(0, 0);
	textPos.x = x;
	textPos.y = y;
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	cvPutText(color, "Test", textPos, &font, cv::Scalar(0.0, 0.0, 0.0));	
	
}



static void writeDepthandColor() {
	double* colorAngleArr = GetAngleFromColorIndex(clickedX, clickedY);
	double* rdWorldPos = Get3DCoordinates(colorAngleArr, depthImg);

	CvScalar color_pxl = cvGet2D(color, clickedY, clickedX);

	uint8_t rgb = uint8_t(color_pxl.val[0]),
		cg = uint8_t(color_pxl.val[1]),
		cb = uint8_t(color_pxl.val[2]),
		c4 = uint8_t(color_pxl.val[3]);
	//std::cout << "G: " << (int)rgb << " B: " << (int)cg << " R: " << (int)cb << " " << (int)c4 << std::endl;

	std::ostringstream os;
	os << rdWorldPos[2];
	std::string str = os.str();
	std::cout << "Coordinates: X: " << rdWorldPos[0] << " Y: " << rdWorldPos[1]<< " Z: " << rdWorldPos[2] 
			  << "\tColor: B: " << (int)rgb << " G: " << (int)cg << " R: " << (int)cb << " Alpha: " << (int)c4 << std::endl;
	cv::Point textPos(0, 0);
	textPos.x = clickedX+3;
	textPos.y = clickedY+3;

	if (clickedPoint1 != nullptr && clickedPoint2 != nullptr) {
		cv::Point2d d2P1 = cv::Point2d(clickedPoint1[3], clickedPoint1[4]);
		cv::Point2d d2P2 = cv::Point2d(clickedPoint2[3], clickedPoint2[4]);
		double length = GetLength(clickedPoint1, clickedPoint2);
		cvLine(color, d2P1, d2P2, cv::Scalar(0.0, 0.0, 0.0), 2);
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		cvPutText(color, to_string(int(length)).c_str(), d2P2, &font, cv::Scalar(0.0, 0.0, 0.0));
	//	std::cout << "Draw Line " << length << "at P1: " << clickedPoint1[0] << "/" << clickedPoint1[1] << " P1: " << clickedPoint2[0] << "/" << clickedPoint2[1] << std::endl;
	}

/*	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	cvPutText(color, "Test", textPos, &font, cv::Scalar(0.0, 0.0, 0.0)); */
}

static void onClick(int event, int x, int y, int f, void*) {
	if(event == CV_EVENT_LBUTTONDOWN){
		clickedX = x;
		clickedY = y;
		if (clickedPoint1 != nullptr && clickedPoint2 != nullptr) {
			clickedPoint1 = nullptr;
			clickedPoint2 = nullptr;
		}

		if (clickedPoint1 == nullptr) {
			double* colorAngleArr = GetAngleFromColorIndex(clickedX, clickedY);
			clickedPoint1 = Get3DCoordinates(colorAngleArr, depthImg);
			clickedPoint1[3] = x;
			clickedPoint1[4] = y;
		}
		else if (clickedPoint2 == nullptr) {
			double* colorAngleArr = GetAngleFromColorIndex(clickedX, clickedY);
			clickedPoint2 = Get3DCoordinates(colorAngleArr, depthImg);
			clickedPoint2[3] = x;
			clickedPoint2[4] = y;
		}
	}
}

int main(int argc, char * argv[]) {
	

	color = cvCreateImageHeader(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 4);

	depth = cvCreateImageHeader(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), IPL_DEPTH_8U, CHANNEL);

	cvNamedWindow("color image", CV_WINDOW_AUTOSIZE);

	cvNamedWindow("depth image", CV_WINDOW_AUTOSIZE);

	//cv::setMouseCallback("color image", onMouse);

	cv::setMouseCallback("color image", onClick);

	HRESULT hr = NuiInitialize(
		NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX
		| NUI_INITIALIZE_FLAG_USES_COLOR);

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

	while (1)
	{
		WaitForSingleObject(h3, INFINITE);
		depthImg = getDepthImage(h4, depth, 320, 240);
		WaitForSingleObject(h1, INFINITE);
		drawColor(h2);
		writeDepthandColor();
		int c = cvWaitKey(1);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

	cvReleaseImageHeader(&depth);
	cvReleaseImageHeader(&color);
	
	cvDestroyWindow("depth image");
	cvDestroyWindow("color image");

	NuiShutdown();

	return 0;
}


