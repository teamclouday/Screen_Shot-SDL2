#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>
#include <windows.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <iostream>

#define WINDOW_TITLE "ScreenShot"
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

SDL_Texture *image = nullptr;
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
std::vector<int> windowPositions(4, 0);

enum CurrentState
{
    DEFAULT,
    SELECTING,
    SAVING
};

void switchRenderer(CurrentState *state);
void pollEvents(unsigned char *quit, CurrentState *state);
bool loadImage(const std::string name);
void saveImage();
void updateWindowPos();
SDL_Surface *loadSurfaceFromBITMAP(BITMAP *bmp);

int main(int argc, char **argv)
{
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Failed to init SDL!\nSDL_ERROR: %s\n", SDL_GetError());
        return -1;
    }

    // Init image
    int imageFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if(!(!IMG_Init(imageFlags) ^ imageFlags))
    {
        printf("Failed to init IMG!\nIMG_ERROR: %s\n", IMG_GetError());
        return -1;
    }

    // create SDLwindow
    window = SDL_CreateWindow(WINDOW_TITLE,
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH,
                                          WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if(!window)
    {
        printf("Failed to create SDL_Window!\nSDL_ERROR: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return -2;
    }

    // create window renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer)
    {
        printf("Failed to create SDL_Renderer!\nSDL_ERROR: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return -3;
    }

    // load images
    if(!loadImage("cover.png"))
    {
        IMG_Quit();
        SDL_Quit();
        return -4;
    }

    unsigned char quit = 0;
    CurrentState state = DEFAULT;
    while(!quit)
    {
        pollEvents(&quit, &state);
        switchRenderer(&state);
        SDL_Delay(50);
    }

    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

void switchRenderer(CurrentState *state)
{
    SDL_RenderClear(renderer);
    switch(*state)
    {
        case DEFAULT:
            SDL_RenderCopy(renderer, image, NULL, NULL);
            break;
        case SAVING:
            saveImage();
            *state = DEFAULT;
            break;
        case SELECTING:
            break;
    }
    SDL_RenderPresent(renderer);
}

void saveImage()
{
    HDC dcScreen = GetDC(NULL);
    HDC dcTarget = CreateCompatibleDC(dcScreen);
    // create bitmap info
    BITMAPINFO bi;
    memset(&bi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = windowPositions[2];
    bi.bmiHeader.biHeight = -windowPositions[3];
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    VOID *pvBits;
    HBITMAP bmpTarget = CreateDIBSection(dcScreen, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    SelectObject(dcTarget, bmpTarget);
    BitBlt(dcTarget, 0, 0, windowPositions[2], windowPositions[3], dcScreen, windowPositions[0], windowPositions[1], SRCCOPY);

    BITMAP bitmap;
    GetObject(bmpTarget, sizeof(BITMAP), &bitmap);
    SDL_Surface *surface = loadSurfaceFromBITMAP(&bitmap);

    bool done = false;
    while(!done)
    {
        printf("Enter the path and name you want to store:\n");
        std::string pathname;
        std::getline(std::cin, pathname);
        int success = SDL_SaveBMP(surface, pathname.c_str());
        if(success)
            printf("Invalid path! Try again!\n");
        else
            done = true;
    }

    SDL_FreeSurface(surface);

    DeleteDC(dcTarget);
    ReleaseDC(NULL, dcScreen);
    SDL_Delay(100);
    SDL_ShowWindow(window);
    SDL_SetWindowSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_SetWindowOpacity(window, 1.0f);
}

void pollEvents(unsigned char *quit, CurrentState *state)
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT)
        {
            *quit = 1;
            return;
        }
        else if(e.type == SDL_KEYDOWN)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    *quit = 1;
                    break;
                case SDLK_s:
                    if(*state != SAVING)
                    {
                        SDL_SetWindowResizable(window, SDL_TRUE);
                        SDL_SetWindowOpacity(window, 0.2f);
                        *state = SELECTING;
                    }
                    break;
                case SDLK_r:
                    if(*state != DEFAULT)
                    {
                        SDL_ShowWindow(window);
                        SDL_SetWindowSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
                        SDL_SetWindowResizable(window, SDL_FALSE);
                        SDL_SetWindowOpacity(window, 1.0f);
                        *state = DEFAULT;
                    }
                    break;
                case SDLK_q:
                    if(*state != DEFAULT)
                    {
                        updateWindowPos();
                        SDL_HideWindow(window);
                        *state = SAVING;
                    }
                    break;
            }
        }
    }
}

bool loadImage(const std::string name)
{
    SDL_Surface *loadIMG = IMG_Load(("./Images/" + name).c_str());
    if(!loadIMG)
    {
        printf("Failed to load images!\nIMG_ERROR: %s\n", IMG_GetError());
        return false;
    }
    image = SDL_CreateTextureFromSurface(renderer, loadIMG);
    SDL_FreeSurface(loadIMG);
    if(!image)
    {
        printf("Failed to create image texture!\nIMG_ERROR: %s\n", IMG_GetError());
        return false;
    }
    return true;
}

void updateWindowPos()
{
    int width, height;
    int x, y;
    SDL_GetWindowSize(window, &width, &height);
    SDL_GetWindowPosition(window, &x, &y);
    windowPositions[0] = x;
    windowPositions[1] = y;
    windowPositions[2] = width;
    windowPositions[3] = height;
}

// This function is inspired by GarlandlX's answer on:
// https://www.gamedev.net/forums/topic/315178-hbitmap-to-sdl_surface/
SDL_Surface *loadSurfaceFromBITMAP(BITMAP *bmp)
{
    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                             bmp->bmWidth,
                                             bmp->bmHeight,
                                             bmp->bmBitsPixel,
                                             0x000000FF,
                                             0x0000FF00,
                                             0x00FF0000,
                                             0xFF000000);
    Uint8 *bits = new Uint8[bmp->bmWidthBytes * bmp->bmHeight];
    memcpy(bits, bmp->bmBits, bmp->bmWidthBytes * bmp->bmHeight);

    // reverse BGRA to RGBA
    for (int i = 0; i < bmp->bmWidthBytes * bmp->bmHeight; i += 4) 
	{
		Uint8 tmp;
		tmp = bits[i];
		bits[i] = bits[i+2];
		bits[i+2] = tmp;
	}

    SDL_LockSurface(surf);
    memcpy(surf->pixels, bits, bmp->bmWidthBytes * bmp->bmHeight);
    SDL_UnlockSurface(surf);

    delete [] bits;

    SDL_Surface *surface = SDL_ConvertSurfaceFormat(surf, SDL_GetWindowPixelFormat(window), 0);
    SDL_FreeSurface(surf);

    return surface;
}