#ifndef MySDLEngine_H
#define MySDLEngine_H
#include <iostream>
#include "Client.h"

extern "C" {
#include <SDL2/SDL.h>
#include <SDL_image/SDL_image.h>
#undef main
}

class MySDLEngine {
private:
	enum KeyPressSurfaces
	{
		KEY_PRESS_SURFACE_DEFAULT,
		KEY_PRESS_SURFACE_UP,
		KEY_PRESS_SURFACE_DOWN,
		KEY_PRESS_SURFACE_LEFT,
		KEY_PRESS_SURFACE_RIGHT,
		KEY_PRESS_SURFACE_TOTAL
	};

	//Screen dimension constants
	const int SCREEN_WIDTH = 640;
	const int SCREEN_HEIGHT = 480;

	//The window we'll be rendering to
	SDL_Window* gWindow;

	SDL_Surface* gScreenSurface;

	//The window renderer
	SDL_Renderer* gRenderer;

	//Current displayed texture
	SDL_Texture* gTexture;

	Client* client;

	SDL_Texture* loadTextureFromFile(std::string path);

	void close();

public:
	MySDLEngine();
	~MySDLEngine();

	void setTexture(AVFrame* avFrameYUV);

	void setClient(Client* client);

	bool loadMedia();

	void run();
};
#endif MySDLEngine_H