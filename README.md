## ScreenShot  
This is an windows application built with SDL2 which can capture specific screen and save it as bmp file  

------

**Dependencies**  
1. SDL2  
2. SDL2_image  
3. gdi32 (windows specific)  

------

**How to compile**  
1. First compile icon and obj file  
```bash
windres myIcon.rc -O coff -o myIcon.res
g++ -O2 -std=c++11 -c main.cpp -lgdi32 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mconsole
```
2. Next compile them into one program  
```bash
g++ -O2 -std=c++11 -o run.exe main.o myIcon.res -lgdi32 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mconsole
```

------

**Additional Infomation**  
In plain SDL2, it's impossible to capture the system screen. Therefore, I only use SDL2 as a window application tool. For the part of screen capture, I create `MIPMAP` obj from the current screen context, then transformed it into `SDL_Surface` object type. At last, I use the built in function `SDL_SaveBMP` to save the data as bmp file.  
Taking a screenshot is quite system specific, and quite complicated. There's more to learn