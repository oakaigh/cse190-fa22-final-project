#include "arduino_bluenrg_ble.h"
#include "arduino_hal.h"


//Debug output adds extra flash and memory requirements!
#define BLE_DEBUG true

#ifdef __cplusplus
 extern "C" {
#endif
	#include "bluenrg_utils.h"
	#include "hal_types.h"
	#include "ble_status.h"
	#include "hci.h"
	#include "hci_le.h"
	#include "bluenrg_aci.h"
	#include "gp_timer.h"
	#include "bluenrg_gap.h"
	#include "sm.h"
#ifdef __cplusplus
}

#endif
