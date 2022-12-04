#pragma once

#include <string>

#include <cstddef>
#include <cstdint>

#include "arduino.h"

#include "i2c.h"


namespace bma250 {
// NOTE ref http://www1.futureelectronics.com/doc/BOSCH/BMA250-0273141121.pdf
/* 5.2 Register map */
struct register_map_s {
	/* */
	uint8_t chip_id;

	/* */
	char : 8;

	/* */
	struct accl_data_s {
		bool new_data : 1;
		char : 5;
		int16_t acc : 10;

		operator std::string() const {
			return std::to_string(this->acc)
				+ (this->new_data ? "*" : "");
		}
	} __attribute__((packed));
	struct accl_dataset_s {
		struct accl_data_s x, y, z;

		operator std::string() const {
			return "x = " + std::string(this->x) + ", "
				+ "y = " + std::string(this->y) + ", "
				+ "z = " + std::string(this->z);
		}
	} __attribute__((packed)) accl_dataset;

	/* 5.5 Temperature data */
	uint8_t temp_data;

	/* 5.6 Status registers */
	enum sign_e : uint8_t {
		SIGN_POS = 0,
		SIGN_NEG = 1
	};
	enum orient_xy_e : uint8_t {
		ORIENTXY_PORTRAIT_UP   = 0b00,
		ORIENTXY_PORTRAIT_DOWN = 0b01,
		ORIENTXY_LAND_LEFT     = 0b10,
		ORIENTXY_LAND_RIGHT    = 0b11
	};
	enum orient_z_e : uint8_t {
		ORIENTZ_UP   = 0,
		ORIENTZ_DOWN = 1
	};
	struct intt_stat_s {
		// Table 31: Interrupt status, register (0x09)
		struct intt_s {
			bool low_int : 1,
				high_int : 1,
				slope_int : 1;
			char : 1;
			bool d_tap_int : 1,
				s_tap_int : 1;
			bool orient_int : 1,
				flat_int : 1;
		} __attribute__((packed)) intt;

		// Table 32: New data status, register (0x0A)
		struct new_data_s {
			char : 7;
			bool data_int : 1;
		} __attribute__((packed)) new_data;

		// Table 33: Tap and slope interrupts status, register (0x0B)
		struct tap_slope_s {
			bool slope_first_x : 1,
				slope_first_y : 1,
				slope_first_z : 1;
			enum sign_e slope_sign : 1;

			bool tap_first_x : 1,
				tap_first_y : 1,
				tap_first_z : 1;
			enum sign_e tap_sign : 1;
		} __attribute__((packed)) tap_slope;

		// Table 34: Flat and orientation Status, register (0x0C)
		struct flat_orient_s {
			bool high_first_x : 1,
				high_first_y : 1,
				high_first_z : 1;
			bool high_sign : 1;

			enum orient_xy_e orient_xy : 2;
			enum orient_z_e orient_z : 1;

			bool flat : 1;
		} __attribute__((packed)) flat_orient;
	} __attribute__((packed)) intt_stat;

	/* Registers (0x0D) and (0x0E) are reserved */
	char : 8;
	char : 8;

	/* 4.4.1 Acceleration data */
	enum range_e : uint8_t {
		RANGE_2G  = 0b0011,
		RANGE_4G  = 0b0101,
		RANGE_8G  = 0b1000,
		RANGE_16G = 0b1100
	};
	/* */
	struct range_conf_s {
		enum range_e range : 4;
		char : 4;
	} __attribute__((packed)) range_conf;

	/* */
	enum bw_e : uint8_t {
		INTVL_64MS  = 0b1000,
		INTVL_32MS  = 0b1001,
		INTVL_16MS  = 0b1010,
		INTVL_8MS   = 0b1011,
		INTVL_4MS   = 0b1100,
		INTVL_2MS   = 0b1101,
		INTVL_1MS   = 0b1110,
		INTVL_0_5MS = 0b1111
	};
	struct bw_conf_s {
		bw_e bw : 5;
		char : 3;
	} __attribute__((packed)) bw_conf;

