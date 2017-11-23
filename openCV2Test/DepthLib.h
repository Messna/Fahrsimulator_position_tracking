#pragma once

#include "KinectLayer.h"

inline double GetRadianFromDegree(const double angleInDegree) {
	return angleInDegree * PI / 180.0;
}

inline double GetDegreeFromRadian(const double angleInRadian) {
	return angleInRadian * 180.0 / PI;
}

//return 2 angles, for horizontal (x) and vertical (y)
//double* GetAngleFromColorIndex(const int colorX, const int colorY) {
//	const double coordMidX = COLOR_WIDTH / 2.0 - 0.5;
//	const double coordMidY = COLOR_HEIGHT / 2.0 - 0.5;
//	double* returnArr = new double[2]{ -100, -100 };
//
//	const double width = tan(GetRadianFromDegree(fovColorX / 2.0)) * 2;
//	const double height = tan(GetRadianFromDegree(fovColorY / 2.0)) * 2;
//	const double widthStep = width / (COLOR_WIDTH - 1);
//	const double heightStep = height / (COLOR_HEIGHT - 1);
//
//	const double centeredX = colorX - coordMidX;
//	const double centeredY = colorY - coordMidY;
//
//	const double trueAngleX = GetDegreeFromRadian(atan(centeredX * widthStep));
//	const double trueAngleY = GetDegreeFromRadian(atan(centeredY * heightStep));
//
//	//cout << " for coords (" << colorX << ";" << colorY << ") the degrees are (" << trueAngleX << ";" << trueAngleY << ")" << endl;
//
//	returnArr[0] = trueAngleX;
//	returnArr[1] = trueAngleY;
//	return returnArr;
//}
//
//inline double* Get3DCoordinates(double* angles) {
//	double* realWorldCoords = new double[5]{ -1000, -1000, -1000, 1, 1 };
//
//	const double colorAngleX = angles[0];
//	const double colorAngleY = angles[1];
//
//	const double width = tan(GetRadianFromDegree(fovDepthX / 2.0)) * 2;
//	const double height = tan(GetRadianFromDegree(fovDepthY / 2.0)) * 2;
//	const double widthStep = width / (DEPTH_WIDTH - 1);
//	const double heightStep = height / (DEPTH_HEIGHT - 1);
//
//	const double coordMidX = DEPTH_WIDTH / 2.0 - 0.5;
//	const double coordMidY = DEPTH_HEIGHT / 2.0 - 0.5;
//
//	const double distX = tan(GetRadianFromDegree(colorAngleX));
//	const double distY = tan(GetRadianFromDegree(colorAngleY));
//
//
//	//calc index position in depth array
//	int idxDepthX = (int)(distX / widthStep + coordMidX + 0.5);
//	int idxDepthY = (int)(distY / heightStep + coordMidY + 0.5);
//
//	//check range of index (is in FoV)
//	if (idxDepthX >= 0 && idxDepthX < DEPTH_WIDTH && idxDepthY >= 0 && idxDepthY < DEPTH_HEIGHT) {
//		double depthValZ = kinect.getDepthForPixel(idxDepthX, idxDepthY);
//
//		double realWorldZ = depthValZ / 10.0; //convert from mm to cm
//		double realWorldX = tan(GetRadianFromDegree(colorAngleX)) * realWorldZ;
//		double realWorldY = tan(GetRadianFromDegree(colorAngleY)) * realWorldZ;
//		realWorldCoords[0] = realWorldX;
//		realWorldCoords[1] = realWorldY;
//		realWorldCoords[2] = realWorldZ;
//
//		//std::cout << "3D pos in cm: (" << realWorldX << ";" << realWorldY << ";" << realWorldZ << ")" << std::endl;
//	}
//	return realWorldCoords;
//}

inline double GetLength(double* p1, double* p2) {
	double l1 = (p2[0] - p1[0]) * (p2[0] - p1[0]);
	double l2 = (p2[1] - p1[1]) * (p2[1] - p1[1]);
	double l3 = (p2[2] - p1[2]) * (p2[2] - p1[2]);
	return sqrt(l1 + l2 + l3);
}