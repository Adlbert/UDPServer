#include "Client.h"
#include "MySDLEngine.h"

void Client::pgm_save(unsigned char* buf, int wrap, int xsize, int ysize, const char* filename) {
	FILE* f;
	int i;

	fopen_s(&f, filename, "w");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

int Client::decodeToImage(AVCodecContext* dec_ctx, SwsContext* img_convert_ctx,
	AVFrame* avFrameYUV, AVFrame* avFrameRGB, AVPacket* pkt, const char* filename, bool save) {
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, avFrameYUV);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return -1;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		engine->setTexture(avFrameYUV);

		if (save) {
			printf("saving frame %3d\n", dec_ctx->frame_number);
			fflush(stdout);

			img_convert_ctx = sws_getContext(
				avFrameYUV->width, avFrameYUV->height, dec_ctx->pix_fmt, avFrameYUV->width, avFrameYUV->height,
				AVPixelFormat::AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
			if (!img_convert_ctx) {
				fprintf(stderr, "Could not allocate image convert context\n");
				exit(1);
			}
			ret = av_image_alloc(avFrameRGB->data, avFrameRGB->linesize, avFrameYUV->width, avFrameYUV->height,
				AVPixelFormat::AV_PIX_FMT_RGB24, 32);
			if (ret < 0) {
				fprintf(stderr, "Could not allocate raw picture buffer\n");
				exit(6);
			}

			int out = sws_scale(img_convert_ctx, avFrameYUV->data, avFrameYUV->linesize, 0, avFrameYUV->height,
				avFrameRGB->data, avFrameRGB->linesize);
			if (!out) {
				AVERROR(ENOMEM);
				exit(1);
			}


			/* the picture is allocated by the decoder. no need to
			   free it */
			   //snprintf(buf, sizeof(buf), "media/screenshots/%s-%d.pgm", filename, dec_ctx->frame_number);
			   //std::string img_name("media/screenshots/Frame" + std::to_string(dec_ctx->frame_number) + ".png");
			   //stbi_write_png(img_name.c_str(), avFrameYUV->width, avFrameYUV->height, 3, avFrameRGB->data[0], avFrameRGB->linesize[0]);
			   //stbi_write_jpg(img_name.c_str(), avFrameYUV->width, avFrameYUV->height, 3, avFrameRGB->data[0], 3 * avFrameYUV->width);
			   //pgm_save(avFrameRGB->data[0], avFrameRGB->linesize[0], avFrameYUV->width, avFrameYUV->height, filename);
		}
		return 0;
	}
	return -1;
}

void Client::decodeToVideo(AVPacket* pkt, const char* filename, int framenumber, FILE* f, bool debugframe) {
	std::string name("media/screenshots/frame_decode" + std::to_string(framenumber) + ".jpg");
	if (debugframe) {
		std::cout << std::endl << framenumber << "_" << pkt->size << std::endl;
		for (int i = 0; i < 10; i++) {
			std::cout << ntohl(htonl(pkt->data[i * 40])) << " ";
		}
		std::cout << std::endl;
	}
	fwrite(pkt->data, 1, pkt->size, f);
	av_free_packet(pkt);
}

uint8_t* Client::addDataToPkt(uint8_t* pktData, uint32_t* buff, uint8_t index, uint32_t pktsize) {
	for (int i = 4; i < 1400; i++) {
		int j = (i - 4) + (index * 1396);
		if (j < pktsize)
			pktData[j] = buff[i];
	}
	return pktData;
}

uint8_t* Client::buildPkt(uint32_t** buff, uint8_t crecvBufindex, uint32_t pktsize) {
	uint8_t* pktData = new uint8_t[pktsize];
	int index = 0;
	int i = 0;
	while (index < crecvBufindex + 1) {
		if (buff[i][1] == index) {
			pktData == addDataToPkt(pktData, buff[i], index, pktsize);
			index++;
		}
		i++;
		if (i > crecvBufindex)
			i = 0;
	}
	return pktData;
}

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

bool Client::bufferIsComplete(uint32_t** buff, uint8_t crecvBufindex) {
	uint32_t* fragNums = new uint32_t[crecvBufindex];
	//store fragNums in new array
	for (int i = 0; i < crecvBufindex; i++) {
		fragNums[i] = buff[i][1];
	}
	//Sort array of fragments accending
	std::qsort(fragNums, crecvBufindex, sizeof(uint32_t), compare);
	//Check if first element is 0-Element
	if (fragNums[0] != 0)
		return false;
	//Check if an element is missing
	for (int i = 0; i < crecvBufindex; i++) {
		uint32_t x;
		uint32_t y;
		if (i == crecvBufindex - 1) {
			x = fragNums[i] - 1;
			y = fragNums[i - 1];
		}
		else {
			x = fragNums[i] + 1;
			y = fragNums[i + 1];
		}
		if (x != y)
			return false;
	}
	return true;
}

