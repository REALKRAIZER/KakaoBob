// play_wav.c

#include "play_wav.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*1. Установить SDL через sudo apt install libsdl2-dev.

2. Скомпилировать код моего примера и библиотеки через gcc -o play_wav_usage_example play_wav_usage_example.c play_wav.c -lSDL2.

3. Запустить ./play_wav_usage_example, он требует рядом наличия файла sound.wav для проигрывания.*/
enum PlayWavErrors {
    ERR_PLAY_WAV_SDL_FAILED = 0xC8B30001,
    ERR_PLAY_WAV_PLAY_WAV_RW_FAILED,
};

#define MEMZERO(obj) memset(&(obj), 0, sizeof(obj))

typedef uint32_t Error; 

struct ErrInfo {
    uint32_t line;
    Error err;
    char const * serr;
    char const * func;
    char const * code;
    char const * file;
    char msg[256];
};

enum { c_max_errors_cnt = 128 };

static struct ErrInfo g_errors[c_max_errors_cnt];
static uint32_t g_errors_cnt = 0;

static void ClearErrors() {
    MEMZERO(g_errors);
    g_errors_cnt = 0;
}

#define ERRIFMSG(cond, err_, msg_) \
    if (cond) { \
        if (g_errors_cnt < c_max_errors_cnt) { \
            g_errors[g_errors_cnt].err = (err_); \
            g_errors[g_errors_cnt].line = __LINE__; \
            g_errors[g_errors_cnt].func = __FUNCTION__; \
            g_errors[g_errors_cnt].file = __FILE__; \
            g_errors[g_errors_cnt].code = #cond; \
            g_errors[g_errors_cnt].serr = #err_; \
            MEMZERO(g_errors[g_errors_cnt].msg); \
            if (msg_) strncpy(g_errors[g_errors_cnt].msg, (msg_), sizeof(g_errors[g_errors_cnt].msg) - 1); \
            ++g_errors_cnt; \
        } \
        return (err_); \
    }
#define ERRIF(cond, err) ERRIFMSG(cond, err, "")    
#define SDLERRIF(cond) ERRIFMSG(cond, ERR_PLAY_WAV_SDL_FAILED, SDL_GetError())

static Error PlayWavRW(SDL_RWops * data) {
    // load WAV file
    SDL_AudioSpec wavSpec;
    Uint32 wavLength = 0;
    Uint8 * wavBuffer = 0;
    SDLERRIF(SDL_LoadWAV_RW(data, 0, &wavSpec, &wavBuffer, &wavLength) == 0);
    
    // open audio device
    SDL_AudioDeviceID dev = 0;
    SDLERRIF((dev = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0)) == 0);
    
    // clear existing audio in queue
    SDL_ClearQueuedAudio(dev);
    
    // play audio
    SDLERRIF(SDL_QueueAudio(dev, wavBuffer, wavLength) != 0);
    SDL_PauseAudioDevice(dev, 0);
    
    // wait while audio is playing
    while (1) {
        Uint32 queued_size = SDL_GetQueuedAudioSize(dev);
        if (queued_size == 0)
            break;
        SDL_Delay(20);
    }

    // stop playing    
    SDL_PauseAudioDevice(dev, 1);
    
    // clean up
    SDL_CloseAudioDevice(dev);
    SDL_FreeWAV(wavBuffer);
     
    return 0;
}

static bool g_sdl_inited = false;

static void PlayWavInit() {
    if (!g_sdl_inited) {
        SDL_Init(SDL_INIT_AUDIO);
        g_sdl_inited = true;
    }
}

static void PlayWavQuit() {
    if (g_sdl_inited) {
        SDL_Quit();
        g_sdl_inited = false;
    }
}

Error PlayWavMem(char const * buf, uint32_t size) {
    PlayWavInit();
    SDL_RWops * data = 0;
    SDLERRIF((data = SDL_RWFromConstMem(buf, size)) == 0);
    ERRIF(PlayWavRW(data) != 0, ERR_PLAY_WAV_PLAY_WAV_RW_FAILED);
    SDL_FreeRW(data);
    return 0;
}

Error PlayWavFile(char const * file_name) {
    PlayWavInit();
    SDL_RWops * data = 0;
    SDLERRIF((data = SDL_RWFromFile(file_name, "rb")) == 0);
    ERRIF(PlayWavRW(data) != 0, ERR_PLAY_WAV_PLAY_WAV_RW_FAILED);
    SDL_FreeRW(data);
    return 0;
}

char const * PlayWavError(int clear_errors) {
    static char buf[2048];
    MEMZERO(buf);
    for (uint32_t i = 0; i < g_errors_cnt; ++i)
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d:%s:%d:%s():\"%s\":\"%s\"\n",
            i, g_errors[i].file, g_errors[i].line, g_errors[i].func, /*g_errors[i].serr, g_errors[i].err,*/ g_errors[i].code, g_errors[i].msg);
    if (clear_errors)
        ClearErrors();
    return buf;
}