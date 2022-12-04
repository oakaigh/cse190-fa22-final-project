/* === DO NOT REMOVE: Initialize C library === */
extern "C" void __libc_init_array(void);

#include "arduino.h"

#include "utils.h"
#include "logging.h"

#include "clock.h"
#include "timer.h"
#include "ledcircle.h"
#include "bma250.h"

#include "pm.h"

namespace app {
static const constexpr bool production = true;
}

// ref packages/TinyCircuits/hardware/samd/1.1.0/cores/arduino/wiring.c
/* TinyCircuits - Defining VERY_LOW_POWER breaks Arduino APIs since all pins are considered INPUT at startup
	However, it really lowers the power consumption by a factor of 20 in low power mode (0.03mA vs 0.6mA) */
#define VERY_LOW_POWER

int main(void) {
	init();
	__libc_init_array();

	// USB configuration
	if (!app::production) {
		USBDevice.init();
		USBDevice.attach();		
	}

	// TODO
	// peripheral clock configuration
	PM->APBCMASK.reg = PM_APBCMASK_RESETVALUE;

	PM->APBCMASK.bit.ADC_ = true;
	PM->APBCMASK.bit.DAC_ = true;
	PM->APBCMASK.bit.SERCOM3_ = true;
	PM->APBCMASK.bit.TC3_ = true;

	// logging
	static const constexpr auto log_level 
		= app::production
			? logging::L_NOTSET
			: logging::L_DEBUG;

	static tinyzero::usbserial_logger logger("privtag");
	// set USB serial baud rate
	logger.init(115200);
	logger.set_level(log_level);

	// out-of-band logging (USB serial unavail)
	static tinyzero::led_logger logger_le("privtag_le");
	logger_le.init();
	logger_le.set_level(log_level);

	// clock
	logger.info("clock: initialization");
	static constexpr auto &sysclk = tinyzero::clocks::clk0;
	sysclk.init();

	static constexpr auto &clk = tinyzero::clocks::clk4;
	clk.init();

	// clock enabled in standby
	clk.set_src(clk.CLKSRCID_OSCULP32K, true);

	// I2C serial
	logger.info("i2c: initialization");
	static constexpr auto &i2c = tinyzero::i2c_sockets::primary_sercom3;
	// TODO baud rate
	i2c.init(&sysclk, 100000);

	i2c.enable();

	// accelerometer
	logger.info("accelerometer: initialization");
	static constexpr auto &accel = tinyzero::accel;

	accel.init(&i2c);
	accel.set_range(bma250::range_preset_t::RANGE_2G)
		.set_intvl(bma250::intvl_preset_t::INTVL_64MS)
		.set_lopower(bma250::bma250::sleepdur_e::DUR_500MS);

	// TODO latch mode not working!!!
	accel.listen(bma250::event_e::SLOPE, true);

	// NOTE using polling instead of interrupt 
	static auto check_movement = []() -> bool {
		auto stat = bma250_register_read(&accel, intt_stat.intt);
		accel.accept();

		return stat.slope_int;
	};

	// message display
	logger.info("message display: initialization");
	static union message {
		struct {
			uint8_t id;
			overflow_protected_max<uint8_t>
			__attribute__((packed)) n_minutes;
		} __attribute__((packed)) data;
		uint16_t raw;
	} __attribute__((packed)) msg = {
		.data = {
			.id =
				// NOTE won't compile in arduino unless extension
				// is C/C++ header what a joke
				#include "id.dat.h"
		}
	};
	msg.data.n_minutes = 0;

	// timer
	// TODO rm when bma250 interrupt can be latched

	// how long it takes before declaring device lost
	static const constexpr std::size_t
		idle_interval_sec = 2,
		idle_timeout_sec = 60;

	// timer
	logger.info("timer: initialization");
	static constexpr auto &timer = tinyzero::timers::tc3;
	timer.init(&clk);
	logger.debug("timer: initialization done");

	auto s = timer.set_interval(
		idle_interval_sec, 
		timer.INTVLPRIOR_RANGE
	);
	logger.info("timer: interval set");
	if (s != timer.STATUS_SUCCESS) {
		logger.error("timer: invalid interval");
		return EXIT_FAILURE;
	}

	static bool is_lost = false;

	timer.listen([]() {
		// this is synced with the led display
		static constexpr auto &n_minutes_lost = msg.data.n_minutes;

		static std::size_t n_seconds = 0;
		static bool has_movement = false;

		logger.debug(
			std::string("timer: tick: ")
				+ "total elapsed = " 
					+ std::to_string(n_seconds) + "sec"
		);

		n_seconds += idle_interval_sec;

		// check movement
		if (check_movement()) {
			has_movement = true;
			logger.debug("timer: movement check: motion");
			logger_le.debug("timer: has movement");
		} else {
			logger_le.debug("");
		}

		if (n_seconds % idle_timeout_sec != 0)
			return;

		logger.debug(
			std::string("timer: idle timeout: ")
				+ "duration = " + std::to_string(idle_timeout_sec) + "sec, "
				+ "lost counter = " + std::to_string(n_minutes_lost) + "min, "
				+ "movement = " + std::to_string(has_movement) + ", "
				+ "is lost = " + std::to_string(is_lost)
		);

		if (is_lost)
			n_minutes_lost += idle_timeout_sec / 60;

		if (!has_movement) {
			is_lost = true;
		} else {
			// TODO reset lost status?
			// is_lost = false;
			has_movement = false;
		}

		n_seconds = 0;
	});

	/*
	// how long it takes before declaring device lost
	static const constexpr std::size_t
		idle_timeout_minutes = 1;

	logger.info("timer: initialization");
	static constexpr auto &timer = tinyzero::timers::tc3;
	timer.init(&clk);

	// NOTE interval in seconds
	auto s = timer.set_interval(
		idle_timeout_minutes * 60, 
		timer.INTVLPRIOR_RANGE
	);
	if (s != timer.STATUS_SUCCESS) {
		logger.error("timer: invalid interval");
		return EXIT_FAILURE;
	}

	// is device lost?
	static bool is_lost = false;
	
	timer.listen([]() {
		// this is synced with the value on led display
		static constexpr auto 
			&n_minutes_lost = msg.data.n_minutes;

		// check movement
		bool has_movement = check_movement();
		if (!has_movement) {
			is_lost = true;
			logger_le.debug("");
		} else {
			// TODO reset lost status?
			// is_lost = false;
			logger_le.debug("timer: has movement");
		}

		logger.debug(
			std::string("timer: idle timeout: ")
				+ "duration = " + std::to_string(idle_timeout_minutes) + "min, "
				+ "lost counter = " + std::to_string(n_minutes_lost) + "min, "
				+ "movement = " + std::to_string(has_movement) + ", "
				+ "is lost = " + std::to_string(is_lost)
		);

		// TODO
		if (is_lost)
			n_minutes_lost += idle_timeout_minutes;
	});
	*/

	// timer enabled in standby
	timer.enable(true);

	while (true) {
		if (!is_lost) {
			vendor_samd::standby();
			continue;
		}

		tinyzero::displays::ledcircle.enable(msg.raw, 1e3);
	}

	return EXIT_SUCCESS;
}
