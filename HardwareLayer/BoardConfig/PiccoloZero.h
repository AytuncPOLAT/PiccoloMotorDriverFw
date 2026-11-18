#ifndef HARDWARE_LAYER_BOARD_CONFIG_PICCOLOZERO_H
#define HARDWARE_LAYER_BOARD_CONFIG_PICCOLOZERO_H

/*
 * PiccoloZero is known to use the DRV8316R. Its pinout and electrical values
 * have not yet been provided, so this profile is intentionally not buildable.
 */
#define BOARD_NAME "PiccoloZero"
#define GATE_DRIVER_DRV8316R 1
#define BOARD_PROFILE_COMPLETE 0

#error "PiccoloZero board profile is incomplete; provide its pinout and electrical values"

#endif // HARDWARE_LAYER_BOARD_CONFIG_PICCOLOZERO_H
