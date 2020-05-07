#include <iostream>
#include <WS2tcpip.h>

#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h">

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Include the Winsock library (lib) file
#pragma comment (lib, "ws2_32.lib")

int compare(const void* a, const void* b)
{
	const int* x = (int*)a;
	const int* y = (int*)b;

	if (*x > * y)
		return 1;
	else if (*x < *y)
		return -1;

	return 0;
}

bool bufferIsComplete(uint32_t** buff, uint8_t crecvBufindex) {
	uint32_t* fragNums = new uint32_t[crecvBufindex];
	//store fragNums in new array
	for (int i = 0; i < crecvBufindex; i++) {
		fragNums[i] = buff[i][1];
		//for (int j = 0; j < 1400; j++) {
		//	std::cout << buff[i][j] << " ";
		//}
		//std::cout << std::endl;
	}
	std::qsort(fragNums, crecvBufindex, sizeof(uint32_t), compare);
	//for (int i = 0; i < crecvBufindex; i++) {
	//	std::cout << fragNums[i] << " ";
	//}
	//std::cout << std::endl;
	if (fragNums[0] != 0)
		return false;
	for (int i = 0; i < crecvBufindex; i++) {
		uint32_t x;
		uint32_t y;
		if (i == crecvBufindex - 1) {
			x = fragNums[i] - 1;
			y = fragNums[i - 1];
			//std::cout << i << "_" << x << "_" << y << std::endl;
		}
		else {
			x = fragNums[i] + 1;
			y = fragNums[i + 1];
			//std::cout << i << "_" << x << "_" << y << std::endl;
		}
		if (x != y)
			return false;
	}
	return true;
}

uint32_t* AddDataToPkt(uint32_t* pktData, uint32_t* buff, uint8_t crecvBufindex) {
	for (int i = 1; i < 1400; i++) {
		int index = (i - 1) * (crecvBufindex + 1);
		pktData[index] = buff[i];
	}
	return pktData;
}

uint32_t* BuildPkt(uint32_t** buff, uint8_t crecvBufindex) {
	uint32_t* pktData = new uint32_t[(crecvBufindex + 1) * 1398];
	int index = 0;
	if (index < crecvBufindex) {
		for (int i = 0; i < crecvBufindex; i++) {
			if (buff[i][1] == index) {
				pktData == AddDataToPkt(pktData, buff[i], index);
			}
		}
	}
	return pktData;
}

