#pragma once

#include "opencv2/opencv.hpp"
#include "ColorPixel.h"
#include "KinectLayer.h"
#include <stack>

using namespace std;

long int sum_x;
long int sum_y;
long int pixel_count;

list<cv::Vec4b> previous_colors;


inline bool has_target_color(ColorPixel* target_color_max, ColorPixel* target_color_min, cv::Vec4b& color_pxl) {
	const auto blue = uint8_t(color_pxl[0]),
		green = uint8_t(color_pxl[1]),
		red = uint8_t(color_pxl[2]);

	if (blue >= target_color_min->blue && blue <= target_color_max->blue &&
		green >= target_color_min->green && green <= target_color_max->green &&
		red >= target_color_min->red && red <= target_color_max->red) {
		return true;
	}

	return false;
}

inline void find_neighbors_iter(int x, int y, ColorPixel* target_color_max, ColorPixel* target_color_min,
                                bool** visited_array) {
	stack<int> xStack;
	stack<int> yStack;
	xStack.push(x);
	yStack.push(y);


	while (!xStack.empty()) {
		int currX = xStack.top();
		int currY = yStack.top();
		xStack.pop();
		yStack.pop();

		pixel_count++;
		sum_x += currX;
		sum_y += currY;
		visited_array[currX][currY] = true;

		//safety check if large background regions are accessed
		if (pixel_count > MAX_REGION_SIZE) {
			return;
		}

		for (int xOffset = -1; xOffset <= 1; xOffset++) {
			for (int yOffset = -1; yOffset <= 1; yOffset++) {
				int nbX = currX + xOffset;
				int nbY = currY + yOffset;
				//check range
				if (nbX >= 0 && nbX < COLOR_WIDTH && nbY >= 0 && nbY < COLOR_HEIGHT) {
					//check not visited 
					if (visited_array[nbX][nbY] != true) {
						//check range
						auto& color_val = color.at<cv::Vec4b>(nbY, nbX);
						if (has_target_color(target_color_max, target_color_min, color_val)) {
							xStack.push(nbX);
							yStack.push(nbY);
						}
					}
				}
			}
		}
	}
}

inline void region_growing2(int* start, ColorPixel* target_color_max, ColorPixel* target_color_min,
                            bool** visited_array) {
	pixel_count = 0;
	sum_x = 0;
	sum_y = 0;

	find_neighbors_iter(start[0], start[1], target_color_max, target_color_min, visited_array);

	//calculate centroide
	if (sum_x > 0 && sum_y > 0) {
		start[0] = static_cast<int>(sum_x / static_cast<double>(pixel_count) + 0.5);
		start[1] = static_cast<int>(sum_y / static_cast<double>(pixel_count) + 0.5);
	}
}

