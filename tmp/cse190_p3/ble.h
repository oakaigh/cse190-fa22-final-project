#ifndef BLE_H
#define BLE_H

// Initalize the BLE driver
int ble_init();

// Run this in the main loop to handle any BLE events
void ble_loop();

uint8_t lib_aci_send_data(uint8_t ignore, uint8_t* sendBuffer, uint8_t sendLength);

#endif
