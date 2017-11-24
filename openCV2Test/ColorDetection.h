#pragma once

#include "opencv2/opencv.hpp"
#include "ColorPixel.h"
#include "KinectLayer.h"

using namespace std;

inline bool has_target_color(ColorPixel* target_color_max, ColorPixel* target_color_min, cv::Vec4b& color_pxl) {
	const uint8_t blue = uint8_t(color_pxl[0]),
		green = uint8_t(color_pxl[1]),
		red = uint8_t(color_pxl[2]),
		alpha = uint8_t(color_pxl[3]);

	if (blue >= target_color_min->blue && blue <= target_color_max->blue &&
		green >= target_color_min->green && green <= target_color_max->green &&
		red >= target_color_min->red && red <= target_color_max->red) {
		return true;
	}

	return false;
}

inline void findNeighbors(
		int x, 
		int y, 
		ColorPixel* target_color_max,
		ColorPixel* target_color_min,
		std::map<string, bool>& already_checked_pixel,
		std::vector<std::pair<int, int>>& color_region) {

	string xyKey = to_string(x) + "|" + to_string(y);
	//const string xyKey = x + "|" + y;
	cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);

	if (already_checked_pixel.find(xyKey) == already_checked_pixel.end() &&
			has_target_color(target_color_max, target_color_min, color_val)) {

		already_checked_pixel[xyKey] = true;
		color_region.push_back(std::make_pair(x, y));

		if (x > 1) {
			findNeighbors(x - 1, y, target_color_max, target_color_min, already_checked_pixel, color_region);
		}
		if (y > 1) {
			findNeighbors(x, y - 1, target_color_max, target_color_min, already_checked_pixel, color_region);
		}
		if (x < COLOR_WIDTH - 1) {
			findNeighbors(x + 1, y, target_color_max, target_color_min, already_checked_pixel, color_region);
		}
		if (y < COLOR_HEIGHT - 1) {
			findNeighbors(x, y + 1, target_color_max, target_color_min, already_checked_pixel, color_region);
		}
	}
}

inline void region_growing(int* start, ColorPixel* target_color_max, ColorPixel* target_color_min) {
	int pixel_count = 0;
	long int sum_x = 0;
	long int sum_y = 0;

	//findNeighbors(start[0], start[1], target_color_max, target_color_min, already_checked_pixel, color_region);
	for (int i = start[0] - max_search_arealength; i < start[0] + max_search_arealength - 1; i = i + 2)
	{
		if (i < 0 || i >= COLOR_WIDTH) continue;
		for (int j = start[1] - max_search_arealength; j < start[1] + max_search_arealength - 1; j = j + 2)
		{
			if (j < 0 || j >= COLOR_HEIGHT) continue;
			string xyKey = to_string(i) + "|" + to_string(j);
			cv::Vec4b& color_val = color.at<cv::Vec4b>(j, i);
			if (has_target_color(target_color_max, target_color_min, color_val)) {
				pixel_count++;
				sum_x += i;
				sum_y += j;
			}
		}
	}

	if (sum_x > 0 && sum_y > 0) {
		start[0] = sum_x / pixel_count + 0.5;
		start[1] = sum_y / pixel_count + 0.5;
	}
}

inline int* findBestPixelForColorRange(ColorPixel* target_color_max, ColorPixel* target_color_min, const ColorPixel& target_colorpixel) {
	int* best_pos = nullptr;
	long int min_error = generalTolerance * 3 * 255 + 0.5;

	for (int x = target_colorpixel.x - max_search_arealength - 1; x < target_colorpixel.x + max_search_arealength + 1; x++) {
		if (x < 0 || x >= COLOR_WIDTH) continue;

		for (int y = target_colorpixel.y - max_search_arealength - 1; y < target_colorpixel.y + max_search_arealength + 1; y++) {
			if (y < 0 || y >= COLOR_HEIGHT) continue;

			cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);
			const uint8_t blue = uint8_t(color_val[0]),
				green = uint8_t(color_val[1]),
				red = uint8_t(color_val[2]),
				alpha = uint8_t(color_val[3]);

			if (blue >= target_color_min->blue && blue <= target_color_max->blue &&
				green >= target_color_min->green && green <= target_color_max->green &&
				red >= target_color_min->red && red <= target_color_max->red) {

				if (abs(blue - target_colorpixel.blue) + abs(green - target_colorpixel.green) + abs(red - target_colorpixel.red) < min_error) {
					min_error = abs(blue - target_colorpixel.blue) + abs(green - target_colorpixel.green) + abs(red - target_colorpixel.red);
					if (best_pos == nullptr) best_pos = new int[2];
					best_pos[0] = x;
					best_pos[1] = y;
				}
			}
		}
	}
	if(best_pos != nullptr)
	region_growing(best_pos, target_color_max, target_color_min);

	return best_pos;
}

