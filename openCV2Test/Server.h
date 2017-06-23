#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef UNICODE
#define UNICODE
#endif



// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#define REVBUFFLEN 8142

int inline startServer()
{
	// Initialize Winsock.
	run = true;

	char recvBuffer[REVBUFFLEN];
	std::vector<std::string> points = { "P1:127.531/48.848/17.8\n"
		, "P2:83.939/392.19/3.123\n"
		, "P3:91.930/201.83/91.872\n" };
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
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
	InetPton(AF_INET, (PCWSTR) "127.0.0.1", &(service.sin_addr));
	service.sin_port = htons(27015);
	iResult = bind(ListenSocket, (sockaddr *)& service, sizeof service);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ListenSocket, 1) == SOCKET_ERROR)
	{
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	//----------------------
	// Create a SOCKET for accepting incoming requests.
	SOCKET AcceptSocket;
	wprintf(L"Waiting for client to connect...\n");

	//----------------------
	// Accept the connection.

	std::string msg = "Hallo there!";

	while (run)
	{
		AcceptSocket = accept(ListenSocket, NULL, NULL);

		if (AcceptSocket == INVALID_SOCKET)
		{
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		else
		{
			wprintf(L"Client connected.\n");

			recv(AcceptSocket, recvBuffer, REVBUFFLEN, 0);
			std::cout << "Received: " << std::string(recvBuffer) << std::endl;
			if (std::string(recvBuffer).compare("send_points") == 0) {
				for (auto s : points) {
					send(AcceptSocket, s.c_str(), s.length() + 1, MSG_OOB);
				}
			}

		}
	}
	// No longer need server socket
	closesocket(ListenSocket);
	std::cout << "Connection closed" << std::endl;
	std::cin;
	WSACleanup();
	return 0;
}