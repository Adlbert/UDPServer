#include "MySDLEngine.h"
#include <stdio.h>

SDL_Texture* MySDLEngine::loadTextureFromFile(std::string path) {
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	return newTexture;
}

bool MySDLEngine::loadMedia() {
	//Loading success flag
	bool success = true;

	//Load PNG texture
	gTexture = loadTextureFromFile("Frame1.jpg");
	if (gTexture == NULL)
	{
		printf("Failed to load texture image!\n");
		success = false;
	}

	return success;
}

void MySDLEngine::close() {
	//Free loaded image
	SDL_DestroyTexture(gTexture);
	gTexture = NULL;

	//Destroy window
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

void MySDLEngine::run() {
	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;


	// Enter a loop
	while (!quit)
	{
		if (false) {

		}

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			//User presses a key
			else if (e.type == SDL_KEYDOWN)
			{
				uint32_t arr[1];
				//Select surfaces based on key press
				switch (e.key.keysym.sym)
				{
				case SDLK_w:
					arr[0] = 0;
					client->sendKey(arr);
					break;

				case SDLK_s:
					arr[0] = 1;
					client->sendKey(arr);
					break;

				case SDLK_a:
					arr[0] = 2;
					client->sendKey(arr);
					break;

				case SDLK_d:
					arr[0] = 3;
					client->sendKey(arr);
					break;

				case SDLK_LEFT:
					arr[0] = 4;
					client->sendKey(arr);
					break;

				case SDLK_RIGHT:
					arr[0] = 5;
					client->sendKey(arr);
					break;

				default:
					break;
				}
			}
		}

		//Clear screen
		SDL_RenderClear(gRenderer);

		//Render texture to screen
		SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
		//Apply the current image

		//Update screen
		SDL_RenderPresent(gRenderer);
	}
}

MySDLEngine::MySDLEngine() {

	client = NULL;
	gWindow = NULL;
	gRenderer = NULL;
	gScreenSurface = NULL;
	gTexture = NULL;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(-1);
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			exit(-1);
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				exit(-1);
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				gScreenSurface = SDL_GetWindowSurface(gWindow);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG || IMG_INIT_JPG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					exit(-1);
				}
			}
		}
	}
}

MySDLEngine::~MySDLEngine() {}

void MySDLEngine::setTexture(AVFrame* avFrameYUV) {
	SDL_Texture* bmp = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, avFrameYUV->width, avFrameYUV->height);
	int ret = SDL_UpdateYUVTexture(bmp, NULL, avFrameYUV->data[0], avFrameYUV->linesize[0], avFrameYUV->data[1], avFrameYUV->linesize[1], avFrameYUV->data[2], avFrameYUV->linesize[2]);
	gTexture = bmp;
}

void MySDLEngine::setClient(Client* client) {
	this->client = client;
}

