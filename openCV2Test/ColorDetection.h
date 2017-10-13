#pragma once

#include "opencv2\opencv.hpp"

using namespace std;

bool has_target_color(double* target_color_max, double* target_color_min, CvScalar& color_pxl) {

	uint8_t green = 0, blue = 0, red = 0, c4 = 0;
	green = uint8_t(color_pxl.val[0]),
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
	std::map<string, bool>& hashSet,
	std::vector<std::pair<int, int>>& region) {
	string xyKey = x + "|" + y;
	IplImage tmpColor = color;
	if ((hashSet.find(xyKey) == hashSet.end()) &&
		has_target_color(target_color_max, target_color_min, cvGet2D(&tmpColor, y, x))) {
		hashSet[xyKey] = true;
		region.push_back(  std::make_pair(x, y));

		if (x > 1) {
			findNeighbors(x - 1, y, target_color_max, target_color_min, hashSet, region);
		}
		if (y > 1) {
			findNeighbors(x, y - 1, target_color_max, target_color_min, hashSet, region);
		}
		if (x < tmpColor.width - 1) {
			findNeighbors(x + 1, y, target_color_max, target_color_min, hashSet, region);
		}
		if (y < tmpColor.height - 1) {
			findNeighbors(x, y + 1, target_color_max, target_color_min, hashSet, region);
		}
	}
}

void region_growing(int* start, double* target_color_max, double* target_color_min) {
	std::map<string, bool> hashSet;
	std::vector<std::pair<int, int>> region;

	try
	{
		findNeighbors(start[0], start[1], target_color_max, target_color_min, hashSet, region);

	}
	catch (const std::exception &e)
	{
		std::cout << "Exception at findNeighbors-call" << std::endl;
	}
	long int sum_x = 0;
	long int sum_y = 0;
	if (!region.empty()) {
		for (auto a : region) {
			sum_x += a.first;
			sum_y += a.second;
		}
		start[0] = sum_x / hashSet.size();
		start[1] = sum_y / hashSet.size();
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

	IplImage tmpColor = color;
	for (int x = 0; x < tmpColor.width; x++) {
		for (int y = 0; y < tmpColor.height; y++) {
			CvScalar color_pxl = cvGet2D(&tmpColor, y, x);
			uint8_t green = uint8_t(color_pxl.val[0]),
				blue = uint8_t(color_pxl.val[1]),
				red = uint8_t(color_pxl.val[2]),
				c4 = uint8_t(color_pxl.val[3]);
			//target_color = RBG

			if (red >= target_color_min[0] && red <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				blue >= target_color_min[2] && blue <= target_color_max[2]) {

				int x2 = x;
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


	int* best_pos = new int[2]{ 1, 1 };
	long int min_error = 255 * 255 * 255;
	int i = 0;
	double red_sum = 0.0;
	double blue_sum = 0.0;
	double green_sum = 0.0;
	IplImage tmpColor = color;

	for (int x = target_color[3]; x < target_color[3]+50; x++) {
		for (int y = target_color[4]; y < target_color[4]+50; y++) {
			CvScalar color_pxl = cvGet2D(&tmpColor, y, x);
			uint8_t green = uint8_t(color_pxl.val[0]),
				blue = uint8_t(color_pxl.val[1]),
				red = uint8_t(color_pxl.val[2]),
				c4 = uint8_t(color_pxl.val[3]);
			//target_color = RBG

			if (red >= target_color_min[0] && red <= target_color_max[0] &&
				green >= target_color_min[1] && green <= target_color_max[1] &&
				blue >= target_color_min[2] && blue <= target_color_max[2]) {

				if (abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]) < min_error) {
					min_error = abs(red - target_color[0]) * abs(blue - target_color[1]) * abs(green - target_color[2]);
					best_pos[0] = x;
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
	cv::Mat output_frame(color);

	cv::Point *target = new cv::Point(int(0.5 + a[0]), a[1]);

	cv::circle(color, *target, 1, cv::Scalar(0, 0, 0));
	double* angle = GetAngleFromColorIndex(a[0], a[1]);
	double* realcoord = Get3DCoordinates(angle);
	//cvCircle(color, *target, 2, cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));
	if (textPos.x < target->x &&textPos.y < target->y) {
		textPos.x = target->x + 2;
		textPos.y = target->y + 2;
	}

	cv::rectangle(color, cv::Point(rgb_target[3], rgb_target[4]), cv::Point(rgb_target[3] + 50, rgb_target[4] + 50), cv::Scalar(rgb_target[1], rgb_target[2], rgb_target[0]));

	if (abs(rgb_target[3] - target->x) < 40 && abs(rgb_target[4] - target->y) < 40) {
		rgb_target[3] = target->x > 25 ? (target->x < COLOR_WIDTH - 25 ? target->x - 25 : COLOR_WIDTH - 50) : 1;
		rgb_target[4] = target->y > 25 ? (target->y < COLOR_HEIGHT - 25 ? target->y - 25 : COLOR_HEIGHT - 50) : 1;
	}
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

	cv::circle(color, cv::Point(320, 240), 3, cv::Scalar(0, 255, 0));
	delete rgb_max;
	delete rgb_min;
	delete target;
	delete[] a;
}