void main()
{
	const char* filename = "D:\\Projects\\UDPServer\\MPG1Video_highbitrate.mpg";
	FILE* f;
	fopen_s(&f, filename, "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(4);
	}

	////////////////////////////////////////////////////////////
	// INITIALIZE WINSOCK
	////////////////////////////////////////////////////////////

	// Structure to store the WinSock version. This is filled in
	// on the call to WSAStartup()
	WSADATA data;

	// To start WinSock, the required version must be passed to
	// WSAStartup(). This server is going to use WinSock version
	// 2 so I create a word that will store 2 and 2 in hex i.e.
	// 0x0202
	WORD version = MAKEWORD(2, 2);

	// Start WinSock
	int wsOk = WSAStartup(version, &data);
	if (wsOk != 0)
	{
		// Not ok! Get out quickly
		std::cout << "Can't start Winsock! " << wsOk;
		return;
	}

	////////////////////////////////////////////////////////////
	// SOCKET CREATION AND BINDING
	////////////////////////////////////////////////////////////

	// Create a socket, notice that it is a user datagram socket (UDP)
	SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);

	// Create a server hint structure for the server
	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(54000); // Convert from little to big endian

	// Try and bind the socket to the IP and port
	if (bind(in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << std::endl;
		return;
	}

	////////////////////////////////////////////////////////////
	// MAIN LOOP SETUP AND ENTRY
	////////////////////////////////////////////////////////////

	sockaddr_in client; // Use to hold the client information (port / ip address)
	int clientLength = sizeof(client); // The size of the client information

	bool debugFragments = false;
	bool debugFrames = false;

	int cBufindex = 0;
	int buffsize = 20;
	int recvBuffsize = 1400;
	uint32_t prevFragNum = -1;
	uint32_t recvbuff[1400];
	uint32_t** buff = new uint32_t * [buffsize];
	for (int i = 0; i < buffsize; ++i)
		buff[i] = new uint32_t[recvBuffsize];

	// Enter a loop
	while (true)
	{
		ZeroMemory(&client, clientLength); // Clear the client structure
		ZeroMemory(recvbuff, recvBuffsize); // Clear the receive buffer

		// Wait for message
		int bytesIn = recvfrom(in, (char*)recvbuff, sizeof(uint32_t) * recvBuffsize, 0, (sockaddr*)&client, &clientLength);
		if (bytesIn == SOCKET_ERROR)
		{
			std::cout << "Error receiving from client " << WSAGetLastError() << std::endl;
			continue;
		}

		// Display message and client info
		char clientIp[256]; // Create enough space to convert the address byte array
		ZeroMemory(clientIp, 256); // to string of characters

		// Convert from byte array to chars
		inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);


		if (debugFragments) {
			std::cout << ntohl(recvbuff[0]) << "_";
			std::cout << ntohl(recvbuff[1]) << "_";
			for (int i = 0; i < 10; i++) {
				std::cout << ntohl(recvbuff[i * 70]);
			}
			std::cout << std::endl;
		}
		uint32_t recvFragnum = ntohl(recvbuff[1]);
		uint32_t frameCount = ntohl(recvbuff[0]);
		if (recvFragnum == 0) {
			std::string name("saved Frame " + std::to_string(buff[0][0]));
			std::string img_name("media/screenshots/Frame" + std::to_string(buff[0][0]) + ".jpg");
			//Save P Frame
			if (prevFragNum == 0) {
				//Need to remove header
				//Save Screenshot
				stbi_write_jpg(img_name.c_str(), 8, 6, 4, buff[0], 4 * 8);
				//Debug Frames
				if (debugFrames) {
					std::cout << name << std::endl;
					std::cout << std::endl << frameCount << std::endl;
					for (int i = 0; i < 10; i++) {
						std::cout << ntohl(buff[0][i * 40]) << " ";
					}
				}
				//SaveFrame
				fwrite(buff[0], 1, cBufindex * 1398, f);
				//Delete Buffer
				delete[] buff;
				buff = new uint32_t * [buffsize];
				for (int i = 0; i < buffsize; ++i)
					buff[i] = new uint32_t[recvBuffsize];
				cBufindex = 0;
			}
			//Check if buffer is empty
			if (cBufindex != 0) {
				//Check if fragments in buffer are complete
				if (bufferIsComplete(buff, cBufindex)) {
					uint32_t* pktData = BuildPkt(buff, cBufindex);
					//Save Screenshot
					stbi_write_jpg(img_name.c_str(), 8, 6, 4, pktData, 4 * 8);
					//Debug Frames
					if (debugFrames) {
						std::cout << name << std::endl;
						std::cout << std::endl << frameCount << std::endl;
						for (int i = 0; i < 10; i++) {
							std::cout << pktData[i * 40] << " ";
						}
					}
					//SaveFrame
					fwrite(pktData, 1, cBufindex * 1398, f);
				}
				else {
					//Delete Buffer
					delete[] buff;
					buff = new uint32_t * [buffsize];
					for (int i = 0; i < buffsize; ++i)
						buff[i] = new uint32_t[recvBuffsize];
					cBufindex = 0;
				}
			}
		}

		//Fill next Buffer
		for (int i = 0; i < recvBuffsize; i++) {
			buff[cBufindex][i] = ntohl(recvbuff[i]);
		}

		//Increment Index
		if (cBufindex < buffsize - 1)
			cBufindex++;
		else
			cBufindex = 0;

		//Set prev Fragment num
		prevFragNum = recvFragnum;

	}

	// Close socket
	closesocket(in);

	// Shutdown winsock
	WSACleanup();
}

