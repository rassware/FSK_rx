/*
*   by dl8mcg Jan. 2025
*/

#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define FRAMES_PER_BUFFER 2
#define SAMPLING_RATE 13333.3333f    // Sampling rate in Hz

extern volatile bool decready;

extern volatile char decoded_char;

#endif // CONFIG_H