	/* */
	enum sleepdur_e : uint8_t {
		DUR_1000MS = 0b1111,
		DUR_500MS  = 0b1110,
		DUR_100MS  = 0b1101,
		DUR_50MS   = 0b1100,
		DUR_25MS   = 0b1011,
		DUR_10MS   = 0b1010,
		DUR_6MS    = 0b1001,
		DUR_4MS    = 0b1000,
		DUR_2MS    = 0b0111,
		DUR_1MS    = 0b0110,
		DUR_0_5MS  = 0b0101
	};
	struct pwr_conf_s {
		char : 1;
		enum sleepdur_e sleep_dur : 4;
		char : 1;
		bool lowpower_en : 1;
		bool suspend : 1;
	} __attribute__((packed)) pwr_conf;

	/* 5.10 Special control settings */
	enum resetcmd_e : uint8_t {
		RESETCMD_SOFT = 0xB6,
		RESETCMD_DONE = 0x00
	};
	struct specctrl_conf_s {
		char : 8;

		char : 6;
		bool shadow_dis : 1;
		bool data_high_bw : 1;

		enum resetcmd_e softreset;

		char : 8;
	} __attribute__((packed)) specctrl_conf;

	/* 5.11 Interrupt settings */
	struct intt_conf_s {
		// Table 39: Interrupt setting, register (0x16)
		bool slope_en_x : 1,
			slope_en_y : 1,
			slope_en_z : 1;
		char : 1;
		bool d_tap_en : 1,
			s_tap_en : 1;
		bool orient_en : 1;
		bool flat_en : 1;

		// Table 40: Interrupt setting, register (0x17)
		bool high_en_x : 1,
			high_en_y : 1,
			high_en_z : 1;
		bool low_en : 1;
		bool data_en : 1;
		char : 3;
	} __attribute__((packed)) intt_conf;

	// Register (0x18) is reserved.
	char : 8;

	struct intt_map_conf_s {
		// Table 41 - 43: Interrupt mapping, register (0x19 - 0x1B)
		bool int1_low : 1,
			int1_high : 1,
			int1_slope : 1;
		char : 1;
		bool int1_d_tap : 1,
			int1_s_tap : 1;
		bool int1_orient : 1;
		bool int1_flat : 1;
		bool int1_data : 1;

		char : 6;

		bool int2_data : 1;
		bool int2_low : 1,
			int2_high : 1,
			int2_slope : 1;
		char : 1;
		bool int2_d_tap : 1,
			int2_s_tap : 1;
		bool int2_orient : 1;
		bool int2_flat : 1;
	} __attribute__((packed)) intt_map_conf;

	// Registers (0x1C) and (0x1D) are reserved.
	char : 8;
	char : 8;

	// Table 44: Interrupt data source definition, register (0x1E)
	struct intt_dsrc_conf_s {
		bool int_src_low : 1;
		bool int_src_high : 1;
		bool int_src_slope : 1;
		char : 1;
		bool int_src_tap : 1;
		bool int_src_data : 1;
		char : 1;
		char : 1;
	} __attribute__((packed)) intt_dsrc_conf;

	// Register (0x1F) is reserved.
	char : 8;

	// Table 45: Electrical behaviour of interrupt pin, register (0x20)
	struct intt_elec_conf_s {
		bool int1_lvl : 1;
		bool int1_od : 1;

		bool int2_lvl : 1;
		bool int2_od : 1;

		char : 1;
		char : 1;
		char : 1;
		char : 1;
	} __attribute__((packed)) intt_elec_conf;

	/* ... */
	enum latch_e : uint8_t {
		LATCHMODE_NONE        = 0b0000,
		LATCHMODE_TEMP_250MS  = 0b0001,
		LATCHMODE_TEMP_500MS  = 0b0010,
		LATCHMODE_TEMP_1s     = 0b0011,
		LATCHMODE_TEMP_2s     = 0b0100,
		LATCHMODE_TEMP_4s     = 0b0101,
		LATCHMODE_TEMP_8s     = 0b0110,
		LATCHMODE_FULL        = 0b0111,
		//LATCHMODE_NONE        = 0b1000,
		LATCHMODE_TEMP_500US  = 0b1001,
		//LATCHMODE_TEMP_500US  = 0b1010,
		LATCHMODE_TEMP_1MS    = 0b1011,
		LATCHMODE_TEMP_12_5MS = 0b1100,
		LATCHMODE_TEMP_25MS   = 0b1101,
		LATCHMODE_TEMP_50MS   = 0b1110,
		//LATCHMODE_FULL        = 0b1111
	};
	// Table 46: Interrupt reset bit and interrupt mode selection, register (0x21)
	struct intt_ctrl_s {
		enum latch_e latch_int : 4;
		char : 3;
		bool reset_int : 1;
	} __attribute__((packed)) intt_ctrl;