inline int* find_best_pixel_for_color_range(ColorPixel* target_color_max, ColorPixel* target_color_min,
                                            const ColorPixel& target_colorpixel) {
	int* best_pos = nullptr;
	long int min_error = generalTolerance * 3 * 255 + 0.5;

	for (int x = target_colorpixel.x - max_search_arealength - 1; x < target_colorpixel.x + max_search_arealength + 1; x++) {
		if (x < 0 || x >= COLOR_WIDTH) continue;

		for (int y = target_colorpixel.y - max_search_arealength - 1; y < target_colorpixel.y + max_search_arealength + 1; y++) {
			if (y < 0 || y >= COLOR_HEIGHT) continue;

			auto color_val = color.at<cv::Vec4b>(y, x);
			const auto blue = uint8_t(color_val[0]),
				green = uint8_t(color_val[1]),
				red = uint8_t(color_val[2]);

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

	return best_pos;
}

inline ColorPixel find_color_and_mark(ColorPixel& target_pixel, bool** visited_array, string name,
                                       const double tolerance_factor = generalTolerance) {
	const int range = tolerance_factor * 255 + 0.5;
	ColorPixel rgb_min = {
		max(0, target_pixel.red - range) , max(0, target_pixel.green - range), max(0, target_pixel.blue - range)
	};
	ColorPixel rgb_max = {
		min(255, target_pixel.red + range), min(255, target_pixel.green + range), min(255, target_pixel.blue + range)
	};
	cv::Point text_pos(0, 0);

	auto return_val = ColorPixel();
	return_val.red = target_pixel.red;
	return_val.green = target_pixel.green;
	return_val.blue = target_pixel.blue;
	return_val.x = target_pixel.x;
	return_val.y = target_pixel.y;

	int* best_pos = find_best_pixel_for_color_range(&rgb_max, &rgb_min, target_pixel);
	cv::Point* target;
	if (best_pos != nullptr) {
		region_growing2(best_pos, &rgb_max, &rgb_min, visited_array);
		target = new cv::Point(int(best_pos[0]), best_pos[1]);
		//overide target ColorPos
		auto& color_val = color.at<cv::Vec4b>(best_pos[1], best_pos[0]);
		if (color_val[0] != 0 && color_val[1] != 0 && color_val[2] != 0) {
			return_val.red = round(return_val.red * 0.97 + color_val[2] * 0.03);
			return_val.green = round(return_val.green * 0.97 + color_val[1] * 0.03);
			return_val.blue = round(return_val.blue * 0.97 + color_val[0] * 0.03);
		}
		return_val.x = best_pos[0];
		return_val.y = best_pos[1];
	}
	else {
		target = new cv::Point(target_pixel.x, target_pixel.y);
		best_pos = new int[2];
		best_pos[0] = target_pixel.x;
		best_pos[1] = target_pixel.y;
	}
	circle(color, *target, 1, cv::Scalar(0, 0, 0));

	float fx = best_pos[0];
	float fy = best_pos[1];
	const auto real_world_pos = new double[5]{-1000, -1000, -1000, 1, 1};
	auto camera_space_point = new CameraSpacePoint();
	ERROR_CHECK(kinect.coordinateMapper->MapDepthPointToCameraSpace(DepthSpacePoint{ fx, fy }, kinect.getDepthForPixel(fx, fy), camera_space_point));
	real_world_pos[0] = camera_space_point->X;
	real_world_pos[1] = camera_space_point->Y;
	real_world_pos[2] = camera_space_point->Z;

	if (text_pos.x < target->x && text_pos.y < target->y) {
		text_pos.x = target->x + 2;
		text_pos.y = target->y + 2;
	}

	target_pixel.x = target->x;
	target_pixel.y = target->y;

	if (abs(target_pixel.x - target->x) < max_search_arealength && 
		abs(target_pixel.y - target->y) < max_search_arealength) {
		target->x = target->x > max_search_arealength
			            ? (target->x < COLOR_WIDTH - max_search_arealength
				               ? target->x - max_search_arealength
				               : COLOR_WIDTH - 2 * max_search_arealength)
			            : 1;
		target->y = target->y > max_search_arealength
			            ? (target->y < COLOR_HEIGHT - max_search_arealength
				               ? target->y - max_search_arealength
				               : COLOR_HEIGHT - 2 * max_search_arealength)
			            : 1;
	}
	rectangle(color,
	          cv::Point(target->x, target->y),
	          cv::Point(target->x + 2 * max_search_arealength, target->y + 2 * max_search_arealength),
	          cv::Scalar(255, 255, 255));

	// Put real coords in map for Unity
	string ps = name;
	ps[0] = 'P';
	realCoordsMap[ps] = new double[3];
	realCoordsMap.at(ps)[0] = real_world_pos[0];
	realCoordsMap.at(ps)[1] = real_world_pos[1];
	realCoordsMap.at(ps)[2] = real_world_pos[2];

	// Draw real coords:
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	ostringstream os;
	os << real_world_pos[2];
	const string str = os.str();
	string output_str = name.append(" ").append(str);
	putText(color, output_str.c_str(), text_pos, 1, 1.5 / resize_factor, cv::Scalar(255.0, 255.0, 255.0));

	delete target;
	delete[] best_pos;

	return return_val;
}
