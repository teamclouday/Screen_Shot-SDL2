#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>

// set window properties
#define WINDOW_TITLE "ScreenShot"
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
// set mouse magnification width and height
#define RECT_SELECT_WIDTH  40
#define RECT_SELECT_HEIGHT 40
// set magifier width and height
#define MAG_WIDTH  100
#define MAG_HEIGHT 100
// global variables
SDL_Texture *image = nullptr;
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_DisplayMode DM;
std::vector<int> mousePosition(2, 0);
std::vector<int> selectPositions(4, 0);
// state info for window
enum CurrentState
{
    DEFAULT,
    SELECTING,
    SAVING
};
// state info for mouse
enum MouseState
{
    MOUSE_DEFAULT,
    MOUSE_DOWN,
    MOUSE_SELECTING,
    MOUSE_UP,
    MOUSE_DONE
};
// functions header
void switchRenderer(CurrentState *state, MouseState *mstate);
void pollEvents(unsigned char *quit, CurrentState *state, MouseState *mstate);
bool loadImage(const std::string name);
void drawMagnifier();
void drawSelectRect(MouseState *mstate);
void saveImage();
void updateMousePos();
SDL_Surface *loadSurfaceFromScreen(int x, int y, int w, int h);

int main(int argc, char **argv)
{
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Failed to init SDL!\nSDL_ERROR: %s\n", SDL_GetError());
        return -1;
    }

    // Init image
    int imageFlags = IMG_INIT_PNG;
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

    // load cover image
    if(!loadImage("cover.png"))
    {
        IMG_Quit();
        SDL_Quit();
        return -4;
    }

    // get display mode
    SDL_GetCurrentDisplayMode(0, &DM);
    // set default values of the states
    unsigned char quit = 0;
    CurrentState state = DEFAULT;
    MouseState mstate = MOUSE_DEFAULT;
    while(!quit)
    {
        pollEvents(&quit, &state, &mstate);
        switchRenderer(&state, &mstate);
        SDL_Delay(20);
    }
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

// function that sets renderer according to the state info
void switchRenderer(CurrentState *state, MouseState *mstate)
{
    // set default background color: black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    switch(*state)
    {
        case DEFAULT:
            SDL_RenderCopy(renderer, image, NULL, NULL);
            break;
        case SAVING:
            saveImage();
            // after saving, automatically change to DEFAULT state
            *state = DEFAULT;
            break;
        case SELECTING:
            updateMousePos();
            drawSelectRect(mstate);
            drawMagnifier();
            break;
    }
    // draw on screen
    SDL_RenderPresent(renderer);
}