	// NOTE not implemented
} __attribute__((packed));

using accl_dataset_t = register_map_s::accl_dataset_s;
using range_preset_t = register_map_s::range_e;
using intvl_preset_t = register_map_s::bw_e;

enum event_e {
	NEW_DATA,
	SLOPE
};

enum interrupt_e {
	INT1 = 1,
	INT2 = 2
};

class bma250 {
protected:
	// TODO dependency
	using i2c_io = vendor_samd::i2c_socket::primary;

	i2c_io *io = nullptr;

	std::size_t _write(
		const uint8_t &addr,
		const char &data
	) {
		// NOTE bma250 only allows one byte per write transaction

		struct packet_s {
			uint8_t reg_addr;
			char payload;
		} __attribute__((packed)) p = {
			.reg_addr = addr,
			.payload = data
		};

		return this->io->send(&p, sizeof(p));
	}

	std::size_t _read(
		const uint8_t &addr,
		char *data, std::size_t len
	) {
		struct packet_req_s {
			uint8_t reg_addr;
		} __attribute__((packed)) p = {
			.reg_addr = addr
		};

		// closed
		if (this->io->send(&p, sizeof(p)) == 0)
			return 0;
		return this->io->recv(data, len);
	}

public:
	bma250() {}

	bma250 &init(i2c_io *io) { 
		this->io = io; 

		//this->reset();

		return *this;
	}

	std::size_t write(
		const uint8_t &addr,
		const char *data, std::size_t len
	) {
		std::size_t len_ = 0;
		std::size_t slen;

		while (len_ < len) {
			slen = this->_write(addr, data[len_]);
			// closed
			if (slen == 0)
				return len_;

			len_ += slen;
		}

		return len_;
	}

	std::size_t read(
		const uint8_t &addr,
		char *data, std::size_t len
	) { return this->_read(addr, data, len); }

	template <typename data_type>
	std::size_t write(const uint8_t addr, const data_type &data) {
		return this->write(addr, (char *)&data, sizeof(data));
	}

	template <typename data_type>
	std::size_t read(const uint8_t addr, data_type &data) {
		return this->read(addr, (char *)&data, sizeof(data));
	}

	// register manipulation convenience macros 
	#define bma250_register_field_type(field)	\
		decltype(((::bma250::register_map_s *)nullptr)->field)

