#pragma once

#include <vector>
#include <utility>
#include "opencv2\opencv.hpp"
#include "KinectLayer.h"

#define COLOR_WIDTH 1920    
#define COLOR_HEIGHT 1080    
#define DEPTH_WIDTH 512    
#define DEPTH_HEIGHT 424      
#define CHANNEL 3

#define PI 3.14159265358979323846

using namespace std;

//const float a = 0.00173667;
const float fovDepthX = 70.6;
const float fovDepthY = 60;
const float fovColorX = 84.1;
const float fovColorY = 53.8;
const double generalTolerance = 0.05;

int clickedX = 1;
int clickedY = 1;
double* clickedPoint1 = nullptr;
double* clickedPoint2 = nullptr;
static vector<pair<string, double *>> pointVec;
map<string, double *> realCoordsMap;
static bool run;

cv::Mat color;
IplImage* depth;
int** depthImg = nullptr;
KinectLayer kinect;