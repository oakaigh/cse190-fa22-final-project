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
//#include "STBLE.h"

#include "src/stble/ble.h"

#define PIPE_UART_OVER_BTLE_UART_TX_TX 0

struct ble_conf _test_ble_conf = {
	.bdaddr = {0x12, 0x72, 0xe9, 0x66, 0x06, 0x00},
	.name = "PrivTag"
};


#include "src/stble/STBLE/src/STBLE.h"

#include "arduino.h"

enum local_name_type : uint8_t {
	SHORT = AD_TYPE_SHORTENED_LOCAL_NAME,
	FULL  = AD_TYPE_COMPLETE_LOCAL_NAME
} __attribute__((packed));

#include <algorithm>

#include <cstring>

template <std::size_t buf_len = 1>
struct local_name_value {
	char data[buf_len];

	local_name_value() : data({[0] = 0}) {}

	local_name_value(const char *s, std::size_t len) {
		this->assign(s, len);
	}

	local_name_value(const char *s) {
		this->assign(s);
	}

	local_name_value &assign(const char *s, std::size_t len) {
		std::memcpy(this->data, s, std::min(len, buf_len));
		return *this;
	}

	local_name_value &assign(const char *s) {
		return this->assign(s, strlen(s));
	}

	constexpr std::size_t size() const { return strlen(this->data) + 1; }

	constexpr std::size_t capacity() const { return buf_len; }
} __attribute__((packed));


// see aci_gap_set_discoverable
template <std::size_t buf_len = 40 - 14>
struct local_name {
	using type_e = local_name_type;

	enum local_name_type type;
	struct local_name_value<buf_len> val;

	constexpr std::size_t size() { 
		return sizeof(this->type) + this->val.size(); 
	}
} __attribute__((packed));


void testa() {
	struct local_name<> ln = {
		.type = local_name_type::FULL,
		.val = "CSE190A"
	};
}




void setup() {
	SerialUSB.begin(115200);
	//while (!SerialUSB); //This line will block until a serial monitor is opened with TinyScreen+!
	SerialUSB.print("Starting...\n");
	ble_init(_test_ble_conf);
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