// function that draw the selected area with rectangle
void drawSelectRect(MouseState *mstate)
{
    if(*mstate == MOUSE_DOWN)
    {
        selectPositions[0] = mousePosition[0];
        selectPositions[1] = mousePosition[1];
        // once mouse is done, enter selecting state
        *mstate = MOUSE_SELECTING;
    }
    else if(*mstate == MOUSE_UP)
    {
        // get width and height
        int w = mousePosition[0] - selectPositions[0];
        int h = mousePosition[1] - selectPositions[1];
        // get x and y
        if(w < 0)
        {
            w = -w;
            selectPositions[0] = mousePosition[0];
        }
        if(h < 0)
        {
            h = -h;
            selectPositions[1] = mousePosition[1];
        }
        // save x, y, w, h in selectPositions
        selectPositions[2] = w;
        selectPositions[3] = h;
        *mstate = MOUSE_DONE;
    }
    else if(*mstate == MOUSE_SELECTING)
    {
        SDL_Rect rect;
        int w = selectPositions[0] - mousePosition[0];
        int h = selectPositions[1] - mousePosition[1];
        if(w < 0)
        {
            rect.x = selectPositions[0];
            rect.w = -w;
        }
        else
        {
            rect.x = mousePosition[0];
            rect.w = w;
        }
        if(h < 0)
        {
            rect.y = selectPositions[1];
            rect.h = -h;
        }
        else
        {
            rect.y = mousePosition[1];
            rect.h = h;
        }
        // draw the rectangle (white) during selecting
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    else if(*mstate == MOUSE_DONE)
    {
        // once done, draw the selected rect (white) on screen
        SDL_Rect rect;
        rect.x = selectPositions[0];
        rect.y = selectPositions[1];
        rect.w = selectPositions[2];
        rect.h = selectPositions[3];
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

// draw the magnifier near the mouse
void drawMagnifier()
{
    // get the top left (x, y) of the mouse area
    int x = mousePosition[0] - (int)(RECT_SELECT_WIDTH / 2);
    int y = mousePosition[1] - (int)(RECT_SELECT_HEIGHT / 2);
    // get mouse area bitmap surface
    SDL_Surface *magSurf = loadSurfaceFromScreen(x, y, RECT_SELECT_WIDTH, RECT_SELECT_HEIGHT);
    SDL_Texture *magTex = SDL_CreateTextureFromSurface(renderer, magSurf);
    SDL_FreeSurface(magSurf);

    int mX = mousePosition[0];
    int mY = mousePosition[1];
    int sW = DM.w;
    int sH = DM.h;
    SDL_Rect magRect;
    magRect.w = MAG_WIDTH;
    magRect.h = MAG_HEIGHT;
    if((mX + 20 + MAG_WIDTH) > sW)
        magRect.x = mX - 20 -  MAG_WIDTH;
    else
        magRect.x = mX + 20;
    if((mY + 20 + MAG_HEIGHT) > sH)
        magRect.y = mY - 20 - MAG_HEIGHT;
    else
        magRect.y = mY + 20;
    // draw the magnifier
    SDL_RenderCopy(renderer, magTex, NULL, &magRect);
    // draw cross lines in middle
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, magRect.x + (MAG_WIDTH / 5 * 2), magRect.y + (MAG_HEIGHT / 2), magRect.x + (MAG_WIDTH / 5 * 3), magRect.y + (MAG_HEIGHT / 2));
    SDL_RenderDrawLine(renderer, magRect.x + (MAG_WIDTH / 2), magRect.y + (MAG_HEIGHT / 5 * 2), magRect.x + (MAG_WIDTH / 2), magRect.y + (MAG_HEIGHT / 5 * 3));
    magRect.x = magRect.x + 2;
    magRect.y = magRect.y + 2;
    magRect.w = magRect.w - 4;
    magRect.h = magRect.h - 4;
    SDL_RenderDrawRect(renderer, &magRect);
    SDL_DestroyTexture(magTex);
}

// function that saves image after selecting
void saveImage()
{
    // load image surface
    SDL_Surface *surface = loadSurfaceFromScreen(selectPositions[0],
                                                 selectPositions[1],
                                                 selectPositions[2],
                                                 selectPositions[3]);
    // save image
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
    // create texture for displaying
    SDL_Texture *imageTex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_Delay(50);
    // set window property for displaying
    SDL_SetWindowOpacity(window, 1.0f);
    if(selectPositions[2] < (DM.w / 2) && selectPositions[3] < (DM.h / 2))
    {
        if(selectPositions[2] > (DM.h / 10) && selectPositions[3] > (DM.h / 10))
            SDL_SetWindowSize(window, selectPositions[2], selectPositions[3]);
        else
            SDL_SetWindowSize(window, selectPositions[2] * 4, selectPositions[3] * 4);
    }
    else
        SDL_SetWindowSize(window, selectPositions[2] * 2 / 3, selectPositions[3] * 2 / 3);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowBordered(window, SDL_TRUE);
    SDL_SetWindowTitle(window, "Hit any key to continue...");
    SDL_ShowWindow(window);
    // show image on screen
    SDL_RenderCopy(renderer, imageTex, NULL, NULL);
    SDL_RenderPresent(renderer);
    // keep displaying until any key is pressed
    SDL_Event e;
    while(e.type != SDL_KEYDOWN)
    {
        SDL_PollEvent(&e);
        SDL_Delay(50);
    }
    // restore window property
    SDL_DestroyTexture(imageTex);
    SDL_SetWindowSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetWindowTitle(window, WINDOW_TITLE);
}

// function that handle keyboard and mouse events
void pollEvents(unsigned char *quit, CurrentState *state, MouseState *mstate)
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
                // if s is pressed on DEFAULT state, change to SELECTING state
                    if(*state == DEFAULT)
                    {
                        SDL_SetWindowOpacity(window, 0.3f);
                        SDL_SetWindowBordered(window, SDL_FALSE);
                        SDL_SetWindowSize(window, DM.w, DM.h);
                        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                        *state = SELECTING;
                    }
                    break;
                case SDLK_r:
                // if r is pressed, change to DEFAULT state
                    if(*state != DEFAULT)
                    {
                        SDL_SetWindowOpacity(window, 1.0f);
                        SDL_SetWindowBordered(window, SDL_TRUE);
                        SDL_SetWindowSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
                        SDL_ShowWindow(window);
                        *state = DEFAULT;
                        *mstate = MOUSE_DEFAULT;
                    }
                    break;
                case SDLK_q:
                // if q is pressed (and not in DEFAULT state), change to SAVING state
                    if(*state != DEFAULT)
                    {
                        SDL_HideWindow(window);
                        *state = SAVING;
                        *mstate = MOUSE_DEFAULT;
                    }
                    break;
            }
        }
        // in SELECTING state, handle mouse down and up events
        if(*state == SELECTING)
        {
            if(e.type == SDL_MOUSEBUTTONDOWN)
            {
                if(*mstate == MOUSE_DEFAULT || *mstate == MOUSE_DONE)
                    *mstate = MOUSE_DOWN;
            }
            else if(e.type == SDL_MOUSEBUTTONUP)
            {
                *mstate = MOUSE_UP;
            }
        }
    }   
}

