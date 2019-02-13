#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <sstream>

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
std::vector<SDL_Texture *> images(2, nullptr);
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Surface *final_image = nullptr;
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
bool loadImage(const std::string cover, const std::string saving);
void drawMagnifier();
void drawSelectRect(MouseState *mstate);
void saveImage();
bool displayImage();
void updateMousePos();
SDL_Surface *loadSurfaceFromScreen(int x, int y, int w, int h);

int main(int argc, char **argv)
{
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::stringstream sstr;
        sstr << "Failed to init SDL!\nSDL_ERROR: " << SDL_GetError();
        std::string errorMessage = sstr.str();
        MessageBox(NULL,
                   (errorMessage.c_str()),
                   "SDL_Init Error",
                   MB_ICONERROR | MB_OK);
        return -1;
    }

    // Init image
    int imageFlags = IMG_INIT_PNG;
    if(!(!IMG_Init(imageFlags) ^ imageFlags))
    {
        std::stringstream sstr;
        sstr << "Failed to init IMG!\nIMG_ERROR: " << IMG_GetError();
        std::string errorMessage = sstr.str();
        MessageBox(NULL,
                   (errorMessage.c_str()),
                   "IMG_Init Error",
                   MB_ICONERROR | MB_OK);
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
        std::stringstream sstr;
        sstr << "Failed to create SDL_Window!\nSDL_ERROR: " << SDL_GetError();
        std::string errorMessage = sstr.str();
        MessageBox(NULL,
                   (errorMessage.c_str()),
                   "SDL_Window Error",
                   MB_ICONERROR | MB_OK);
        IMG_Quit();
        SDL_Quit();
        return -2;
    }

    // create window renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer)
    {
        std::stringstream sstr;
        sstr << "Failed to create SDL_Renderer!\nSDL_ERROR: " << SDL_GetError();
        std::string errorMessage = sstr.str();
        MessageBox(NULL,
                   (errorMessage.c_str()),
                   "SDL_Renderer Error",
                   MB_ICONERROR | MB_OK);
        IMG_Quit();
        SDL_Quit();
        return -3;
    }

    // load cover image
    if(!loadImage("cover.png", "save.png"))
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
        {
            SDL_RenderCopy(renderer, images[0], NULL, NULL);
            break;
        }
        case SAVING:
        {
            bool pass = displayImage();
            if(pass)
                saveImage();
            else
            {
                SDL_SetWindowTitle(window, WINDOW_TITLE);
                SDL_SetWindowOpacity(window, 0.3f);
                SDL_SetWindowBordered(window, SDL_FALSE);
                SDL_SetWindowSize(window, DM.w, DM.h);
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                *state = SELECTING;
                SDL_FreeSurface(final_image);
                final_image = nullptr;
                break;
            }
            // after saving, automatically change to DEFAULT state
            *state = DEFAULT;
            SDL_FreeSurface(final_image);
            final_image = nullptr;
            break;
        }
        case SELECTING:
        {
            updateMousePos();
            drawSelectRect(mstate);
            drawMagnifier();
            break;
        }
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
// this function is transformed from Microsoft official documents
// https://docs.microsoft.com/en-us/windows/desktop/dlgbox/using-common-dialog-boxes
void saveImage()
{
    OPENFILENAME ofn;       // common dialog box structure
    char szFile[256];       // buffer for file name

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrDefExt = "png";
    ofn.lpstrFilter = "All\0*.*\0PNG(.png)\0*.png\0JPG(.jpg)\0*.jpg\0BitMap(.bmp)\0*.bmp\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |OFN_OVERWRITEPROMPT;

    // Display the Save dialog box. 
    if(GetSaveFileName(&ofn))
    {
        const char *filename = (char*)ofn.lpstrFile;
        SDL_SaveBMP(final_image, filename);
    }
}

// function that displays the selected image
bool displayImage()
{
    if(!final_image)
    {
        SDL_FreeSurface(final_image);
    }
    // load image surface
    final_image = loadSurfaceFromScreen(selectPositions[0],
                                        selectPositions[1],
                                        selectPositions[2],
                                        selectPositions[3]);
    SDL_Texture *imgTex = SDL_CreateTextureFromSurface(renderer, final_image);
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
    SDL_SetWindowTitle(window, "Hit R to reselect...");
    SDL_ShowWindow(window);
    // show image on screen
    SDL_RenderCopy(renderer, imgTex, NULL, NULL);
    SDL_RenderPresent(renderer);
    // keep displaying until any key is pressed
    SDL_Event e;
    while(e.type != SDL_KEYDOWN)
    {
        SDL_PollEvent(&e);
        if(e.key.keysym.sym == SDLK_r || e.key.keysym.sym == SDLK_ESCAPE)
            return false;
        SDL_Delay(50);
    }
    SDL_DestroyTexture(imgTex);
    // restore window property
    SDL_SetWindowSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowTitle(window, WINDOW_TITLE);
    // display the saving image
    SDL_RenderCopy(renderer, images[1], NULL, NULL);
    SDL_RenderPresent(renderer);
    return true;
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
                    if(*state != DEFAULT && *mstate == MOUSE_DONE)
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
bool loadImage(const std::string cover, const std::string saving)
{
    SDL_Surface *loadIMG1 = IMG_Load(("./Images/" + cover).c_str());
    SDL_Surface *loadIMG2 = IMG_Load(("./Images/" + saving).c_str());
    if(!loadIMG1 || !loadIMG2)
    {
        std::stringstream sstr;
        sstr << "Failed to load images!\nIMG_ERROR: " << IMG_GetError();
        std::string errorMessage = sstr.str();
        MessageBox(NULL,
                   (errorMessage.c_str()),
                   "Load Image Error",
                   MB_ICONERROR | MB_OK);
        return false;
    }
    images[0] = SDL_CreateTextureFromSurface(renderer, loadIMG1);
    images[1] = SDL_CreateTextureFromSurface(renderer, loadIMG2);
    SDL_FreeSurface(loadIMG1);
    SDL_FreeSurface(loadIMG2);
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
    DeleteDC(dcScreen);
    DeleteObject(bmpTarget);

    return surface;
}