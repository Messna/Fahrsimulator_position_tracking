#pragma once

#include <vector>
#include <map>
#include <utility>

#include "opencv2\opencv.hpp"

#define COLOR_WIDTH 640    
#define COLOR_HEIGHT 480    
#define DEPTH_WIDTH 320    
#define DEPTH_HEIGHT 240      
#define CHANNEL 3

#define PI 3.14159265358979323846

using namespace std;

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
vector<std::pair<string, double *>> pointVec;
map<string, int *> colorMap;

IplImage* color;
IplImage* depth;
int** depthImg = nullptr;