int Client::recvtask() {
	sockaddr_in client; // Use to hold the client information (port / ip address)
	int clientLength = sizeof(client); // The size of the client information

	bool debugFragments = false;
	bool debugFrames = false;

	int cBufindex = 0;
	int buffsize = 20;
	int recvBuffsize = 1400;
	uint32_t recvbuff[1400];
	uint32_t** buff = new uint32_t * [buffsize];
	for (int i = 0; i < buffsize; ++i)
		buff[i] = new uint32_t[recvBuffsize];
	do {
		ZeroMemory(&client, clientLength); // Clear the client structure
		ZeroMemory(recvbuff, recvBuffsize); // Clear the receive buffer

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

		int prevBuffIndex = cBufindex - 1;
		uint32_t frameCount = recvbuff[0];
		uint32_t recvFragnum = recvbuff[1];
		uint32_t isSingleFlag = recvbuff[2];
		uint32_t pktsize = recvbuff[3];
		if (debugFragments) {
			std::cout << frameCount << "_";
			std::cout << recvFragnum << "_";
			std::cout << isSingleFlag << "_";
			std::cout << pktsize << "_";
			for (int i = 0; i < 10; i++) {
				std::cout << recvbuff[i * 70];
			}
			std::cout << std::endl;
			std::cout << "cBufindex: " << cBufindex;
			std::cout << std::endl;
		}
		//Save Previous Frame
		//Check if buffer is empty
		if (recvFragnum == 0 && prevBuffIndex >= 0) {
			bool prevsingle = buff[prevBuffIndex][2] == 1;
			//Check if fragments in buffer are complete
			bool buffComplete = true;
			if (!prevsingle)
				buffComplete = bufferIsComplete(buff, prevBuffIndex);
			if (buffComplete) {
				//BuildPacket
				uint8_t* pktData = buildPkt(buff, prevBuffIndex, buff[0][3]);
				//Save Screenshot
				pkt->data = pktData;
				pkt->size = buff[0][3];
				int ret = decodeToImage(image_ctx, img_convert_ctx, avFrameYUV, avFrameRGB, pkt, outfilename, false);
				//decodeToVideo(pkt, outfilename, image_ctx->frame_number, f, false);
				//Debug Frames
				if (debugFrames) {
					std::cout << std::endl << "Write frame " << buff[0][0] << " with packet size " << buff[0][3] << std::endl;
					for (int i = 0; i < 10; i++) {
						std::cout << pktData[i * 40] << " ";
					}
					std::cout << std::endl;
				}
			}
			//Delete Buffer
			delete[] buff;
			buff = new uint32_t * [buffsize];
			for (int i = 0; i < buffsize; ++i)
				buff[i] = new uint32_t[recvBuffsize];
			cBufindex = 0;
		}

		//Fill next Buffer	
		for (int i = 0; i < recvBuffsize; i++) {
			buff[cBufindex][i] = recvbuff[i];
		}

		//Increment Index
		if (cBufindex < buffsize - 1)
			cBufindex++;
		else
			cBufindex = 0;
	} while (true);
}


Client::Client() {
	filename = "/MPG4Video_highbitrate.mpg";
	outfilename = "test";
	image_ctx = NULL;
	img_convert_ctx = NULL;
	inPort = htons(54000);
	outPort = htons(54001);
	engine = NULL;


	int extentWidth = 800;
	int extentHeight = 600;


	fopen_s(&f, filename, "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(4);
	}

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
		exit(-1);
	}

	// Create a socket, notice that it is a user datagram socket (UDP)
	in = socket(AF_INET, SOCK_DGRAM, 0);

	// Create a server hint structure for the server
	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = inPort; // Convert from little to big endian

	// Try and bind the socket to the IP and port
	if (bind(in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << std::endl;
		exit(-1);
	}

	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	/* find the MPEG-4 video decoder */
	image_codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
	if (!image_codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}


	/*Allocate the image context*/
	image_ctx = avcodec_alloc_context3(image_codec);
	if (!image_ctx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	/*
	* Image Context
	*/
	/* For some codecs, such as msmpeg4 and mpeg4, width and height
	   MUST be initialized there because this information is not
	   available in the bitstream. */

	   /* open it */
	if (avcodec_open2(image_ctx, image_codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}
	image_ctx->width = 800;
	image_ctx->height = 600;
	image_ctx->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;



	avFrameYUV = av_frame_alloc();
	if (!avFrameYUV) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	//Maybe to use if video does not fit
	//avFrameYUV->format = video_ctx->pix_fmt;
	//avFrameYUV->width = video_ctx->width;
	//avFrameYUV->height = video_ctx->height;

	avFrameRGB = av_frame_alloc();
	if (!avFrameRGB) {
		fprintf(stderr, "Could not allocate picture frame\n");
		exit(1);
	}

	//Maybe to use if video does not fit
	//avFrameRGB->format = video_ctx->pix_fmt;
	//avFrameRGB->width = video_ctx->width;
	//avFrameRGB->height = video_ctx->height;
}

Client::~Client() {
	avcodec_free_context(&image_ctx);
	av_frame_free(&avFrameYUV);
	av_frame_free(&avFrameRGB);
	av_packet_free(&pkt);
	// Close socket
	closesocket(in);

	WSACleanup();
}

void Client::sendKey(uint32_t* key) {
	// Create a hint structure for the server
	sockaddr_in server;
	server.sin_family = AF_INET; // AF_INET = IPv4 addresses
	server.sin_port = outPort; // Little to big endian conversion
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr); // Convert from string to byte array

	// Socket creation, note that the socket type is datagram
	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

	// Write out to that socket
	int sendOk = sendto(out, (char*)key, sizeof(uint32_t) * 1, 0, (sockaddr*)&server, sizeof(server));


	if (sendOk == SOCKET_ERROR)
	{
		std::cout << "That didn't work! " << WSAGetLastError() << std::endl;
	}

	// Close the socket
	closesocket(out);
}

void Client::runrecvtask() {
	std::thread recvtask(&Client::recvtask, this);
	recvtask.detach();
}

void Client::setEngine(MySDLEngine* engine) {
	this->engine = engine;
}