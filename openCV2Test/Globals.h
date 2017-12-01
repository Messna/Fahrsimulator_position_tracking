#pragma once

#include <vector>
#include <utility>
#include "opencv2\opencv.hpp"
#include "KinectLayer.h"

#define COLOR_WIDTH 512    
#define COLOR_HEIGHT 424
#define MAX_REGION_SIZE 2000

using namespace std;

const double generalTolerance = 0.1;
const int max_search_arealength = 25;
const int resize_factor = 2;

static vector<pair<string, double *>> pointVec;
map<string, double *> realCoordsMap;
static bool run;

cv::Mat color;
KinectLayer kinect;