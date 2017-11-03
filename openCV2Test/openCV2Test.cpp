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

#include "Kinect.h"
#include "KinectLayer.h"

#include "opencv2\opencv.hpp"
#include "opencv2\world.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\core\cvstd.hpp"

#include "Server.h"
#include "ColorPixel.h"
#include "XMLWriter.h"

using namespace std;

BYTE buf[DEPTH_WIDTH * DEPTH_HEIGHT * CHANNEL];
map<string, ColorPixel> colorMap;
XMLWriter* writer = nullptr;

int drawColor() {
	if (!pointVec.empty()) {
		for (pair<string, double*> e : pointVec) {
			cv::circle(color, cv::Point2d(e.second[3], e.second[4]), 2, cv::Scalar(0.0, 0.0, 0.0), -1);
			
			stringstream text;
			text << setprecision(3) << e.first << " X:" << e.second[0] << " Y:" << e.second[1] << " Z:" << e.second[2];
			cv::putText(color, text.str().c_str(), cv::Point2d(e.second[3] + 4, e.second[4] - 4), 1, 1, cv::Scalar(0.0, 0.0, 0.0));
		}
	}

	for (auto & p : colorMap) {
		findColorAndMark(p.second, p.first);
	}
	
	imshow("color image", color);
	
	return 0;
}

static void addToColorMap(const ColorPixel pixel)
{
	colorMap["C" + to_string(colorMap.size() + 1)] = pixel;
	writer->AddPixel("C" + to_string(colorMap.size()), colorMap.at("C" + to_string(colorMap.size())));
}

static ColorPixel addPoint(const int x, const int y) {
	cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);

	const uint8_t blue = uint8_t(color_val[0]), // B
		green = uint8_t(color_val[1]), // G
		red = uint8_t(color_val[2]); // R
									 //	alpha = uint8_t(colorVal[3]); // Alpha

	cout << "Color: B: " << static_cast<int>(blue) << " G: " << static_cast<int>(green) << " R: " << static_cast<int>(red) << endl;

	double* colorAngleArr = GetAngleFromColorIndex(x, y);
	double* rdWorldPos = Get3DCoordinates(colorAngleArr);
	rdWorldPos[3] = x;
	rdWorldPos[4] = y;
	stringstream tmp;
	tmp << "P" << pointVec.size() + 1 << "(x: " << x << " y: " << y << ")";
	pointVec.push_back(make_pair(tmp.str(), rdWorldPos));

	int rec_x = x > 25 ? (x < COLOR_WIDTH - 25 ? x - 25 : COLOR_WIDTH - 50) : 1;
	int rec_y = y > 25 ? (y < COLOR_HEIGHT - 25 ? y - 25 : COLOR_HEIGHT - 50) : 1;

	return ColorPixel{ red, green, blue, rec_x, rec_y };
}

static void removePoint() {
	if (!pointVec.empty()) {
		pointVec.pop_back();
		writer->RemovePixel("C" + to_string(colorMap.size()));
		colorMap.erase(colorMap.find("C" + to_string(colorMap.size())));
	}
}

static void onClick(const int event, const int x, const int y, int f, void*) {
	if (event == CV_EVENT_LBUTTONDOWN) {
		addToColorMap(addPoint(x, y));
	}
	else if (event == CV_EVENT_RBUTTONDOWN) {
		removePoint();
	}
}

int main() {
	cv::namedWindow("color image", CV_WINDOW_AUTOSIZE);
	cv::setMouseCallback("color image", onClick);

	kinect.setDepth();
	kinect.setRGB(color);
	while (color.at<cv::Vec4b>(100, 100) == cv::Vec4b(0, 0, 0, 0)) // Check if Matrix is filled now
		kinect.setRGB(color);

	writer = new XMLWriter("Points.xml");
	colorMap = *(writer->getPixels());
	for (auto p : colorMap) {
		addPoint(p.second.x, p.second.y);
	}
	thread serverThread(&startServer);
	cout << "Main thread" << endl;
	while (true)
	{
		drawColor();

		kinect.setRGB(color);
		//depthImg = getDepthImage(kinect.depthImage, depth, kinect.depthImage.cols, kinect.depthImage.rows);

		int c = cvWaitKey(1);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

	run = false;
	serverThread.join();

	cv::destroyAllWindows();
	return 0;
}


