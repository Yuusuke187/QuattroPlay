/*
    VGM writing
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <wchar.h>
#include <time.h>

#include "vgm.h"
#include "fileio.h"

// has to be larger than ~20MB
#define VGM_BUFFER 50000000

    uint32_t buffer_size;
    uint32_t delayq;
    uint32_t samplecnt;
    uint32_t loop_set;
    uint8_t* vgmdata;
    uint8_t* data;
    char* filename;

// Increments destination pointer
void my_memcpy(uint8_t** dest, void* src, int size)
{
    memcpy(*dest,src,size);
    *dest += size;
}

void add_datablockcmd(uint8_t** dest, uint8_t dtype, uint32_t size, uint32_t romsize, uint32_t offset)
{
    **dest = 0x67;*dest+=1;
    **dest = 0x66;*dest+=1;
    **dest = dtype;*dest+=1;
    size += 8;
    my_memcpy(dest,&size,4);
    my_memcpy(dest,&romsize,4);
    my_memcpy(dest,&offset,4);
}

void add_delay(uint8_t** dest, int delay)
{
    samplecnt += delay;

    int commandcount = floor(delay/65535);
    uint16_t finalcommand = delay%65535;

    while(commandcount)
    {
        **dest = 0x61;*dest+=1;
        **dest = 0xff;*dest+=1;
        **dest = 0xff;*dest+=1;
        commandcount--;
    }

    if(finalcommand > 16)
    {
        **dest = 0x61;*dest+=1;
        my_memcpy(dest,&finalcommand,2);
    }
    else if(finalcommand > 0)
    {
        **dest = 0x70 + finalcommand-1;
        *dest+=1;
    }
}

void vgm_open(char* fname)
{
    filename = (char*)malloc(strlen(fname)+10);
    strcpy(filename,fname);
    delayq=0;
    loop_set=0;

    // create initial buffer
    vgmdata=(uint8_t*)malloc(VGM_BUFFER);
    data = vgmdata;
    buffer_size = VGM_BUFFER;
    memset(data, 0, VGM_BUFFER);

    // vgm magic
    memcpy(data, "Vgm ", 4);

    // version
    data+=8;
    *data++ = 0x71;
    *data++ = 0x01;

    //data offset
    *(uint32_t*)(vgmdata+0x34)=0x100-0x34;

    data=vgmdata+0x100;
}

void vgm_poke32(int32_t offset, uint32_t d)
{
    *(uint32_t*)(vgmdata+offset)= d;
}

void vgm_poke8(int32_t offset, uint8_t d)
{
    *(uint8_t*)(vgmdata+offset)= d;
}

// notice: start offset was replaced with ROM mask.
void vgm_datablock(uint8_t dbtype, uint32_t dbsize, uint8_t* datablock, uint32_t maxsize, uint32_t mask, int32_t flags)
{
    add_datablockcmd(&data, dbtype, dbsize|flags, maxsize, 0);

    int i;
    for(i=0;i<dbsize;i++)
        *data++ = datablock[i & mask];

    //my_memcpy(&data, datablock, dbsize);
}

void vgm_setloop()
{
    // add delays
    if(delayq/10 > 1)
    {
        add_delay(&data,delayq/10);
        delayq=delayq%10;
    }

    loop_set = samplecnt;
    *(uint32_t*)(vgmdata+0x1c)= data-vgmdata-0x1c;
}

void vgm_write(uint8_t command, uint8_t port, uint16_t reg, uint16_t value)
{
    if(delayq/10 > 1)
    {
        add_delay(&data,delayq/10);
        delayq=delayq%10;
    }

// todo: need to handle command types if using other chips
    *data++ = command;

    if(command == 0xe1) // C352
    {
        *data++ = reg>>8;
        *data++ = reg&0xff;
        *data++ = value>>8;
        *data++ = value&0xff;
    }
    else if(command == 0x54) // YM2151
    {
        *data++ = reg;
        *data++ = value;
    }
    else // following is for D0-D6 commands...
    {
        *data++ = port;
        *data++ = (reg&0xff);
        *data++ = (value&0xff);
    }

    // resize buffer if needed
    if(buffer_size-(data-vgmdata) < 1000000)
    {
        uint8_t* temp;
        temp = realloc(vgmdata,buffer_size*2);
        if(temp)
        {
            buffer_size *= 2;
            data = temp+(data-vgmdata);
            vgmdata = temp;
        }
    }
}

// delay is in VGM samples*10.
void vgm_delay(uint32_t delay)
{
    delayq+=delay;
}

// https://github.com/cppformat/cppformat/pull/130/files
void gd3_write_string(char* s)
{
    size_t l;
    #if defined(_WIN32) && defined(__MINGW32__) && !defined(__NO_ISOCEXT)
        l = _snwprintf((wchar_t*)data,256,L"%S", s);
    #else
        l = swprintf((wchar_t*)data,256,L"%s", s);
    #endif // defined

    data += (l+1)*2;
}

void vgm_write_tag(char* gamename,int songid)
{
    time_t t;
    struct tm * tm;
    time(&t);
    tm = localtime(&t);
    char ts [32];
    strftime(ts,32,"%Y-%m-%d %H:%M:%S",tm);
    char tracknotes[256];
    tracknotes[0] = 0;
    if(songid >= 0)
        sprintf(tracknotes,"Song ID: %03x\n",songid&0x7ff);
    strcpy(tracknotes+strlen(tracknotes),"Generated using QuattroPlay by ctr (Built "__DATE__" "__TIME__")");

    // Tag offset
    *(uint32_t*)(vgmdata+0x14)= data-vgmdata-0x14;

    memcpy(data, "Gd3 \x00\x01\x00\x00" , 8);
    uint8_t* len_s = data+8;
    data+=12;

    gd3_write_string(""); // Track name
    gd3_write_string(""); // Track name (native)
    gd3_write_string(gamename); // Game name
    gd3_write_string(""); // Game name (native)
    gd3_write_string("Arcade Machine"); // System name
    gd3_write_string(""); // System name (native)
    gd3_write_string(""); // Author name
    gd3_write_string(""); // Author name (native)
    gd3_write_string(ts); // Time
    gd3_write_string(""); // Pack author
    gd3_write_string(tracknotes); // Notes

    *(uint32_t*)(len_s) = data-len_s-4;        // length
}

void vgm_stop()
{
    if(delayq/10 > 1)
    {
        add_delay(&data,delayq/10);
        delayq=0;
    }
    *data++ = 0x66;

    // Sample count/loop sample count
    *(uint32_t*)(vgmdata+0x18)= samplecnt;
    if(loop_set)
        *(uint32_t*)(vgmdata+0x20)= samplecnt-loop_set;
}

void vgm_close()
{
    // EoF offset
    *(uint32_t*)(vgmdata+0x04)= data-vgmdata-4;

    write_file(filename, vgmdata, data-vgmdata);

    free(vgmdata);
}
