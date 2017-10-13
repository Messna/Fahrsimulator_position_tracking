#include "stdafx.h"

#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <thread>

#include <vector>
#include <map>
#include <utility>

#include "Globals.h"
#include "DepthLib.h"
#include "ColorDetection.h"

#include <string>
#include <iostream>

#include <math.h>

#include "Kinect.h"
#include "KinectLayer.h"

#include "opencv2\opencv.hpp"
#include "opencv2\world.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\core\cvstd.hpp"
#include "Server.h"

using namespace std;

BYTE buf[DEPTH_WIDTH * DEPTH_HEIGHT * CHANNEL];

cv::Mat DrawCircleAtMiddle() {
	cv::circle(color, cv::Point(COLOR_WIDTH / 2, COLOR_HEIGHT / 2), 5, cv::Scalar(0, 255, 0));
	return color;
}

int drawColor() {
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.3, 0.3); 
	
	if (!pointVec.empty()) {
		for (std::pair<string, double*> e : pointVec) {
			cv::circle(color, cv::Point2d(e.second[3], e.second[4]), 2, cv::Scalar(0.0, 0.0, 0.0), -1);
			
			stringstream text;
			text << setprecision(3) << e.first << " X:" << e.second[0] << " Y:" << e.second[1] << " Z:" << e.second[2];
			cv::putText(color, text.str().c_str(), cv::Point2d(e.second[3] + 4, e.second[4] - 4), 1, 1, cv::Scalar(0.0, 0.0, 0.0));
		}
	}

	/*****************Find different colors and mark them on image*******************/
	//Color-Format = RBG
	int* rgb_target;

	for (auto const& p : colorMap) {
		findColorAndMark(p.second, p.first);
	}
	
	cv::imshow("color image", color);
	
	return 0;
}

int** getDepthImage(cv::Mat depthImg, IplImage* depth, int width, int height) {

	int** returnArray = new int*[width];
	for (int i = 0; i < width; i++) {
		returnArray[i] = new int[height];
	}

	/*int minVal = 100000;
	int maxVal = -10000;

	const int MIN_DIST = 800;
	const int MAX_DIST = 3000;

	double range = MAX_DIST - MIN_DIST;
	double scale = range / 254.0;

	int channelCount = depth->nChannels;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int index = y * width + x;
			unsigned short pixelVal = depthImg.at<unsigned short>(y, x);
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

	//cvSetData(depth, buf, width * CHANNEL);
	cv::imshow("depth image", depthImg);
	//cvShowImage("depth image", depth); */

	return returnArray;
}

static void writeDepthandColor() {
	double* colorAngleArr = GetAngleFromColorIndex(clickedX, clickedY);
	double* rdWorldPos = Get3DCoordinates(colorAngleArr);

	IplImage tmpColor = color;
	CvScalar color_pxl = cvGet2D(&tmpColor, clickedY, clickedX);

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
		cv::line(color, d2P1, d2P2, cv::Scalar(0.0, 0.0, 0.0), 2);
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		cv::putText(color, to_string(int(length)).c_str(), d2P2, 1, 1, cv::Scalar(0.0, 0.0, 0.0));
	//	std::cout << "Draw Line " << length << "at P1: " << clickedPoint1[0] << "/" << clickedPoint1[1] << " P1: " << clickedPoint2[0] << "/" << clickedPoint2[1] << std::endl;
	}

/*	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	cvPutText(color, "Test", textPos, &font, cv::Scalar(0.0, 0.0, 0.0)); */
}

static void onClick(int event, int x, int y, int f, void*) {
	if (event == CV_EVENT_LBUTTONDOWN) {
		IplImage tmpColor = color;
		CvScalar color_pxl = cvGet2D(&tmpColor, y, x);

		uint8_t rgb = uint8_t(color_pxl.val[0]), // B
			cg = uint8_t(color_pxl.val[1]), // G
			cb = uint8_t(color_pxl.val[2]); // R
		//	c4 = uint8_t(color_pxl.val[3]); // Alpha

		std::cout << "Color: B: " << (int)rgb << " G: " << (int)cg << " R: " << (int)cb << std::endl;

		double* colorAngleArr = GetAngleFromColorIndex(x, y);
		double* rdWorldPos = Get3DCoordinates(colorAngleArr);
		rdWorldPos[3] = x;
		rdWorldPos[4] = y;
		stringstream tmp;
		tmp << "P" << pointVec.size() + 1 << "(x: " << x << " y: " << y << ")";
		pointVec.push_back(make_pair(tmp.str(), rdWorldPos));

		int rec_x = x > 25 ? (x < COLOR_WIDTH - 25 ? x - 25 : COLOR_WIDTH - 50) : 1;
		int rec_y = y > 25 ? (y < COLOR_HEIGHT - 25 ? y - 25 : COLOR_HEIGHT - 50) : 1;

		colorMap["C" + to_string(colorMap.size())] = new int[5]{ cb, rgb, cg, rec_x, rec_y };
	}
	else if (event == CV_EVENT_RBUTTONDOWN) {
		if (!pointVec.empty())
			pointVec.pop_back();
	}
}

static void editColorValuesOfPoints(int point) {
	
}

int main(int argc, char * argv[]) {
	// Red
	//colorMap["C1"] = new int[3]{ 182, 50, 40 };

	//colorMap["C2"] = new int[3]{ 70, 121, 120 };

	//colorMap["C3"] = new int[3]{ 86, 133, 88 };

	//colorMap["C4"] = new int[3]{ 181, 120, 183 };

	depth = cvCreateImageHeader(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), IPL_DEPTH_8U, CHANNEL);

	cv::namedWindow("color image", CV_WINDOW_AUTOSIZE);

	cv::setMouseCallback("color image", onClick);


	thread serverThread(&startServer);
	cout << "Main thread" << endl;
	while (true)
	{
		kinect.setRGB();
	
		//depthImg = getDepthImage(kinect.depthImage, depth, kinect.depthImage.cols, kinect.depthImage.rows);
		color = kinect.rgbImage;
		drawColor();
		
		int c = cvWaitKey(1);

		if (c == 'c' || c == 'C')
			editColorValuesOfPoints(1);

		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

	run = false;
	serverThread.join();

	cvReleaseImageHeader(&depth);

	cv::destroyAllWindows();

	return 0;
}


