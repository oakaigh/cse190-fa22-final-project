#pragma once

#include <cstddef>
#include <string>

namespace logging {
// verbosity level (low to high)
enum level_e : std::ptrdiff_t {
	L_NOTSET = -1,
	L_ERR = 0,
	L_MIN = L_ERR,
	L_INFO = 1,
	L_DEBUG = 2,
	L_MAX = L_DEBUG
};

class base_logger {
public:
	using print_fn_t = void (level_e, const std::string &);

protected:
	level_e lvl = level_e::L_NOTSET;

	print_fn_t *pr_f = nullptr;

public:
	base_logger(print_fn_t *pr_f_) : pr_f(pr_f_) {}

	base_logger &set_level(level_e lvl) {
		this->lvl = lvl;
		return *this;
	}

	virtual void print(level_e lvl, const std::string &s) {
		if (lvl > this->lvl)
			return;

		if (this->pr_f == nullptr)
			return;
		this->pr_f(lvl, s);
	}

	void error(std::string s) { return this->print(level_e::L_ERR, s); }
	void info(std::string s) { return this->print(level_e::L_INFO, s); }
	void debug(std::string s) { return this->print(level_e::L_DEBUG, s); }
};

class logger : public base_logger {
protected:
	static std::string delim(
		const std::string &s,
		const std::string &prefix = "[",
		const std::string &suffix = "]"
	) { return prefix + s + suffix; }

	std::string name = "";
	std::string lvl_prefixes[level_e::L_MAX + 1] = {
		[level_e::L_ERR] = "E",
		[level_e::L_INFO] = "I",
		[level_e::L_DEBUG] = "D"
	};
	std::string prefix = " ";

public:
	logger(
		const std::string &name,
		print_fn_t *pr_f
	) : name(name), base_logger(pr_f) {}

	void print(level_e lvl, const std::string &s) override {
		// NOTE a valid verbosity level is always indexable
		bool is_valid =
			level_e::L_MIN <= lvl
				&& lvl <= level_e::L_MAX;
		if (!is_valid)
			return;

		return this->base_logger::print(
			lvl,
			this->delim(this->name)
				+ this->delim(this->lvl_prefixes[lvl])
				+ this->prefix
				+ s
		);
	}
};
}

#include "arduino.h"

namespace tinyzero {
class usbserial_logger : public logging::logger {
public:
	usbserial_logger(const std::string &name)
		: logging::logger(
			name,
			[](logging::level_e _l, const std::string &s) {
				SerialUSB.println(s.c_str());
				SerialUSB.flush();
			}
		) {}

	usbserial_logger &init(std::size_t baudrate = 115200) {
		SerialUSB.begin(baudrate);
		return *this;
	}
};

// TODO use ledcircle code base
// ref arduino example blink
// see https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
class led_logger : public logging::base_logger {
public:
	led_logger(const std::string &name) 
		: logging::base_logger(
			[](logging::level_e _l, const std::string &s) {
				// NOTE only blink led everything else is ignored

				// TODO
				digitalWrite(LED_BUILTIN, 
					s.empty() 
						? LOW	// turn the LED off by making the voltage LOW
						: HIGH	// turn the LED on (HIGH is the voltage level)
				);
			}
		) {}

	led_logger &init() {
		pinMode(LED_BUILTIN, OUTPUT);
		return *this;
	}
};
}
