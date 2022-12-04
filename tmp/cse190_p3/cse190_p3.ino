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

#include <SPI.h>
#include "STBLE.h"
#include "ble.h"

#define PIPE_UART_OVER_BTLE_UART_TX_TX 0

void setup() {
  SerialUSB.begin(115200);
  while (!SerialUSB); //This line will block until a serial monitor is opened with TinyScreen+!
  SerialUSB.print("Starting...\n");
  ble_init();
}


void loop() {
  ble_loop(); //Process any ACI commands or events from the main BLE handler, must run often. Keep main loop short.

  if (SerialUSB.available()) {//Check if serial input is available to send
    delay(10);//should catch input
    uint8_t sendBuffer[21];
    uint8_t sendLength = 0;
    while (SerialUSB.available() && sendLength < 19) {
      sendBuffer[sendLength] = SerialUSB.read();
      sendLength++;
    }
    if (SerialUSB.available()) {
      SerialUSB.print(F("Input truncated, dropped: "));
      if (SerialUSB.available()) {
        SerialUSB.write(SerialUSB.read());
      }
    }
    sendBuffer[sendLength] = '\0'; //Terminate string
    sendLength++;
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)sendBuffer, sendLength))
    {
      SerialUSB.println(F("TX dropped!"));
    }
  }
}
