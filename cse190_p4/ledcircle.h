#pragma once

#include <stddef.h>

#include "arduino.h"

#include "utils.h"


// TODO
namespace tinyzero {

namespace displays {
class ledcircle {
protected:
	class pin {
		uint32_t _base;

	public:
		template <typename T>
		pin(T n) : _base(n) {}

		void
		set_high()
		{
			digitalWrite(this->_base, HIGH);
			pinMode(this->_base, OUTPUT);
		}

		void
		set_low()
		{
			digitalWrite(this->_base, LOW);
			pinMode(this->_base, OUTPUT);
		}

		void
		set_high_z()
		{
			pinMode(this->_base, INPUT);
			digitalWrite(this->_base, LOW);
		}
	};

public:
	class led {
		pin pin_hi, pin_lo;

	public:
		template <typename T>
		led(T pin_hi, T pin_lo)
			: pin_hi(pin_hi), pin_lo(pin_lo) {}

		void
		enable()
		{
			this->pin_hi.set_high();
			this->pin_lo.set_low();
		}

		void
		off()
		{
			this->pin_hi.set_high_z();
			this->pin_lo.set_low();
		}

		void
		disable()
		{
			this->pin_hi.set_high_z();
			this->pin_lo.set_high_z();
		}
	};

	pin pins[5] = {5, 6, 7, 8, 9};
	led leds[16] = {
		led(5, 6),
		led(6, 5),
		led(5, 7),
		led(7, 5),
		led(6, 7),
		led(7, 6),
		led(6, 8),
		led(8, 6),
		led(5, 8),
		led(8, 5),
		led(8, 7),
		led(7, 8),
		led(9, 7),
		led(7, 9),
		led(9, 8),
		led(8, 9)
	};

	ledcircle() {}

	void
	led_on(std::size_t led_i)
	{
		this->leds[led_i].enable();
	}

	void
	led_off(std::size_t led_i)
	{
		this->leds[led_i].disable();
	}

	// duration in micro sec
	void
	enable(uint16_t leds, std::size_t duration = 1)
	{
		for (
			std::size_t i = 0;
			i < sizeof_bit(leds);
			i++
		) {
			if (bit_read(leds, i))
				this->led_on(i);
			delayMicroseconds(duration);
			this->led_off(i);
		}
	}

	void
	disable()
	{
		for (auto &pin : this->pins)
			pin.set_high_z();
	}

	void
	display(uint16_t leds, std::size_t duration = 0)
	{
		std::size_t n = __builtin_popcount(leds);
		if (n == 0) {
			//this->enable(leds);
			this->disable();
			delayMicroseconds(duration);
			return;
		}

		this->enable(leds, duration / n);
	}
} ledcircle;	
}

}
