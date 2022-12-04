#pragma once

#include <samd.h>
#include "arduino.h"

#include <cstddef>
#include <initializer_list>


#include "clock.h"
#include "port.h"


// SAMD21 I2C controller abstraction
// 28. SERCOM I2C – Inter-Integrated Circuit
namespace vendor_samd {
// serial communication interface info
struct i2c_info {
	Sercom *ser;				// base
	enum clock::devid_e clkid;	// clock ID
	IRQn_Type irqn;				// IRQ number
	port *port_sda, *port_scl;	// port: SDA (data), SCL (clock)
};

class i2c_common {
protected:
	struct i2c_info info;

	// NOTE I2CM and I2CS share the same set of registers
	static constexpr SercomI2cm *
	common_of(Sercom *s) { return &s->I2CM; }

public:
	enum mode_e : uint32_t {
		MODE_PRIM = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER_Val,
		MODE_SUB = SERCOM_I2CM_CTRLA_MODE_I2C_SLAVE_Val
	};

	enum event_e {
		EV_ANY,
		EV_ENABLE,
		EV_SYSOP,
		EV_SWRESET
	};

	i2c_common(const struct i2c_info &info) : info(info) {}

	i2c_common &init(clock *clk) {
		this->reset();

		// configure clock
		clk->attach(this->info.clkid);

		// configure port IO type
		for (auto *p : {
			this->info.port_sda,
			this->info.port_scl
		}) {
			p->set_mux(port_type_e::PTYPE_C_SERCOM);
		}

		return *this;
	}

	enum state_e : uint16_t {
		ST_UNKNOWN = 0,
		ST_IDLE = 1,
		ST_OWNER = 2,
		ST_BUSY = 3
	};

	i2c_common &reset() {
		common_of(this->info.ser)->CTRLA.bit.SWRST = true;
		this->wait_sync(event_e::EV_SWRESET);
		return *this;
	}

	i2c_common &force_idle() {
		common_of(this->info.ser)->STATUS.bit.BUSSTATE = state_e::ST_IDLE;
		this->wait_sync(event_e::EV_SYSOP);
		return *this;
	}

	i2c_common &enable() {
		common_of(this->info.ser)->CTRLA.bit.ENABLE = true;
		this->wait_sync(event_e::EV_ENABLE);		

		this->force_idle();

		// TODO interrupts
		//NVIC_EnableIRQ(this->info.irqn);

		return *this;
	}

	i2c_common &disable() {
		// TODO interrupts
		//NVIC_DisableIRQ(this->info.irqn);

		common_of(this->info.ser)->CTRLA.bit.ENABLE = false;
		this->wait_sync(event_e::EV_ENABLE);
		return *this;
	}

	// 28.10.8 Synchronization Busy
	// SYSOP (event_e::EV_SYSOP): (when enabled) CTRLB.CMD, STATUS.BUSSTATE, ADDR, DATA
	// ENABLE (event_e::EV_ENABLE): CTRLA.ENABLE
	i2c_common &wait_sync(enum event_e ev = event_e::EV_ANY) {
		bool is_busy = false;
		while (true) {
			auto &s = common_of(this->info.ser)->SYNCBUSY;

			switch (ev) {
			case event_e::EV_ANY:
				is_busy = s.reg != false;
				break;
			case event_e::EV_ENABLE:
				is_busy	= s.bit.ENABLE;
				break;
			case event_e::EV_SYSOP:
				is_busy = s.bit.SYSOP;
				break;
			case event_e::EV_SWRESET:
				is_busy = s.bit.SWRST;
				break;
			}

			if (!is_busy)
				break;
		}
		return *this;
	}

	i2c_common &set_mode(enum mode_e mode) {
		common_of(this->info.ser)->CTRLA.bit.MODE = mode;
		return *this;
	}

	enum mode_e get_mode() {
		return (enum mode_e)common_of(this->info.ser)->CTRLA.bit.MODE;
	}
};

// I2C header
// NOTE 7 bit addr mode ONLY
union i2c_header {
	struct {
		bool is_read : 1;
		uint8_t addr : 7;
	} __attribute__((packed)) s7;
	uint8_t raw7;
} __attribute__((packed));

// I2C address
union i2c_address {
	uint8_t addr7;
};

// I2C status
enum i2c_status : uint8_t {
	ST_SUCCESS = 0,
	ST_TOOLONG = 1,
	ST_NACK_ADDR = 2,
	ST_NACK_DATA = 3,
	ST_OTHER = 4
};

// I2C primary mode
class i2c_primary : public i2c_common {
protected:
	using base = i2c_common;
	using status_e = i2c_status;

	static const constexpr bool smart = false;

public:
	status_e status = status_e::ST_SUCCESS;

	using base::i2c_common;

	i2c_primary &init(clock *clk, uint8_t baudrate) {
		// configure clock
		this->base::init(clk);

		// configure as primary
		this->set_mode(base::mode_e::MODE_PRIM);
		// set baud rate
		this->info.ser->I2CM.BAUD.bit.BAUD = baudrate;

		// TODO smart mode
		this->info.ser->I2CM.CTRLB.bit.SMEN = this->smart;

		return *this;
	}

