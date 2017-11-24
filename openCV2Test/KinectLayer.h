/*
* Copyright (c) 2016-2017 Yoshihisa Nitta
* Released under the MIT license
* http://opensource.org/licenses/mit-license.php
*/

/* version 1.8.2: 2017/08/16 */

/* http://nw.tsuda.ac.jp/lec/kinect2/ */

#pragma once

#include <sstream>
#include <string>
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
#define ERROR_CHECK2( ret )                                     \
  if( FAILED( ret ) ){                                          \
    std::stringstream ss;                                       \
    ss << "failed " #ret " " << std::hex << ret << std::endl;   \
    throw std::runtime_error( ss.str().c_str() );               \
  }


class KinectLayer {

	// ******* kinect ********
private:
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
		ERROR_CHECK(colorFrameSource->CreateFrameDescription(ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription));
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
		ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(static_cast<UINT>(colorBuffer.size()), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));

		
	}
public:
	cv::Mat rgbImage;
	void setRGB() { setRGB(rgbImage); }
	void setRGB(cv::Mat& image) {
		if (!color_initialized) initializeColorFrame();

		updateColorFrame();
		
		std::vector<ColorSpacePoint> colorSpacePoints(depthWidth * depthHeight);
		ERROR_CHECK(coordinateMapper->MapDepthFrameToColorSpace(depthBuffer.size(), &depthBuffer[0], colorSpacePoints.size(), &colorSpacePoints[0]));
		// Mapping Color to Depth Resolution
		std::vector<BYTE> buffer(depthWidth * depthHeight * colorBytesPerPixel);

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
	void setDepth(bool raw = true) { setDepth(depthImage, raw); }
	void setDepth(cv::Mat& depthImage, bool raw = true) {
		updateDepthFrame();
		depthImage = cv::Mat(depthHeight, depthWidth, CV_16UC1, &depthBuffer[0]);
	}
	int getDepthForPixel(int x, int y) {
		return depthBuffer[y * depthWidth + x];
	}

	// ******* infrared ********
private:
	CComPtr<IInfraredFrameReader> infraredFrameReader = nullptr;
	BOOLEAN infrared_initialized = false;
	int infraredWidth;
	int infraredHeight;
	vector<UINT16> infraredBuffer;
	void initializeInfraredFrame() {
		CComPtr<IInfraredFrameSource> infraredFrameSource;
		ERROR_CHECK(kinect->get_InfraredFrameSource(&infraredFrameSource));
		ERROR_CHECK(infraredFrameSource->OpenReader(&infraredFrameReader));
		CComPtr<IFrameDescription> infraredFrameDescription;
		ERROR_CHECK(infraredFrameSource->get_FrameDescription(&infraredFrameDescription));
		ERROR_CHECK(infraredFrameDescription->get_Width(&infraredWidth));
		ERROR_CHECK(infraredFrameDescription->get_Height(&infraredHeight));
		//cout << "infrared: " << infraredWidth << " " << infraredHeight << endl;
		infraredBuffer.resize(infraredWidth * infraredHeight);
		infrared_initialized = true;
	}
	void updateInfraredFrame() {
		if (!infrared_initialized) initializeInfraredFrame();
		CComPtr<IInfraredFrame> infraredFrame;
		auto ret = infraredFrameReader->AcquireLatestFrame(&infraredFrame);
		if (FAILED(ret)) return;
		ERROR_CHECK(infraredFrame->CopyFrameDataToArray((UINT)infraredBuffer.size(), &infraredBuffer[0]));
	}
public:
	cv::Mat infraredImage;
	void setInfrared() { setInfrared(infraredImage); }
	void setInfrared(cv::Mat& infraredImage) {
		updateInfraredFrame();
		infraredImage = cv::Mat(infraredHeight, infraredWidth, CV_16UC1, &infraredBuffer[0]);
	}

	// ******** bodyIndex *******
private:
	CComPtr<IBodyIndexFrameReader> bodyIndexFrameReader = nullptr;
	BOOLEAN bodyIndex_initialized = false;
	vector<BYTE> bodyIndexBuffer;
	int bodyIndexWidth;
	int bodyIndexHeight;
	cv::Vec3b   colors[8];
	void initializeBodyIndexFrame() {
		CComPtr<IBodyIndexFrameSource> bodyIndexFrameSource;
		ERROR_CHECK(kinect->get_BodyIndexFrameSource(&bodyIndexFrameSource));
		ERROR_CHECK(bodyIndexFrameSource->OpenReader(&bodyIndexFrameReader));
		CComPtr<IFrameDescription> bodyIndexFrameDescription;
		ERROR_CHECK(bodyIndexFrameSource->get_FrameDescription(&bodyIndexFrameDescription));
		bodyIndexFrameDescription->get_Width(&bodyIndexWidth);
		bodyIndexFrameDescription->get_Height(&bodyIndexHeight);
		bodyIndexBuffer.resize(bodyIndexWidth * bodyIndexHeight);

		colors[0] = cv::Vec3b(0, 0, 0);
		colors[1] = cv::Vec3b(255, 0, 0);
		colors[2] = cv::Vec3b(0, 255, 0);
		colors[3] = cv::Vec3b(0, 0, 255);
		colors[4] = cv::Vec3b(255, 255, 0);
		colors[5] = cv::Vec3b(255, 0, 255);
		colors[6] = cv::Vec3b(0, 255, 255);
		colors[7] = cv::Vec3b(255, 255, 255);

		bodyIndex_initialized = true;
	}
	void updateBodyIndexFrame() {
		if (!bodyIndex_initialized) initializeBodyIndexFrame();
		CComPtr<IBodyIndexFrame> bodyIndexFrame;
		auto ret = bodyIndexFrameReader->AcquireLatestFrame(&bodyIndexFrame);
		if (FAILED(ret)) return;
		ERROR_CHECK(bodyIndexFrame->CopyFrameDataToArray((UINT)bodyIndexBuffer.size(), &bodyIndexBuffer[0]));
	}
public:
	cv::Mat bodyIndexImage;
	void setBodyIndex(bool raw = true) { setBodyIndex(bodyIndexImage, raw); }
	void setBodyIndex(cv::Mat& bodyIndexImage, bool raw = true) {
		updateBodyIndexFrame();
		if (raw) {
			bodyIndexImage = cv::Mat(bodyIndexHeight, bodyIndexWidth, CV_8UC1);
			for (int i = 0; i < bodyIndexHeight*bodyIndexWidth; i++) {
				int y = i / bodyIndexWidth;
				int x = i % bodyIndexWidth;
				bodyIndexImage.at<uchar>(y, x) = bodyIndexBuffer[i];
			}
		}
		else {
			bodyIndexImage = cv::Mat(bodyIndexHeight, bodyIndexWidth, CV_8UC3);
			for (int i = 0; i < bodyIndexHeight*bodyIndexWidth; i++) {
				int y = i / bodyIndexWidth;
				int x = i % bodyIndexWidth;
				int c = (bodyIndexBuffer[i] != 255) ? bodyIndexBuffer[i] + 1 : 0;
				bodyIndexImage.at<cv::Vec3b>(y, x) = colors[c];
			}
		}
	}

	// ******** body ********
	CComPtr<IBodyFrameReader> bodyFrameReader = nullptr;
	BOOLEAN body_initialized = false;
	IBody* bodies[BODY_COUNT];
	void initializeBodyFrame() {
		CComPtr<IBodyFrameSource> bodyFrameSource;
		ERROR_CHECK(kinect->get_BodyFrameSource(&bodyFrameSource));
		ERROR_CHECK(bodyFrameSource->OpenReader(&bodyFrameReader));
		CComPtr<IFrameDescription> bodyFrameDescription;
		for (auto& body : bodies) {
			body = nullptr;
		}
		body_initialized = true;
	}
	void updateBodyFrame() {
		if (!body_initialized) initializeBodyFrame();
		CComPtr<IBodyFrame> bodyFrame;
		auto ret = bodyFrameReader->AcquireLatestFrame(&bodyFrame);
		if (FAILED(ret)) return;
		for (auto& body : bodies) {
			if (body != nullptr) {
				body->Release();
				body = nullptr;
			}
		}
		ERROR_CHECK(bodyFrame->GetAndRefreshBodyData(BODY_COUNT, &bodies[0]));
	}
public:
	// Joint.Position.{X,Y,Z}  // CameraSpacePoint
	//    DepthSpacePoint dp;
	//    coordinateMapper->MapCameraPointToDepthSpace(joint.Position, &dp);
	//    ColorSpacePoint cp;
	//    coordinateMapper->MapCameraPointToColorSpace(joint.Position, &cp);
	// Joint.TrackingState  == TrackingState::TrackingState_{Tracked,Inferred}
	vector<int> skeletonId;
	vector<UINT64> skeletonTrackingId;
	vector<vector<Joint> > skeleton;
	void setSkeleton() { setSkeleton(skeleton); }
	void setSkeleton(vector<vector<Joint> >& skeleton) {
		updateBodyFrame();
		skeleton.clear();
		skeletonId.clear();
		skeletonTrackingId.clear();
		for (int i = 0; i < BODY_COUNT; i++) {
			auto body = bodies[i];
			if (body == nullptr) continue;
			BOOLEAN isTracked = false;
			ERROR_CHECK(body->get_IsTracked(&isTracked));
			if (!isTracked) continue;
			vector<Joint> skel;
			Joint joints[JointType::JointType_Count];
			body->GetJoints(JointType::JointType_Count, joints);
			for (auto joint : joints) skel.push_back(joint);
			skeleton.push_back(skel);
			skeletonId.push_back(i);
			UINT64 trackingId;
			ERROR_CHECK(body->get_TrackingId(&trackingId));
			skeletonTrackingId.push_back(trackingId);
		}
	}
	// HandState::HandState_{Unknown,NotTracked,Open,Closed,Lasso}
	// TrackingConfidence::TrackingConfidence_{Low,Hight}
	pair<int, int> handState(int id = 0, bool isLeft = true) {
		if (!body_initialized) throw runtime_error("body not initialized");
		if (id < 0 || id >= BODY_COUNT) throw runtime_error("handstate: bad id " + id);
		pair<int, int> ans(HandState::HandState_Unknown, TrackingConfidence::TrackingConfidence_Low);
		//pair<int, int> ans(HandState::HandState_Unknown, 2);
		if (id >= skeletonId.size()) return ans;
		HRESULT hResult;
		auto body = bodies[skeletonId[id]];
		if (body == nullptr) return ans;
		BOOLEAN isTracked = false;
		ERROR_CHECK(body->get_IsTracked(&isTracked));
		if (!isTracked) return ans;
		HandState handState;
		TrackingConfidence handConfidence;
		if (isLeft) {
			hResult = body->get_HandLeftState(&handState);
			if (!SUCCEEDED(hResult)) return ans;
			hResult = body->get_HandLeftConfidence(&handConfidence);
			if (!SUCCEEDED(hResult)) return ans;
		}
		else {
			hResult = body->get_HandRightState(&handState);
			if (!SUCCEEDED(hResult)) return ans;
			hResult = body->get_HandRightConfidence(&handConfidence);
			if (!SUCCEEDED(hResult)) return ans;
		}
		ans.first = handState; ans.second = handConfidence;
		return ans;
	}

public:
	KinectLayer() { initialize(); }
	~KinectLayer() {
		if (kinect != nullptr) kinect->Close();
	}
	cv::Rect boundingBoxInColorSpace(vector<CameraSpacePoint>& v) {
		int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
		for (CameraSpacePoint sp : v) {
			ColorSpacePoint cp;
			coordinateMapper->MapCameraPointToColorSpace(sp, &cp);
			if (minX > (int)cp.X) minX = (int)cp.X;
			if (maxX < (int)cp.X) maxX = (int)cp.X;
			if (minY >(int)cp.Y) minY = (int)cp.Y;
			if (maxY < (int)cp.Y) maxY = (int)cp.Y;
		}
		if (maxX <= minX || maxY <= minY) return cv::Rect(0, 0, 0, 0);
		return cv::Rect(minX, minY, maxX - minX, maxY - minY);
	}
};