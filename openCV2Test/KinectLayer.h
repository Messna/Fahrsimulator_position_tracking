/*
* Copyright (c) 2016-2017 Yoshihisa Nitta
* Released under the MIT license
* http://opensource.org/licenses/mit-license.php
*/

/* version 1.8.2: 2017/08/16 */

/* http://nw.tsuda.ac.jp/lec/kinect2/ */

#pragma once

#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <atlbase.h>

#include <Kinect.h>

#include <thread>
#include <ppl.h>
#include <wrl/client.h>

using namespace std;

#define ERROR_CHECK(ret)                                \
  if ((ret) != S_OK) {                                  \
    stringstream ss;                                    \
    ss << "failed " #ret " " << hex << ret << endl;     \
    throw runtime_error(ss.str().c_str());              \
  }

class KinectLayer {
	// ******* kinect ********
	CComPtr<IKinectSensor> kinect = nullptr;
public:
	CComPtr<ICoordinateMapper> coordinateMapper = nullptr;

private:
	void initialize() {
		ERROR_CHECK(GetDefaultKinectSensor(&kinect));
		ERROR_CHECK(kinect->Open());
		BOOLEAN isOpen;
		ERROR_CHECK(kinect->get_IsOpen(&isOpen));
		if (!isOpen)throw runtime_error("failed IKinectSensor::get_IsOpen( &isOpen )");

		kinect->get_CoordinateMapper(&coordinateMapper);
	}

	// ******** color ********
private:
	CComPtr<IColorFrameReader> colorFrameReader = nullptr;
	BOOLEAN color_initialized = false;
	vector<BYTE> colorBuffer;
public:
	int colorWidth;
	int colorHeight;
private:
	unsigned int colorBytesPerPixel;

	void initializeColorFrame() {
		// Open Color Reader
		Microsoft::WRL::ComPtr<IColorFrameSource> colorFrameSource;
		ERROR_CHECK(kinect->get_ColorFrameSource(&colorFrameSource));
		ERROR_CHECK(colorFrameSource->OpenReader(&colorFrameReader));

		// Retrieve Color Description
		Microsoft::WRL::ComPtr<IFrameDescription> colorFrameDescription;
		ERROR_CHECK(colorFrameSource->CreateFrameDescription(ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription)
		);
		ERROR_CHECK(colorFrameDescription->get_Width(&colorWidth)); // 1920
		ERROR_CHECK(colorFrameDescription->get_Height(&colorHeight)); // 1080
		ERROR_CHECK(colorFrameDescription->get_BytesPerPixel(&colorBytesPerPixel)); // 4

		// Allocation Color Buffer
		colorBuffer.resize(colorWidth * colorHeight * colorBytesPerPixel);

		color_initialized = true;
	}

	void updateColorFrame() {
		// Retrieve Color Frame
		Microsoft::WRL::ComPtr<IColorFrame> colorFrame;
		const HRESULT ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
		if (FAILED(ret)) {
			return;
		}

		// Convert Format ( YUY2 -> BGRA )
		ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(static_cast<UINT>(colorBuffer.size()), &colorBuffer[0],
			ColorImageFormat::ColorImageFormat_Bgra));
	}

public:
	cv::Mat rgbImage;
	void setRGB() { setRGB(rgbImage); }

	void setRGB(cv::Mat& image) {
		if (!color_initialized) initializeColorFrame();

		updateColorFrame();

		vector<ColorSpacePoint> colorSpacePoints(depthWidth * depthHeight);
		ERROR_CHECK(coordinateMapper->MapDepthFrameToColorSpace(depthBuffer.size(), &depthBuffer[0], colorSpacePoints.size(),
			&colorSpacePoints[0]));
		// Mapping Color to Depth Resolution
		vector<BYTE> buffer(depthWidth * depthHeight * colorBytesPerPixel);

		Concurrency::parallel_for(0, depthHeight, [&](const int depthY) {
			const unsigned int depthOffset = depthY * depthWidth;
			for (int depthX = 0; depthX < depthWidth; depthX++) {
				unsigned int depthIndex = depthOffset + depthX;
				const int colorX = static_cast<int>(colorSpacePoints[depthIndex].X + 0.5f);
				const int colorY = static_cast<int>(colorSpacePoints[depthIndex].Y + 0.5f);
				if ((0 <= colorX) && (colorX < colorWidth) && (0 <= colorY) && (colorY < colorHeight)) {
					const unsigned int colorIndex = (colorY * colorWidth + colorX) * colorBytesPerPixel;
					depthIndex = depthIndex * colorBytesPerPixel;
					buffer[depthIndex + 0] = colorBuffer[colorIndex + 0];
					buffer[depthIndex + 1] = colorBuffer[colorIndex + 1];
					buffer[depthIndex + 2] = colorBuffer[colorIndex + 2];
					buffer[depthIndex + 3] = colorBuffer[colorIndex + 3];
				}
			}
		});

		image = cv::Mat(depthHeight, depthWidth, CV_8UC4, &buffer[0]).clone();
	}

	// ******** depth *******
private:
	CComPtr<IDepthFrameReader> depthFrameReader = nullptr;
	BOOLEAN depth_initialized = false;
	vector<UINT16> depthBuffer;
	int depthWidth;
	int depthHeight;
	UINT16 maxDepth;
	UINT16 minDepth;

	void initializeDepthFrame() {
		// Open Depth Reader
		Microsoft::WRL::ComPtr<IDepthFrameSource> depthFrameSource;
		ERROR_CHECK(kinect->get_DepthFrameSource(&depthFrameSource));
		ERROR_CHECK(depthFrameSource->OpenReader(&depthFrameReader));

		// Retrieve Depth Description
		Microsoft::WRL::ComPtr<IFrameDescription> depthFrameDescription;
		ERROR_CHECK(depthFrameSource->get_FrameDescription(&depthFrameDescription));
		ERROR_CHECK(depthFrameDescription->get_Width(&depthWidth)); // 512
		ERROR_CHECK(depthFrameDescription->get_Height(&depthHeight)); // 424

		// Allocation Depth Buffer
		depthBuffer.resize(depthWidth * depthHeight);
		depth_initialized = true;
	}

	void updateDepthFrame() {
		if (!depth_initialized) initializeDepthFrame();
		// Retrieve Depth Frame
		Microsoft::WRL::ComPtr<IDepthFrame> depthFrame;
		const HRESULT ret = depthFrameReader->AcquireLatestFrame(&depthFrame);
		if (FAILED(ret)) {
			return;
		}

		// Retrieve Depth Data
		ERROR_CHECK(depthFrame->CopyFrameDataToArray(static_cast<UINT>(depthBuffer.size()), &depthBuffer[0]));
	}

public:
	cv::Mat depthImage;
	void setDepth(const bool raw = true) { setDepth(depthImage, raw); }

	void setDepth(cv::Mat& depthImage, bool raw = true) {
		updateDepthFrame();
		depthImage = cv::Mat(depthHeight, depthWidth, CV_16UC1, &depthBuffer[0]);
	}

	int getDepthForPixel(const int x, const int y) {
		return depthBuffer[y * depthWidth + x];
	}

	KinectLayer() { initialize(); }

	~KinectLayer() {
		if (kinect != nullptr) kinect->Close();
	}
};
