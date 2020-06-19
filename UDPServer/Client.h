#ifndef Client_H
#define Client_H
#include <iostream>
#include <WS2tcpip.h>


#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <future>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image/stb_image.h">
//
//#define STBI_MSC_SECURE_CRT
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image/stb_image_write.h"

// Include the Winsock library (lib) file
#pragma comment (lib, "ws2_32.lib")
#pragma warning(disable : 4996)

class MySDLEngine;

class Client {
private:
	const char* outfilename;
	const AVCodec* image_codec;
	AVCodecContext* image_ctx;
	SwsContext* img_convert_ctx;
	AVFrame* avFrameYUV;
	AVFrame* avFrameRGB;
	AVPacket* pkt;
	SOCKET in;
	USHORT inPort;
	USHORT outPort;
	FILE* f;
	MySDLEngine* engine;

	const char* filename;

	static void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize,
		const char* filename);

	int decodeToImage(AVCodecContext* dec_ctx, SwsContext* img_convert_ctx,
		AVFrame* avFrameYUV, AVFrame* avFrameRGB, AVPacket* pkt, const char* filename, bool save);

	static void decodeToVideo(AVPacket* pkt, const char* filename, int framenumber, FILE* f, bool debugframe);

	bool bufferIsComplete(uint32_t** buff, uint8_t crecvBufindex);

	uint8_t* addDataToPkt(uint8_t* pktData, uint32_t* buff, uint8_t index, uint32_t pktsize);

	uint8_t* buildPkt(uint32_t** buff, uint8_t crecvBufindex, uint32_t pktsize);

	int recvtask();

public:
	Client();
	~Client();

	void setEngine(MySDLEngine* engine);

	void sendKey(uint32_t* key);

	void runrecvtask();
};
#endif Client_H
