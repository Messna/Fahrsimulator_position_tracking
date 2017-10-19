#pragma once

#include "opencv2/opencv.hpp"

using namespace std;

inline bool has_target_color(double* target_color_max, double* target_color_min, cv::Vec4b& color_pxl) {

	const uint8_t blue = uint8_t(color_pxl[0]),
		green = uint8_t(color_pxl[1]),
		red = uint8_t(color_pxl[2]),
		alpha = uint8_t(color_pxl[3]);

	if (blue >= target_color_min[0] && blue <= target_color_max[0] &&
		green >= target_color_min[1] && green <= target_color_max[1] &&
		red >= target_color_min[2] && red <= target_color_max[2]) {
		return true;
	}

	return false;
}

inline void findNeighbors(int x, int y, double* target_color_max,
		double* target_color_min,
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

inline void region_growing(int* start, double* target_color_max, double* target_color_min) {
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
		cout << region.size() << endl;
		for (const auto a : region) {
			sum_x += a.first;
			sum_y += a.second;
			cv::circle(color, cv::Point(a.first, a.second), 3, cv::Scalar(0, 255, 0));
		}
		start[0] = sum_x / hashSet.size();
		start[1] = sum_y / hashSet.size();
	}
}

//inline vector<int*> get_seed_coordinates2(double* target_color_max, double* target_color_min, int* target_color) {
//	std::vector<int*> cont;
//	int* best_pos = new int[2]{ 0, 0 };
//	long int min_error = 255 * 255 * 255;
//	int i = 0;
//
//	IplImage tmpColor = color;
//	for (int x = 0; x < tmpColor.width; x++) {
//		for (int y = 0; y < tmpColor.height; y++) {
//			CvScalar color_pxl = cvGet2D(&tmpColor, y, x);
//			uint8_t green = uint8_t(color_pxl.val[0]),
//				blue = uint8_t(color_pxl.val[1]),
//				red = uint8_t(color_pxl.val[2]),
//				c4 = uint8_t(color_pxl.val[3]);
//			//target_color = RBG
//
//			if (red >= target_color_min[0] && red <= target_color_max[0] &&
//				green >= target_color_min[1] && green <= target_color_max[1] &&
//				blue >= target_color_min[2] && blue <= target_color_max[2]) {
//
//				int x2 = x;
//				int *a = new int[2]{ x2, y };
//				cont.push_back(a);
//				if (abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]) < min_error) {
//					min_error = abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]);
//					best_pos[0] = x2;
//					best_pos[1] = y;
//				}
//
//				i++;
//				//red_sum += red;
//				//green_sum += green;
//				//blue_sum += blue;
//				//std::cout << "( R:" << red << ", G: " << green << ", B: " << blue << " ) found";
//			}
//
//
//			/*if(x == x_mid && y == y_mid){
//			std::cout << "Middle-Color: ( R:" <<red << ", G: " << green << ", B: " << blue << std::endl;
//			}*/
//
//		}
//	}
//
//	//region_growing(best_pos, target_color_max, target_color_min, color);
//
//	//std::cout << "Points found: " << i << endl;
//
//	return cont;
//
//}

inline int* get_seed_coordinates3(double* target_color_max, double* target_color_min, int* target_color) {
	int* best_pos = new int[2]{ target_color[3], target_color[4] };
	long int min_error = 255 * 255 * 255;

	for (int x = target_color[3]; x < target_color[3]+50; x++) {
		if (x < 0 || x > COLOR_WIDTH) continue;

		for (int y = target_color[4]; y < target_color[4]+50; y++) {
			if (y < 0 || y > COLOR_HEIGHT) continue;

			cv::Vec4b& color_val = color.at<cv::Vec4b>(y, x);
			const uint8_t blue = uint8_t(color_val[0]),
				green = uint8_t(color_val[1]),
				red = uint8_t(color_val[2]),
				alpha = uint8_t(color_val[3]);
			//target_color = RBG

			if (blue >= target_color_min[0] && blue <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				red >= target_color_min[2] && red <= target_color_max[2]) {

				if (abs(blue - target_color[0]) * abs(green - target_color[1]) * abs(red - target_color[2]) < min_error) {
					min_error = abs(blue - target_color[0]) * abs(green - target_color[1]) * abs(red - target_color[2]);
					best_pos[0] = x;
					best_pos[1] = y;
				}
			}
		}
	}

	region_growing(best_pos, target_color_max, target_color_min);

	return best_pos;
}


inline void findColorAndMark(int* rgb_target, std::string s = "unknown", const double toleranceFactor = generalTolerance) {
	const double range = toleranceFactor * 255;
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

	int* best_pos = get_seed_coordinates3(rgb_max, rgb_min, rgb_target);
	cv::Mat output_frame(color);

	cv::Point *target = new cv::Point(int(0.5 + best_pos[0]), best_pos[1]);

	cv::circle(color, *target, 1, cv::Scalar(0, 0, 0));
	double* angle = GetAngleFromColorIndex(best_pos[0], best_pos[1]);
	double* realcoord = Get3DCoordinates(angle);
	//cvCircle(color, *target, 2, cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));
	if (textPos.x < target->x &&textPos.y < target->y) {
		textPos.x = target->x + 2;
		textPos.y = target->y + 2;
	}

	/* if (abs(rgb_target[3] - target->x) < 40 && abs(rgb_target[4] - target->y) < 40) { // TODO Nicht bewegen wenn kack Farbe
		rgb_target[3] = target->x > 25 ? (target->x < COLOR_WIDTH - 25 ? target->x - 25 : COLOR_WIDTH - 50) : 1;
		rgb_target[4] = target->y > 25 ? (target->y < COLOR_HEIGHT - 25 ? target->y - 25 : COLOR_HEIGHT - 50) : 1;
	} */

	rectangle(color, 
		cv::Point(rgb_target[3], rgb_target[4]), 
		cv::Point(rgb_target[3] + 50, rgb_target[4] + 50), 
		cv::Scalar(rgb_target[0], rgb_target[1], rgb_target[2]));

	// Draw real coords:
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	std::ostringstream os;
	os << realcoord[2];
	std::string str = os.str();
	std::string outputStr = s.append(" ").append(str);
	//std::cout << "X: " << realcoord[0] << " Y: " << realcoord[1] << " Z: " << realcoord[2] << std::endl;
	cv::putText(color, outputStr.c_str(), textPos, 1, 1, cv::Scalar(0.0, 0.0, 0.0));
	//std::cout << result[0][1] << " " << result[0][0] << std::endl;
	//std::cout << result.size() << std::endl;

	//cv::circle(color, cv::Point(320, 240), 3, cv::Scalar(0, 255, 0));

	delete[] rgb_max;
	delete[] rgb_min;
	delete target;
	delete[] best_pos;
}