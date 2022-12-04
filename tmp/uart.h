//-------------------------------------------------------------------------------
//  TinyCircuits ST BLE TinyShield UART Example Sketch
//  Last Updated 2 March 2016
//
//  This demo sets up the BlueNRG-MS chipset of the ST BLE module for compatiblity 
//  with Nordic's virtual UART connection, and can pass data between the Arduino
//  serial monitor and Nordic nRF UART V2.0 app or another compatible BLE
//  terminal. This example is written specifically to be fairly code compatible
//  with the Nordic NRF8001 example, with a replacement UART.ino file with
//  'aci_loop' and 'BLEsetup' functions to allow easy replacement. 
//
//  Written by Ben Rose, TinyCircuits http://tinycircuits.com
//
//-------------------------------------------------------------------------------

#pragma once

#include <stdint.h>


#define PIPE_UART_OVER_BTLE_UART_TX_TX 0


extern uint8_t ble_rx_buffer[21];
extern uint8_t ble_rx_buffer_len;
extern uint8_t ble_connection_state;

struct ble_conf {
  uint8_t bdaddr[6];
  const char *name;
};


int BLEsetup(struct ble_conf);

void aci_loop();

uint8_t lib_aci_send_data(uint8_t, uint8_t *, uint8_t);
