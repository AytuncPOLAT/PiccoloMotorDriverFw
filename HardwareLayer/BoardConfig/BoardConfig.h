#ifndef HARDWARE_LAYER_BOARD_CONFIG_H
#define HARDWARE_LAYER_BOARD_CONFIG_H

#if defined(BOARD_PICCOLO2)
#include "Piccolo2.h"
#elif defined(BOARD_PICCOLOZERO)
#include "PiccoloZero.h"
#else
#error "Select a supported PCB with the CMake BOARD option"
#endif

#endif // HARDWARE_LAYER_BOARD_CONFIG_H
