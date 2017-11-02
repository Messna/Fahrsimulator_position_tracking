#pragma once

#include "opencv2/opencv.hpp"
#include "ColorPixel.h"

using namespace std;

inline bool has_target_color(ColorPixel target_color_max, ColorPixel target_color_min, cv::Vec4b& color_pxl) {
	const uint8_t blue = uint8_t(color_pxl[0]),
		green = uint8_t(color_pxl[1]),
		red = uint8_t(color_pxl[2]),
		alpha = uint8_t(color_pxl[3]);

	if (blue >= target_color_min.blue && blue <= target_color_max.blue &&
		green >= target_color_min.green && green <= target_color_max.green &&
		red >= target_color_min.red && red <= target_color_max.red) {
		return true;
	}

	return false;
}

inline void findNeighbors(int x, int y, ColorPixel target_color_max,
		ColorPixel target_color_min,
		std::map<string, bool>& hashSet,
		std::vector<std::pair<int, int>>& region) {

	const string xyKey = x + "|" + y;
	cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);
	//cout << (int)color_val[0] << " " << (int)color_val[1] << " " << (int)color_val[2] << endl;

	if (hashSet.find(xyKey) == hashSet.end() &&
		has_target_color(target_color_max, target_color_min, color_val)) {
		hashSet[xyKey] = true;
		region.push_back(  std::make_pair(x, y));

		if (x > 1) {
			findNeighbors(x - 1, y, target_color_max, target_color_min, hashSet, region);
		}
		if (y > 1) {
			findNeighbors(x, y - 1, target_color_max, target_color_min, hashSet, region);
		}
		if (x < COLOR_WIDTH - 1) {
			findNeighbors(x + 1, y, target_color_max, target_color_min, hashSet, region);
		}
		if (y < COLOR_HEIGHT - 1) {
			findNeighbors(x, y + 1, target_color_max, target_color_min, hashSet, region);
		}
	}
}

inline void region_growing(int* start, ColorPixel target_color_max, ColorPixel target_color_min) {
	map<string, bool> hashSet;
	vector<pair<int, int>> region;

	try
	{
		findNeighbors(start[0], start[1], target_color_max, target_color_min, hashSet, region);
	}
	catch (const exception &e)
	{
		cout << "Exception at findNeighbors-call: " << e.what() << endl;
	}
	long int sum_x = 0;
	long int sum_y = 0;
	if (!region.empty()) {
		for (const auto a : region) {
			sum_x += a.first;
			sum_y += a.second;
			cv::circle(color, cv::Point(a.first, a.second), 3, cv::Scalar(0, 255, 0));
		}
		start[0] = sum_x / hashSet.size() + 0.5;
		start[1] = sum_y / hashSet.size() + 0.5;
	}
}

inline int* findBestPixelForColorRange(ColorPixel target_color_max, ColorPixel target_color_min, const ColorPixel target_colorpixel) {
	int* best_pos = nullptr;
	long int min_error = generalTolerance * 3 * 255 + 0.5;

	for (int x = target_colorpixel.x - 24; x < target_colorpixel.x + 24; x++) {
		if (x < 0 || x >= COLOR_WIDTH) continue;

		for (int y = target_colorpixel.y - 24; y < target_colorpixel.y + 24; y++) {
			if (y < 0 || y >= COLOR_HEIGHT) continue;

			cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);
			const uint8_t blue = uint8_t(color_val[0]),
				green = uint8_t(color_val[1]),
				red = uint8_t(color_val[2]),
				alpha = uint8_t(color_val[3]);

			if (blue >= target_color_min.blue && blue <= target_color_max.blue &&
				green >= target_color_min.green && green <= target_color_max.green &&
				red >= target_color_min.red && red <= target_color_max.red) {

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


inline void findColorAndMark(ColorPixel& target_pixel, std::string s = "unknown", const double toleranceFactor = generalTolerance) {
	const int range = toleranceFactor * 255 + 0.5;
	const ColorPixel rgb_min = ColorPixel{ max(0, target_pixel.red - range) , max(0, target_pixel.green - range), max(0, target_pixel.blue - range) };
	const ColorPixel rgb_max = ColorPixel{ min(255, target_pixel.red + range), min(255, target_pixel.green + range), min(255, target_pixel.blue + range) };
	cv::Point textPos(0, 0);

	int* best_pos = findBestPixelForColorRange(rgb_max, rgb_min, target_pixel);
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
	double* angle = GetAngleFromColorIndex(best_pos[0], best_pos[1]);
	double* realcoord = Get3DCoordinates(angle);

	if (textPos.x < target->x &&textPos.y < target->y) {
		textPos.x = target->x + 2;
		textPos.y = target->y + 2;
	}

	target_pixel.x = target->x;
	target_pixel.y = target->y;

	if (abs(target_pixel.x - target->x) < 25 && abs(target_pixel.y - target->y) < 25) {
		target->x = target->x > 25 ? (target->x < COLOR_WIDTH - 25 ? target->x - 25 : COLOR_WIDTH - 50) : 1;
		target->y = target->y > 25 ? (target->y < COLOR_HEIGHT - 25 ? target->y - 25 : COLOR_HEIGHT - 50) : 1;
	} 
	rectangle(color, 
		cv::Point(target->x, target->y),
		cv::Point(target->x + 50, target->y + 50),
		cv::Scalar(255, 255, 255));

	// Put real coords in map for Unity
	string ps = s;
	ps[0] = 'P';
	map<string, double *> testCoordsMap = map<string, double *>();
	realCoordsMap[ps] = new double[3];
	realCoordsMap.at(ps)[0] = realcoord[0];
	realCoordsMap.at(ps)[1] = realcoord[1];
	realCoordsMap.at(ps)[2] = realcoord[2];

	// Draw real coords:
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	std::ostringstream os;
	os << realcoord[2];
	std::string str = os.str();
	std::string outputStr = s.append(" ").append(str);
	cv::putText(color, outputStr.c_str(), textPos, 1, 1, cv::Scalar(0.0, 0.0, 0.0));
	

	delete target;
	delete[] best_pos;
}