	enum ackact_e : uint32_t {
		ACKACT_ACK = 0,
		ACKACT_NACK = 1
	};

	i2c_primary &send_ack() {
		if (this->smart)
			return *this;
		this->info.ser->I2CM.CTRLB.bit.ACKACT 
			= ackact_e::ACKACT_ACK;
		return *this;
	}

	i2c_primary &send_nack() {
		if (this->smart)
			return *this;
		this->info.ser->I2CM.CTRLB.bit.ACKACT 
			= ackact_e::ACKACT_NACK;
		return *this;
	}

	enum cmd_e : uint32_t {
		CMD_NOOP = 0,
		CMD_REPEAT_START = 1,
		CMD_READ = 2,
		CMD_STOP = 3
	};

	i2c_primary &send_cmd(enum cmd_e cmd) {
		this->info.ser->I2CM.CTRLB.bit.CMD = cmd;
		this->wait_sync(base::event_e::EV_SYSOP);
		return *this;
	}

	i2c_primary &wait_write_avail() {
		while (!this->info.ser->I2CM.INTFLAG.bit.MB);
		return *this;
	}

	i2c_primary &wait_read_avail() {
		while (!this->info.ser->I2CM.INTFLAG.bit.SB);
		return *this;
	}

	status_e set_addr_sync(const union i2c_address &addr, bool is_read) {
		union i2c_header hdr = {
			.s7 = {
				.is_read = is_read,
				.addr = addr.addr7
			}
		};
		// NOTE FIXME 10-bit mode?
		this->info.ser->I2CM.ADDR.bit.ADDR = hdr.raw7;
		this->wait_sync(event_e::EV_SYSOP);

		if (is_read) this->wait_read_avail();
		else this->wait_write_avail();

		// TODO error checking

		// NACK received
		if (this->info.ser->I2CM.STATUS.bit.RXNACK)
			return status_e::ST_NACK_ADDR;

		return status_e::ST_SUCCESS;
	}

	status_e send_data_sync(const char &data) {
		this->info.ser->I2CM.DATA.bit.DATA = data;
		this->wait_write_avail();

		// TODO error checking

		// NACK received
		if (this->info.ser->I2CM.STATUS.bit.RXNACK)
			return status_e::ST_NACK_DATA;

		return status_e::ST_SUCCESS;
	}

	status_e recv_data_sync(char &data) {
		this->wait_read_avail();
		data = this->info.ser->I2CM.DATA.bit.DATA;

		// TODO error checking

		return status_e::ST_SUCCESS;
	}

	status_e recv_data_sync(char *data, std::size_t len, std::size_t &rlen) {
		// NOTE read ack read ack ... read nack
		for (rlen; rlen <= len; ) {
			// first read?
			if (rlen == 0) /* noop: already has data */;
			else this->send_cmd(cmd_e::CMD_READ);

			status_e s = this->recv_data_sync(data[rlen]);

			if (s != status_e::ST_SUCCESS)
				return s;

			rlen += 1;

			// last read?
			if (rlen == len) this->send_nack();
			else this->send_ack();
		}

		return status_e::ST_SUCCESS;
	}
};

namespace i2c_socket {
class primary : public i2c_primary {
protected:
	using base = i2c_primary;
	using status_e = i2c_status;

	i2c_address addr;

public:
	status_e status = status_e::ST_SUCCESS;

	primary(const struct i2c_info &info) : addr({}), base(info) {}

	primary &init(clock *clk, uint8_t baudrate) {
		this->base::init(clk, baudrate);
		return *this;
	}

	primary &connect(const i2c_address &addr) {
		this->addr = addr;
		return *this;
	}

	std::size_t send(const void *data_, std::size_t len) {
		std::size_t slen = 0;

		auto *data = (const char *)data_;

		this->status = this->base::set_addr_sync(this->addr, false);
		if (this->status != status_e::ST_SUCCESS)
			return slen;

		if (data != nullptr) {
			for (slen; slen <= len; slen += 1) {
				this->status = this->base::send_data_sync(data[slen]);
				if (this->status != status_e::ST_SUCCESS)
					return slen;
			}
		}

		this->base::send_cmd(
			i2c_primary::CMD_STOP
		);

		return slen;
	}

	std::size_t recv(void *data, std::size_t len) {
		std::size_t rlen = 0;

		this->status = this->base::set_addr_sync(this->addr, true);
		if (this->status != status_e::ST_SUCCESS)
			return rlen;

		this->status = this->base::recv_data_sync(
			(char *)data, len,
			rlen
		);
		if (this->status != status_e::ST_SUCCESS)
			return rlen;

		this->base::send_cmd(
			i2c_primary::CMD_STOP
		);

		return rlen;
	}
};
}
}

namespace tinyzero {
using i2c_address = vendor_samd::i2c_address;

namespace i2c_sockets {
inline vendor_samd::i2c_socket::primary primary_sercom3((struct vendor_samd::i2c_info) {
	.ser = SERCOM3,
	.clkid = vendor_samd::clock::devid_e::CLKDEVID_SERCOM3,
	.irqn = SERCOM3_IRQn,
	.port_sda = &tinyzero::port::ports::PA22_AD4_SDA,
	.port_scl = &tinyzero::port::ports::PA23_AD5_SCL
});
}
}
