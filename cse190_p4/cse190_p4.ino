/* === DO NOT REMOVE: Initialize C library === */
extern "C" void __libc_init_array(void);

#include "arduino.h"
#include <tuple>

#include "stble.h"
#include "utils.h"
#include "logging.h"

#include "clock.h"
#include "timer.h"
#include "ledcircle.h"
#include "bma250.h"

#include "pm.h"


static const constexpr bool production = false;

namespace privtag {

class privtag {
public:
	struct {
		char *reset = "found";
	} cmd;

	std::string name = "";

	bool is_lost;
	std::size_t n_minutes_lost;

	privtag(std::string name) : name(name) { 
		this->reset(); 
	} 

	void reset() {
		this->is_lost = false;
		this->n_minutes_lost = 0;
	}

	void set_lost() {
		this->is_lost = true;
	}

	void process_cmd(const char *cmd, std::size_t len) {
		if (utils::bytes_equal(
			cmd, len,
			this->cmd.reset, strlen(this->cmd.reset)
		)) {
			this->reset();
		}
	}

	std::string stats() const {
		return std::string("")
			+ "is lost = " + std::to_string(this->is_lost) + ", "
			+ "lost counter = " + std::to_string(this->n_minutes_lost) + "min";
	}
};

inline privtag app("CSE190A");
}



int main() {
	init();
	__libc_init_array();

	// USB configuration
	if (!production) {
		USBDevice.init();
		USBDevice.attach();		
	}

	// TODO rm debug
	while (!SerialUSB); //This line will block until a serial monitor is opened with TinyScreen+!
	SerialUSB.println("Starting...");

	// TODO
	// peripheral clock configuration
	//PM->APBCMASK.reg = PM_APBCMASK_RESETVALUE;

	PM->APBCMASK.bit.ADC_ = true;
	PM->APBCMASK.bit.DAC_ = true;
	PM->APBCMASK.bit.SERCOM3_ = true;
	PM->APBCMASK.bit.TC3_ = true;

	// logging
	static const constexpr auto log_level 
		= production
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

	// privtag app
	logger.info("privtag: initialization");
	static constexpr auto &app = privtag::app;
	app.reset();

	// bluetooth

	// TODO rm debug
	delay(5000);

	logger.info("bluetooth: initialization");
	static constexpr auto &bt = tinyzero::bluetooth;

	// TODO err handling
	bt.init((stble::ble_info) {
		.addr = { .addr = {0x12, 0x72, 0xe9, 0x66, 0x06, 0x00} }
	});

	// TODO rm debug
	logger.info("set dev name ");

	bt.set_dev_name("PrivTag");

		// TODO rm debug
	logger.info("set tx ");
	// set power to +4 dBm
	bt.set_txpower(true, 3);

	static const stble::local_name<> l_name = {
		.type = stble::local_name<>::type_e::FULL,
		.val = app.name
	};


		// TODO rm debug
	logger.info("set disc ");
	// TODO
	bt.set_discoverable(l_name);

	//bt.unset_discoverable();

	logger.info("bluetooth (UART): initialization");
	static stble::uart bt_uart;

	stble::uart_info bt_uart_i;
	stble::ble::status_e bt_uart_stat;
	std::tie(bt_uart_stat, bt_uart_i) = bt.add_service_uart();
	if (bt_uart_stat != bt_uart.STATUS_SUCCESS) {
		logger.error("bluetooth (UART): service failure");
		return EXIT_FAILURE;
	}		

	bt_uart.callbacks.connect = [](
		const evt_le_connection_complete &evt
	) {
		logger.debug("bluetooth (UART): connection");
	};

	bt_uart.callbacks.disconnect = [](const evt_disconn_complete &evt) {
		logger.debug("bluetooth (UART): client disconnected");

		bt.set_discoverable(l_name);

		//
		if (app.is_lost) {
			//bt.set_discoverable(l_name);
		} else {
			//bt.unset_discoverable();
		}
	};

	bt_uart.callbacks.read = [](const char *data, std::size_t len) {
		logger.debug(
			std::string("bluetooth (UART): data: ")
				+ std::string(data, len)
		);

		app.process_cmd(data, len);
	};

	if (bt_uart.init(bt_uart_i) != bt_uart.STATUS_SUCCESS) {
		logger.error("bluetooth (UART): init failure");
		return EXIT_FAILURE;
	}



	// timer
	// TODO rm when bma250 interrupt can be latched

	// how long it takes before declaring device lost
	static const constexpr std::size_t
		idle_interval_sec = 2,
		idle_timeout_sec = 60;
	
	static const constexpr std::size_t
		stat_interval_sec = 2;// TODO rm debug // 10;

	// timer
	logger.info("timer: initialization");
	static constexpr auto &timer = tinyzero::timers::tc3;
	timer.init(&clk);
	logger.debug("timer: initialization done");

	if (timer.set_interval(
		idle_interval_sec, 
		timer.INTVLPRIOR_RANGE
	) != timer.STATUS_SUCCESS) {
		logger.error("timer: invalid interval");
		return EXIT_FAILURE;
	}

	timer.listen([]() {
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
			logger_le.debug("timer: movement check: motion");
		} else {
			logger_le.debug("");
		}

		// stats
		if (n_seconds % stat_interval_sec == 0) {
			logger.info(
				std::string("timer: privtag stats: ")
					+ app.stats()
			);
			
			if (app.is_lost) {
				// TODO check if connected
				/*bt_uart.print(
					std::string("privtag ") + app.name 
						+ " has been missing for "
						+ std::to_string(app.n_minutes_lost) + " minutes."
				);*/
			}
		}

		// idle timeout
		if (n_seconds % idle_timeout_sec == 0) {
			logger.debug(
				std::string("timer: idle timeout: ")
					+ "duration = " + std::to_string(idle_timeout_sec) + "sec, "
					+ "movement = " + std::to_string(has_movement)
			);			

			if (app.is_lost)
				app.n_minutes_lost += idle_timeout_sec / 60;

			if (!has_movement) {
				app.set_lost();
				// TODO
				//bt.set_discoverable(l_name);
			} else {
				// reset lost status
				app.reset();
				//bt.unset_discoverable();
			}

			// reset status
			n_seconds = 0;
			has_movement = false;
		}
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
		if (!app.is_lost) {
			//vendor_samd::standby();
			//continue;
		}

		stble::process();
	}

	return EXIT_SUCCESS;
}