	// this is ugly has hell but there's no way around it
	// field: the name of field inside bma250::register_map (e.g. intt_stat.intt)
	// accl: a bma250::bma250 object
	#define bma250_register_read(accl, field) ({	\
		bma250_register_field_type(field) __r;	\
		(accl)->read(offsetof(::bma250::register_map_s, field), __r);	\
		__r;	\
	})

	#define bma250_register_write(accl, field, ...) ({	\
		(accl)->write(offsetof(::bma250::register_map_s, field),	\
			((bma250_register_field_type(field))__VA_ARGS__));	\
	})

	using reg = register_map_s;

	void reset() {
		bma250_register_write(this, 
			specctrl_conf.softreset, 
			reg::RESETCMD_SOFT
		);
		/*while (
			bma250_register_read(this, 
				specctrl_conf.softreset
			) != reg::RESETCMD_DONE
		);*/
	}

	void accept() {
		reg::intt_ctrl_s intt_ctrl;
		this->read(offsetof(reg, intt_ctrl), intt_ctrl);
		
		intt_ctrl.reset_int = true;
		this->write(offsetof(reg, intt_ctrl), intt_ctrl);
	}

	using latch_e = reg::latch_e;
	
	void listen(event_e ev, 
		interrupt_e intt, 
		bool req_accept = false,
		bool enable = true
	) {
		reg::intt_conf_s intt_conf = {
			.slope_en_x = false,
			.slope_en_y = false,
			.slope_en_z = false,
			.d_tap_en = false,
			.s_tap_en = false,
			.orient_en = false,
			.flat_en = false,

			.high_en_x = false,
			.high_en_y = false,
			.high_en_z = false,
			.low_en = false,
			.data_en = false
		};

		switch (ev) {
		case event_e::NEW_DATA:
			intt_conf.data_en = enable;
			break;
		case event_e::SLOPE:
			intt_conf.slope_en_x
				= intt_conf.slope_en_y
				= intt_conf.slope_en_z
				= enable;
			break;
		}

		/*
		reg::intt_map_conf_s intt_map_conf = {};
		switch (ev) {
		case event_e::NEW_DATA:
			switch (intt) {
			case interrupt_e::INT1:
				intt_map_conf.int1_data = enable;
				break;
			case interrupt_e::INT2:
				intt_map_conf.int2_data = enable;
				break;
			}
			break;
		case event_e::SLOPE:
			switch (intt) {
			case interrupt_e::INT1:
				intt_map_conf.int1_slope = enable;
				break;
			case interrupt_e::INT2:
				intt_map_conf.int2_slope = enable;
				break;
			}
			break;
		}

		reg::intt_elec_conf_s intt_elec_conf = {};
		switch (intt) {
		case interrupt_e::INT1:
			intt_elec_conf.int1_od = false;
			intt_elec_conf.int1_lvl = false;
			break;
		case interrupt_e::INT2:
			intt_elec_conf.int2_od = false;
			intt_elec_conf.int2_lvl = false;
			break;
		}

		this->write(
			offsetof(reg, intt_map_conf),
			intt_map_conf
		);
		this->write(
			offsetof(reg, intt_elec_conf),
			intt_elec_conf
		);*/

		// TODO 
		if (enable)
			this->listen(ev, intt, req_accept, false);
		delayMicroseconds(600);

		bma250_register_write(this, intt_ctrl, { 
			.latch_int = req_accept
				? reg::LATCHMODE_FULL
				: reg::LATCHMODE_NONE,
			.reset_int = false
		});

		bma250_register_write(this, intt_conf, intt_conf);		
	}

	template <typename cb_type>
	void listen(event_e ev, interrupt_e intt, cb_type cb, bool req_accept = false) {
		this->listen(ev, intt, cb, req_accept);

		// TODO turn off interrupt??
		if (cb == nullptr)
			return;

		// TODO INT1
		attachInterrupt(13, cb, CHANGE);
		// TODO INT0
		//attachInterrupt(11, cb, CHANGE);
	}

	bma250 &set_intvl(intvl_preset_t intvl) {
		reg::bw_conf_s bw_conf = {};
		bw_conf.bw = intvl;
		this->write(offsetof(reg, bw_conf), bw_conf);
		// TODO error handling
		return *this;
	}

	bma250 &set_range(range_preset_t range) {
		reg::range_conf_s range_conf = {};
		range_conf.range = range;
		this->write(
			offsetof(
				reg,
				range_conf
			),
			range_conf
		);
		// TODO error handling
		return *this;
	}

	using sleepdur_e = reg::sleepdur_e;
	bma250 &set_lopower(sleepdur_e sleep_dur) {
		bma250_register_write(this, pwr_conf, {
			.sleep_dur = sleep_dur,
			.lowpower_en = true
		});
		return *this;
	}

	accl_dataset_t read_accl() {
		reg::accl_dataset_s res;

		// TODO error checking
		this->read(
			offsetof(
				reg,
				accl_dataset
			),
			res
		);

		return res;
	}
};
}

namespace tinyzero {
class bma250 : public ::bma250::bma250 {
protected:
	using base = ::bma250::bma250;

public:
	bma250() : base() {}

	bma250 &init(base::i2c_io *io) {
		this->base::init(io);

		// 6.2 Inter-Integrated Circuit (I2C) 
		// "The default I2C address of the device is 0011000b (0x18). 
		//		... The alternative address 0011001b (0x19)." 
		const vendor_samd::i2c_address addrs[] = {{0x18}, {0x19}};
		for (auto a : addrs) {
			this->io->connect(a);
			this->io->send(nullptr, 0);
			if (this->io->status == vendor_samd::i2c_status::ST_SUCCESS)
				return *this;
			// TODO raise error
		}

		return *this;
	}

	void listen(::bma250::event_e ev, bool req_accept = false) {
		return base::listen(ev, ::bma250::interrupt_e::INT1, req_accept);
	}

	template <typename cb_type>
	void listen(::bma250::event_e ev, cb_type cb, bool req_accept = false) {
		// NOTE ~~only INT1 is attached to the main board~~
		// ref https://github.com/tinyzero/tinyzero-TinyZero-ASM2021/blob/master/design_files/TinyZero_Schematic_Rev5.pdf
		return base::listen(ev, ::bma250::interrupt_e::INT1, cb, req_accept);
	}
};

inline bma250 accel;
}