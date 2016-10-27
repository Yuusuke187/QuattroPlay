#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

// Setting to 60 will break debugging ...
#define UI_FPS 30
// Amount of frames to sample for FPS counting
#define UI_FPS_SAMPLES 16

//#define FSIZE_X 8
//#define FSIZE_Y 8
#define FCOLUMNS 80
#define FROWS 50
    int FSIZE_X;
    int FSIZE_Y;


#include "stdint.h"
#include "drv/quattro.h"
#include "audio.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_render.h"

#define SCR(_y_,_x_,str,...) sprintf(&screen.text[_y_][_x_],str , ##__VA_ARGS__)
#define SCRN(_y_,_x_,count,str,...) snprintf(&screen.text[_y_][_x_],count,str , ##__VA_ARGS__)

    SDL_Window *window;
    SDL_Renderer *rend;
    SDL_Texture *font;
    SDL_Texture *dispbuf;

typedef enum {
    COLOR_BLACK = 0,
    COLOR_WHITE,
    COLOR_D_GREY,
    COLOR_N_GREY,
    COLOR_L_GREY,
    COLOR_D_RED,
    COLOR_N_RED,
    COLOR_L_RED,
    COLOR_D_GREEN,
    COLOR_N_GREEN,
    COLOR_L_GREEN,
    COLOR_D_BLUE,
    COLOR_N_BLUE,
    COLOR_L_BLUE,

    CFLAG_YSHIFT_25 = 0x100,
    CFLAG_YSHIFT_50 = 0x200,
    CFLAG_YSHIFT_75 = 0x400,
    CFLAG_YSHIFT    = 0x700

} colorsel_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

typedef struct {
    char text[FROWS][FCOLUMNS*2];
    colorsel_t textcolor[FROWS][FCOLUMNS*2];
    colorsel_t bgcolor[FROWS][FCOLUMNS*2];
    uint8_t dirty[FROWS][FCOLUMNS*2];
} screen_t;

    //int bootsong;
    //QP_PlaylistState *pl;

    //int muterear;
    //float gain;
    //float gamegain;
    float vol;
    screen_t screen;
    screen_t last_screen;

    int trackpattern[32][8];
    int trackpattern_length;

    uint32_t lasttick;
    uint32_t frame_cnt;
    double fps_cnt;

    int debug_stat;
    int draw_count;
    int rect_count;

void redraw_text();
void ui_main();

void set_color(int y,int x,int h,int w,colorsel_t bg,colorsel_t fg);

#endif // UI_H_INCLUDED