inline void find_color_and_mark(ColorPixel& target_pixel, string s = "unknown", const double tolerance_factor = generalTolerance) {
	int range = tolerance_factor * 255 + 0.5;
	ColorPixel rgb_min = { max(0, target_pixel.red - range) , max(0, target_pixel.green - range), max(0, target_pixel.blue - range) };
	ColorPixel rgb_max = { min(255, target_pixel.red + range), min(255, target_pixel.green + range), min(255, target_pixel.blue + range) };
	cv::Point text_pos(0, 0);

	int* best_pos = findBestPixelForColorRange(&rgb_max, &rgb_min, target_pixel);
	cv::Point *target;
	if (best_pos != nullptr)
	{ 
		target = new cv::Point(int(0.5 + best_pos[0]), best_pos[1]);
	}else
	{
		target = new cv::Point(target_pixel.x, target_pixel.y);
		best_pos = new int[2];
		best_pos[0] = target_pixel.x;
		best_pos[1] = target_pixel.y;
	}
	cv::circle(color, *target, 1, cv::Scalar(0, 0, 0));

	float fx = best_pos[0];
	float fy = best_pos[1];
	double* real_world_pos = new double[5]{ -1000, -1000, -1000, 1, 1 };
	auto camera_space_point = new CameraSpacePoint();
	ERROR_CHECK(kinect.coordinateMapper->MapDepthPointToCameraSpace(DepthSpacePoint{ fx, fy }, kinect.getDepthForPixel(fx, fy), camera_space_point));
	real_world_pos[0] = camera_space_point->X;
	real_world_pos[1] = camera_space_point->Y;
	real_world_pos[2] = camera_space_point->Z;

	if (text_pos.x < target->x &&text_pos.y < target->y) {
		text_pos.x = target->x + 2;
		text_pos.y = target->y + 2;
	}

	target_pixel.x = target->x;
	target_pixel.y = target->y;

	if (abs(target_pixel.x - target->x) < max_search_arealength && abs(target_pixel.y - target->y) < max_search_arealength) {
		target->x = target->x > max_search_arealength ? (target->x < COLOR_WIDTH - max_search_arealength ? target->x - max_search_arealength : COLOR_WIDTH - 2 * max_search_arealength) : 1;
		target->y = target->y > max_search_arealength ? (target->y < COLOR_HEIGHT - max_search_arealength ? target->y - max_search_arealength : COLOR_HEIGHT - 2 * max_search_arealength) : 1;
	} 
	rectangle(color, 
		cv::Point(target->x, target->y),
		cv::Point(target->x + 2 * max_search_arealength, target->y + 2 * max_search_arealength),
		cv::Scalar(255, 255, 255));

	// Put real coords in map for Unity
	string ps = s;
	ps[0] = 'P';
	map<string, double *> test_coords_map = map<string, double *>();
	realCoordsMap[ps] = new double[3];
	realCoordsMap.at(ps)[0] = real_world_pos[0];
	realCoordsMap.at(ps)[1] = real_world_pos[1];
	realCoordsMap.at(ps)[2] = real_world_pos[2];

	// Draw real coords:
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	ostringstream os;
	os << real_world_pos[2];
	std::string str = os.str();
	std::string outputStr = s.append(" ").append(str);
	cv::putText(color, outputStr.c_str(), text_pos, 1, 1.5 / resize_factor, cv::Scalar(0.0, 0.0, 0.0));

	delete target;
	delete[] best_pos;
}