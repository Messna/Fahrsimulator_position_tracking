#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef UNICODE
#define UNICODE
#endif

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#define REVBUFFLEN 8142

const cv::Mat transformation_mat = (cv::Mat_<double>(4, 4) <<
	0.0918317, 0.000775034, -0.00115236, -0.232738,
	-0.000989388, 0.0899911, -0.0183199, 3.92404,
	0.000974538, 0.0183302, 0.0899891, -11.552,
	0, 0, 0, 1);

inline int startServer() {
	// Initialize Winsock.
	bool run = true;

	char recvBuffer[REVBUFFLEN];
	WSADATA wsaData;
	const int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	const SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	//string ip = "127.0.0.1";
	//InetPton(AF_INET, LPCTSTR(ip.c_str()), &service.sin_addr.s_addr);
	service.sin_port = htons(27015);

	if (::bind(ListenSocket, reinterpret_cast<SOCKADDR *>(& service), sizeof service) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ListenSocket, 1) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	string msg = "Hallo there!";

	while (run) {
		wprintf(L"Waiting for client to connect...\n");
		const SOCKET AcceptSocket = accept(ListenSocket, nullptr, nullptr);

		if (AcceptSocket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		wprintf(L"Client connected.\n");

		while (true) {
			recv(AcceptSocket, recvBuffer, REVBUFFLEN, 0);
			cout << "Received: " << string(recvBuffer) << endl;
			if (string(recvBuffer).compare("send_points") == 0) {
				string s = "";
				for (const auto p : realCoordsMap) {
					// Format: "P1:127.531/48.848/17.8"
					const cv::Mat point_mat = (cv::Mat_<double>(4, 1) << p.second[0] * 100, p.second[1] * 100, p.second[2] * 100, 1);
					cv::Mat transformed_mat = transformation_mat * point_mat;
					s += p.first + ":" + to_string(transformed_mat.at<double>(0, 0)) + "/" + to_string(
							transformed_mat.at<double>(1, 0))
						+ "/" + to_string(transformed_mat.at<double>(2, 0)) + "\n";
					cout << s << endl;
				}
				send(AcceptSocket, s.c_str(), s.length() + 1, MSG_OOB);
				cout << "Sent data" << endl;
			}
			else { break; }
			for (int i = 0; i < REVBUFFLEN; i++) {
				recvBuffer[i] = '\0';
			}
		}
	}
	// No longer need server socket
	closesocket(ListenSocket);
	cout << "Connection closed" << endl;
	WSACleanup();
	return 0;
}
