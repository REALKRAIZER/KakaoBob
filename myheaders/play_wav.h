#ifndef _PLAY_WAV_H_
#define _PLAY_WAV_H_

#include <stdint.h>
// Play WAV from memory-file.
// Returns 0 on success, and non-0 on error.
uint32_t PlayWavMem(char const * buf, uint32_t size);
// Play WAV from file.
// Returns 0 on success, and non-0 on error.
uint32_t PlayWavFile(char const * file_name);
// Returns error stack, clears errors stack if clear_errors != 0.
char const * PlayWavError(int clear_errors);

#endif // _PLAY_WAV_H_