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
//  customized by Aaron Mega Corp
//
//-------------------------------------------------------------------------------


#pragma once

#define BLE_DEBUG true


struct ble_conf {
  uint8_t bdaddr[6];
  const char *name;
};

// Initalize the BLE driver
int ble_init(struct ble_conf conf);

// Run this in the main loop to handle any BLE events
bool ble_loop();

uint8_t Write_UART_TX(char* TXdata, uint8_t datasize);
