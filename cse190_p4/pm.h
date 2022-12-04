#pragma once

#include <samd.h>

#include "arduino.h"

// SAMD21 power manager abstraction
namespace vendor_samd {
class pm {};

// enter standby mode
// implementation based on @arduino's ArduinoLowPower library
// ref https://github.com/arduino-libraries/ArduinoLowPower/blob/7fc3c446dad729d2c8d94d7617d6ce725e122c3b/src/samd/ArduinoLowPower.cpp#L38
// FOR DETAILS ON FREEDOM OF USE PLEASE REFER TO THE LICENSE INCLUDED IN THE SOURCE REPO
// https://github.com/arduino-libraries/ArduinoLowPower/blob/master/LICENSE 
inline void standby() {
	class _ctx_usb_c {
	protected:
		bool connected = false;

	public:
		void standby() {
			// check if USB "configured and opened by host"
			// see Serial_::operator bool() and USBDeviceClass::connected()
			// ref packages/TinyCircuits/hardware/samd/1.1.0/cores/arduino/USB/CDC.cpp
			// ref packages/TinyCircuits/hardware/samd/1.1.0/cores/arduino/USB/USBCore.cpp
			this->connected = USBDevice.connected();
			if (this->connected) USBDevice.standby();
			else USBDevice.detach();
		}

		void wakeup() {
			if (this->connected) /* noop */;
			else USBDevice.attach();
		}
	} _ctx_usb;

	class _ctx_systick_c {
	public:
		void standby() {
			// disable systick interrupt: (workaround for hardware bug in SAMD21)
			// see https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
			SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;	
		}

		void wakeup() {
			// reenable systick interrupt
			SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;	
		}
	} _ctx_systick;

	_ctx_usb.standby();
	_ctx_systick.standby();

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // deep sleep
	__DSB(); // wait for data sync completion
	__WFI(); // wait for interrupt

	_ctx_systick.wakeup();
	_ctx_usb.wakeup();
}
}