// function that load image from local storage
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

// function that update current mouse position
void updateMousePos()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    mousePosition[0] = x;
    mousePosition[1] = y;
}

// function that load surface from current screen
// This function is inspired by GarlandlX's answer on:
// https://www.gamedev.net/forums/topic/315178-hbitmap-to-sdl_surface/
SDL_Surface *loadSurfaceFromScreen(int x, int y, int w, int h)
{
    HDC dcScreen = GetDC(NULL);
    HDC dcTarget = CreateCompatibleDC(dcScreen);
    // create bitmap info
    BITMAPINFO bi;
    memset(&bi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    VOID *pvBits;
    HBITMAP bmpTarget = CreateDIBSection(dcScreen, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    SelectObject(dcTarget, bmpTarget);
    BitBlt(dcTarget, 0, 0, w, h, dcScreen, x, y, SRCCOPY);

    BITMAP bmp;
    GetObject(bmpTarget, sizeof(BITMAP), &bmp);

    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                             bmp.bmWidth,
                                             bmp.bmHeight,
                                             bmp.bmBitsPixel,
                                             0x000000FF,
                                             0x0000FF00,
                                             0x00FF0000,
                                             0xFF000000);
    Uint8 *bits = new Uint8[bmp.bmWidthBytes * bmp.bmHeight];
    memcpy(bits, bmp.bmBits, bmp.bmWidthBytes * bmp.bmHeight);

    // reverse BGRA to RGBA
    for (int i = 0; i < bmp.bmWidthBytes * bmp.bmHeight; i += 4) 
	{
		Uint8 tmp;
		tmp = bits[i];
		bits[i] = bits[i+2];
		bits[i+2] = tmp;
	}

    SDL_LockSurface(surf);
    memcpy(surf->pixels, bits, bmp.bmWidthBytes * bmp.bmHeight);
    SDL_UnlockSurface(surf);

    delete [] bits;

    SDL_Surface *surface = SDL_ConvertSurfaceFormat(surf, SDL_GetWindowPixelFormat(window), 0);
    SDL_FreeSurface(surf);
    DeleteDC(dcTarget);
    ReleaseDC(NULL, dcScreen);

    return surface;
}