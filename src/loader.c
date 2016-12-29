/*
    File loader and driver initialization
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "SDL2/SDL.h"
#include "qp.h"
// #include "ui.h"
#include "vgm.h"
#include "ini.h"
#include "fileio.h"




// Loads game ini, then the sound data and wave roms...
int LoadGame(game_t *G)
{
    //Q_State* Q = G->QDrv;

    static char msgstring[2048];

    static char filename[128];
    static char path[128];
    //static char gamehackname[128];
    static char wave0[16];
    static char wave1[16];

    int byteswap = 0;
    int wave_count = 0;
    int wave_pos[16];
    int wave_length[16];
    int wave_offset[16];
    unsigned int wave_maxlen; // max length of wave roms.
    G->ChipFreq = 0;
    G->SongCount = 0;

    int patchtype_set = 0;
    int patchaddr_set = 0;
    int patchdata_set = 0;
    int patchcount = 0;

    unsigned int action_id = 0;
    unsigned int action_reg = 0;
    unsigned int action_data = 0;

    int patchtype[64];
    int patchaddr[64];
    int patchdata[64];

    static char wave_filename[16][128];
    static char data_filename[128];
    static char driver_name[128];

    msgstring[0] = 0;
    path[0] = 0;

    sprintf(msgstring,"Failed to load '%s':",G->Name);
    int loadok = strlen(msgstring);

    snprintf(filename,127,"%s/%s.ini",QP_IniPath,G->Name);

#ifdef DEBUG
    printf("Now loading '%s' ...\n",filename);
#endif

    strcpy(G->Title,G->Name);
    memset(wave_pos,0,sizeof(wave_pos));
    memset(wave_length,0,sizeof(wave_length));
    memset(wave_offset,0,sizeof(wave_offset));
    memset(wave_filename,0,sizeof(wave_filename));
    memset(G->Action,0,sizeof(G->Action));
    memset(G->Type,0,sizeof(G->Type));
    memset(driver_name,0,sizeof(driver_name));

    inifile_t initest;
    if(!ini_open(filename,&initest))
    {
        while(!ini_readnext(&initest))
        {
            //printf("'%s'.'%s' = '%s'\n",initest.section,initest.key,initest.value);
            snprintf(wave0,15,"wave.%d",wave_count);
            snprintf(wave1,15,"wave.%d",wave_count+1);

            // this will be updated with more options as needed.
            if(!strcmp(initest.section,"data"))
            {
                if(!strcmp(initest.key,"name"))
                    strcpy(G->Title,initest.value);
                else if(!strcmp(initest.key,"path"))
                    strcpy(path,initest.value);
                else if(!strcmp(initest.key,"filename"))
                    strcpy(data_filename,initest.value);
                else if(!strcmp(initest.key,"driver"))
                    strcpy(driver_name,initest.value);
                else if(!strcmp(initest.key,"type"))
                    strcpy(G->Type,initest.value);
                else if(!strcmp(initest.key,"byteswap"))
                    byteswap = atoi(initest.value) & 1;
                else if(!strcmp(initest.key,"gain"))
                    G->Gain = atof(initest.value);
                else if(!strcmp(initest.key,"muterear"))
                    G->MuteRear = atoi(initest.value);
                else if(!strcmp(initest.key,"chipfreq"))
                    G->ChipFreq = atoi(initest.value);
//                else if(!strcmp(initest.key,"gamehack"))
//                    strcpy(gamehackname,initest.value);
            }

            if(!strcmp(initest.section,"patch"))
            {
                patchtype_set=0;
                patchdata_set = strtol(initest.value,NULL,0);

                if(!strcmp(initest.key,"address"))
                    patchaddr_set = patchdata_set;
                else if(!strcmp(initest.key,"song"))
                    patchaddr_set = patchdata_set*3;
                else if(!strcmp(initest.key,"byte"))
                    patchtype_set = 1;
                else if(!strcmp(initest.key,"word"))
                    patchtype_set = 2;
                else if(!strcmp(initest.key,"pos"))
                    patchtype_set = 3;

                if(patchtype_set)
                {
                    patchtype[patchcount] = patchtype_set;
                    patchaddr[patchcount] = patchaddr_set;
                    patchdata[patchcount] = patchdata_set;
                    patchaddr_set+=patchtype_set;
                    patchcount++;
                }
            }

            // at the next wave rom section?
            if(!strcmp(initest.section,wave1))
            {
                if(wave_count < 16)
                    wave_count++;

                snprintf(wave0,15,"wave.%d",wave_count);
            }
            if(!strcmp(initest.section,wave0))
            {
                if(!strcmp(initest.key,"filename"))
                    strcpy(wave_filename[wave_count],initest.value);
                else if(!strcmp(initest.key,"length"))
                    wave_length[wave_count] = strtol(initest.value,NULL,0);
                else if(!strcmp(initest.key,"position"))
                    wave_pos[wave_count] = strtol(initest.value,NULL,0);
                else if(!strcmp(initest.key,"offset"))
                    wave_offset[wave_count] = strtol(initest.value,NULL,0);
            }
            if(!strcmp(initest.section,"playlist"))
            {
                if(!strcmp(initest.key,"loops"))
                {
                    G->Playlist[G->SongCount-1].script[action_id].wait_type=0;
                    G->Playlist[G->SongCount-1].script[action_id].wait_count=strtol(initest.value,NULL,0);
                }
                else if(!strcmp(initest.key,"time"))
                {
                    G->Playlist[G->SongCount-1].script[action_id].wait_type=1;
                    G->Playlist[G->SongCount-1].script[action_id].wait_count=strtol(initest.value,NULL,0);
                }
                else if(!strcmp(initest.key,"action"))
                {
                    G->Playlist[G->SongCount-1].script[action_id].action_id=strtol(initest.value,NULL,0);
                    action_id++;
                    G->Playlist[G->SongCount-1].script[action_id].action_id = -1;
                    G->Playlist[G->SongCount-1].script[action_id].wait_type = 1; // end immediately...
                }
                else if(!strcmp(initest.key,"loop"))
                {
                    G->Playlist[G->SongCount-1].script[action_id].wait_type=2;
                    G->Playlist[G->SongCount-1].script[action_id].wait_count=strtol(initest.value,NULL,0);
                }
                else if(sscanf(initest.key,"%x",&action_reg)==1)
                {
                    action_id=0;
                    G->Playlist[G->SongCount].SongID = action_reg;
                    strncpy(G->Playlist[G->SongCount].Title,initest.value,254);
                    G->Playlist[G->SongCount].script[action_id].wait_type=0;
                    G->Playlist[G->SongCount].script[action_id].wait_count=2;
                    G->Playlist[G->SongCount].script[action_id].action_id=-1;
                    G->SongCount++;
                }
                Q_DEBUG("playlist %s = %s\n",initest.key,initest.value);
            }
            if(sscanf(initest.section,"action.%d",&action_id)==1 && action_id < 256)
            {
                if(sscanf(initest.key,"r%x",&action_reg)==1)
                {
                    action_data = strtol(initest.value,NULL,0);
                    G->Action[action_id].reg[G->Action[action_id].cnt] = action_reg;
                    G->Action[action_id].data[G->Action[action_id].cnt] = action_data;
                    Q_DEBUG("action %d (%02x) =  r%02x = %04x\n",action_id,G->Action[action_id].cnt,action_reg,action_data);
                    G->Action[action_id].cnt++;
                }
                else if(sscanf(initest.key,"t%x",&action_reg)==1)
                {
                    action_data = strtol(initest.value,NULL,0);
                    G->Action[action_id].reg[G->Action[action_id].cnt] = action_reg+0x100;
                    G->Action[action_id].data[G->Action[action_id].cnt] = action_data;
                    Q_DEBUG("action %d (%02x) =  t%02x = %04x\n",action_id,G->Action[action_id].cnt,action_reg,action_data);
                    G->Action[action_id].cnt++;
                }
            }
        }
    }

    if(initest.status)
    {
        if(initest.status == INI_FILE_LOAD_ERROR)
            strcat(msgstring,my_strerror(filename));
        else
            strcat(msgstring,ini_error[initest.status]);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
        ini_close(&initest);
        return -1;
    }

    ini_close(&initest);

    if(strlen(path) == 0)
        strcpy(path,G->Name);

    G->WaveMask=0;
    G->DataSize = 0x80000;
    G->Data = (uint8_t*)malloc(G->DataSize);

#ifdef DEBUG
    printf("Game title: '%s'\n",G->Title);
    printf("Data filename: '%s'\n",data_filename);
    printf("Wave count: %d\n",wave_count+1);
    if(byteswap)
        printf("Data file byteswapped\n");
    printf("Playlist Song count: %d\n",G->SongCount);
#endif
    snprintf(filename,127,"%s/%s/%s",QP_DataPath,path,data_filename);
    if(read_file(filename,G->Data,0,0,byteswap,&G->DataSize))
    {
        strcat(msgstring,my_strerror(filename));
        //return -1;
    }

    int i;
    for(i=0;i<patchcount;i++)
    {
        printf("Patch type %d addr %06x data %06x\n",patchtype[i],patchaddr[i],patchdata[i]);

        if(patchaddr[i]+patchtype[i] > G->DataSize)
            printf("patch address out of bounds\n");
        else if(patchtype[i] == 1) // byte
            *(uint8_t*)(G->Data+patchaddr[i]) = patchdata[i];
        else if(patchtype[i] == 3) // song table
        {
            patchaddr_set = patchaddr[i];
            if(!strcmp(G->Type,"H8") || !strcmp(G->Type,"H8_ND")) // for most cases....
            {
                if(patchaddr_set < 0x1800)
                    patchaddr_set += 0x800e;
                patchdata_set = patchdata[i];
            }
            else
            {
                if(patchaddr_set < 0x1800)
                    patchaddr_set += 0x1000e;
                patchdata_set = patchdata[i] + 0x200000;
            }

            *(uint16_t*)(G->Data+patchaddr_set) = patchdata_set&0xffff;
            *(uint8_t*)(G->Data+patchaddr_set+2) = patchdata_set>>16;
        }
        else //if (patchtype[i] == 2) // word
            *(uint16_t*)(G->Data+patchaddr[i]) = patchdata[i];
    }

    G->WaveData = (uint8_t*)malloc(0x1000000);
    memset(G->WaveData,0,0x1000000);
    for(i=0;i<wave_count+1;i++)
    {
#ifdef DEBUG
        printf("Wave %d\n",i);
        printf("\tFilename: '%s'\n",wave_filename[i]);
        printf("\tPosition: %06x\n",wave_pos[i]);
        printf("\tLength: %06x\n",wave_length[i]);
        printf("\tOffset: %06x\n",wave_offset[i]);
#endif
        wave_maxlen = 0x1000000 - wave_pos[i];
        snprintf(filename,127,"%s/%s/%s",QP_WavePath,path,wave_filename[i]);
        if(read_file(filename,G->WaveData+wave_pos[i],wave_length[i],wave_offset[i],0,&wave_maxlen))
            strcat(msgstring,my_strerror(filename));
        else
            G->WaveMask |= wave_pos[i]+wave_length[i]-1;
    }

#ifdef DEBUG
    printf("Wave Mask = %06x\n",G->WaveMask);
#endif

    if(loadok != strlen(msgstring))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
        return -1;
    }

    DriverInterface = (struct _DriverInterface*)malloc(sizeof(struct _DriverInterface));
    memset(DriverInterface,0,sizeof(struct _DriverInterface));

    for(i=0;driver_name[i];i++)
        driver_name[i] = tolower(driver_name[i]);

    for(i=0;i<DRIVER_COUNT;i++)
    {
        if(!strcmp(driver_name,DriverTable[i].name))
        {
            printf("loading driver: %s\n",DriverTable[i].name);
            if(DriverCreate(DriverInterface,i))
                break;
            QDrv = NULL;
            if(i == DRIVER_QUATTRO)
                QDrv = DriverInterface->Driver.quattro;
            return 0;
        }
    }
    if(i==DRIVER_COUNT)
        sprintf(msgstring,"%s Unable to find matching driver type for \"%s\"",msgstring,driver_name);
    else
        sprintf(msgstring,"%s Failed to create driver \"%s\"",msgstring,driver_name);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
    return -1;
}

int UnloadGame(game_t *G)
{
    free(G->Data);
    free(G->WaveData);
    //free(Q_Chip);
    QDrv = NULL;
    DriverDestroy(DriverInterface);
    return 0;
}

int InitGame(game_t *Game)
{
    if(DriverInit())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error","Failed to initialize driver",NULL);
        return -1;
    }
    // Initialize sound chip and some initial sound driver parameters.

    Game->PlaylistPosition = 0;
    Game->PlaylistLoop = 0;
    Game->PlaylistControl = 0;
    Game->PlaylistSongID = 0;
    Game->ActionTimer = 0;

    static char filename[FILENAME_MAX];

    char* audiodev = NULL;
    if(strlen(Game->AudioDevice))
        strcpy(audiodev,Game->AudioDevice);

    if(Game->VgmLog)
    {
        strcpy(filename,"qp_log.vgm");
        if(Game->AutoPlay >= 0)
        {
            sprintf(filename,"%s_%03x.vgm",Game->Name,Game->AutoPlay&0x7ff);
        }
        vgm_open(filename);

        DriverInitVgm();
    }

    Game->QueueSong=Game->AutoPlay;

    DriverReset(1);

    QPAudio_Init(Audio,QDrv,QDrv->Chip.rate,Game->AudioBuffer,audiodev);

    //if(Game->AutoPlay >= 0)
    //    QDrv->BootSong=2;

    Audio->state.AutoPlaySong = Game->AutoPlay;
    Audio->state.MuteRear = Game->MuteRear;
    Audio->state.Gain = Game->BaseGain*Game->Gain;

    if(Game->WavLog)
    {
        strcpy(filename,"qp_log.wav");
        if(Game->AutoPlay >= 0)
        {
            sprintf(filename,"%s_%03x.wav",Game->Name,Game->AutoPlay&0x7ff);
        }
        QPAudio_WavOpen(Audio,filename);
    }

    return 0;
}

void DeInitGame(game_t *Game)
{
    if(Audio->state.FileLogging)
    {
        SDL_LockAudioDevice(Audio->dev);
        QPAudio_WavClose(Audio);
        SDL_UnlockAudioDevice(Audio->dev);
    }

    if(Game->VgmLog)
    {
        SDL_LockAudioDevice(Audio->dev);
        DriverCloseVgm();
        vgm_stop();
        vgm_write_tag(strlen(Game->Title) ? Game->Title : Game->Name,Game->AutoPlay);
        vgm_close();
        SDL_UnlockAudioDevice(Audio->dev);
    }

    DriverDeinit();
}

void ResetGame(game_t *Game)
{
    DriverReset(0);
    Game->PlaylistControl=0;
    Game->QueueSong=Game->AutoPlay;
}

// Perform register action (song triggers).
void GameDoAction(game_t *G,unsigned int id)
{
    if(id > 255)
        return;
    int i,reg;
    for(i=0;i<G->Action[id].cnt;i++)
    {
        reg = G->Action[id].reg[i];
        if(reg<0x100)
            DriverSetParameter(G->Action[id].reg[i],G->Action[id].data[i]);
        //G->QDrv->Register[G->Action[id].reg[i]&0xff] = G->Action[id].data[i];
        else if(reg<0x120)
            G->QDrv->SongRequest[G->Action[id].reg[i]&0x1f] = G->Action[id].data[i];
    }
    return;
}

void GameDoUpdate(game_t *G)
{
    int i;

    if(QDrv->BootSong != 0)
        return;

    if(G->PlaylistControl == 1)
    {
        playlist_script_t* S = &G->Playlist[G->PlaylistPosition].script[G->PlaylistScript];

        int state = 0;
        int SongReq = G->PlaylistSongID & 0x800 ? 8 : 0;

        int loopcnt = DriverGetLoopCount(SongReq);

        // time out
        if(S->wait_type == 2)
        {
            G->PlaylistScript = S->wait_count;
            if(++G->PlaylistLoop > 1)
                state = 1;
        }
        if(S->wait_type == 1 && DriverGetPlayingTime(SongReq) > S->wait_count)
            state=1;
        if(S->wait_type == 0 && loopcnt >= S->wait_count)
            state=1;

        // song is stopped
        //if((QDrv->SongRequest[SongReq]&0x8000) == 0)
        if(~DriverGetSongStatus(SongReq)&SONG_STATUS_PLAYING)
            state=2;

        switch(state)
        {
        default:
            break;
        case 1:
            // do action and read next length
            if(S->action_id >= 0)
            {
                // do the previous action immediately...
                if(G->ActionTimer)
                    GameDoAction(G,G->QueueAction);
                Q_DEBUG("doing action %d...\n",S->action_id);
                G->ActionTimer=20; // some songs don't like when actions are triggered immediately after loop...
                G->QueueAction=S->action_id;
                G->PlaylistScript++;
            }
            // or fade song
            else
            {
                G->PlaylistLoop=60;
                //G->QDrv->SongRequest[SongReq]|=0x2000;
                DriverFadeOutSong(SongReq);
            }
            DriverResetLoopCount();
            break;
        case 2:
            G->PlaylistLoop++;
            if(G->PlaylistLoop<2)
            {
                // if all voices are silent, advance immediately.
                // otherwise, we will wait half a second before advancing
                if(DriverDetectSilence())
                    break;
            }
            else if(G->PlaylistLoop<60)
                break;

            // advance playlist
            if(G->PlaylistPosition < G->SongCount-1)
            {
                G->PlaylistControl++;
                G->PlaylistPosition++;
            }
            // or stop playlist
            else
                G->PlaylistControl=0;
            break;
        }
    }

    if(G->PlaylistControl == 2)
    {
        for(i=0;i<DriverGetSlotCount();i++)
            DriverStopSong(i);

        G->QueueSong = G->Playlist[G->PlaylistPosition].SongID;
        G->PlaylistSongID = G->Playlist[G->PlaylistPosition].SongID;
        G->PlaylistControl--;
        G->PlaylistScript = 0;
        G->PlaylistLoop = 0;
        G->ActionTimer = 0;
    }

    if(G->QueueSong >= 0)
    {
        DriverResetLoopCount();
        DriverRequestSong(G->QueueSong & 0x800 ? 8 : 0, G->QueueSong&0x7ff);
        //Q_LoopDetectionReset(G->QDrv);
        //G->QDrv->SongRequest[G->QueueSong & 0x800 ? 8 : 0] = 0x4000 | (G->QueueSong&0x7ff);
    }

    G->QueueSong = -1;

    if(G->ActionTimer && --G->ActionTimer == 0)
        GameDoAction(G,G->QueueAction);
}
