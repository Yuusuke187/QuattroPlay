#include "string.h"
#include "math.h"
#include "../qp.h"

#include "ui.h"

#define PLPAGE (FROWS-7)
    int displaypos;

void displaypos_check()
{
    if(displaypos < 0)
        displaypos = 0;
    if(displaypos >= Game->SongCount)
        displaypos = Game->SongCount-1;
}
void scr_playlist_input()
{
    SDL_LockAudioDevice(Audio->dev);

    got_input=0;

    int increment=1;

    switch(keycode)
    {
    case SDLK_UP:
    case SDLK_PAGEUP:
        increment=-increment;
    case SDLK_DOWN:
    case SDLK_PAGEDOWN:
        if(keycode == SDLK_PAGEUP || keycode == SDLK_PAGEDOWN)
            increment *= PLPAGE;
        displaypos  += increment;
        displaypos_check();
        break;
    case SDLK_RETURN:
        displaypos_check();
        Game->PlaylistPosition=displaypos;
    case SDLK_r:
        Game->PlaylistControl=2;
        break;
    case SDLK_f:
    case SDLK_s:
        Game->PlaylistControl = 0;
        int SongReq = Game->PlaylistSongID & 0x800 ? 8 : 0;
        if(keycode==SDLK_f)
            QDrv->SongRequest[SongReq] |= Q_TRACK_STATUS_FADE;
        if(keycode==SDLK_s)
            QDrv->SongRequest[SongReq] &= ~(Q_TRACK_STATUS_BUSY);
        break;
    case SDLK_n:
        displaypos = Game->PlaylistPosition+1;
        displaypos_check();
        Game->PlaylistPosition=displaypos;
        Game->PlaylistControl=2;
        break;
    case SDLK_b:
        displaypos = Game->PlaylistPosition-1;
        displaypos_check();
        Game->PlaylistPosition=displaypos;
        Game->PlaylistControl=2;
        break;
    }

    SDL_UnlockAudioDevice(Audio->dev);
}


void scr_playlist()
{
    if(refresh & R_SCR_PLAYLIST)
    {
        refresh &= ~R_SCR_PLAYLIST;
        displaypos=0;
    }

    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);
    set_color(5,1,FROWS-7,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
    set_color(49,0,1,FCOLUMNS,COLOR_D_BLUE,COLOR_L_GREY);
    SCRN(1,1,FCOLUMNS-2,"%s",QDrv->SongMessage);

    int SongReq = Game->PlaylistSongID & 0x800 ? 8 : 0;

    switch(Game->PlaylistControl)
    {
    case 0:
        SCRN(3,1,FCOLUMNS-2,"Playlist not currently active");
    default:
        break;
    case 1:
    case 2:
        SCRN(3,1,FCOLUMNS-6,"Playing song %02d: %s",
             Game->PlaylistPosition+1,
             Game->Playlist[Game->PlaylistPosition].Title);

        SCRN(3,FCOLUMNS-6,6,"%2.0f:%02.0f",
             floor(QDrv->SongTimer[SongReq]/60),floor(fmod(QDrv->SongTimer[SongReq],60)));
        break;
    }


    if(Game->SongCount == 0)
    {
        SCRN(5,1,FCOLUMNS-2,"Playlist not defined.");
    }
    else
    {
        if(got_input)
            scr_playlist_input();

        int y = 0;
        int i = displaypos;
        int offset = 0;
        int max = PLPAGE;
        if(Game->SongCount < max)
            max = Game->SongCount;
        if(i>(PLPAGE/2) && Game->SongCount > max)
            offset = i-(PLPAGE/2);
        if(offset+max > Game->SongCount)
            offset -= (offset+max)-Game->SongCount;

        for(y=0;y<max;y++)
        {
            i=y+offset;
            colorsel_t bg=COLOR_D_BLUE,fg=COLOR_L_GREY;

            if(displaypos == i)
                bg = COLOR_N_BLUE;
            if(Game->PlaylistPosition == i)
                fg = COLOR_WHITE;

            set_color(5+y,1,1,FCOLUMNS-2,bg,fg);

            SCRN(5+y,1,FCOLUMNS-2,"%02d %s",i+1,Game->Playlist[i].Title);
        }

        SCRN(49,1,48,"ENTER: play song, S: stop, F: fade, P: pause");
    }
}
