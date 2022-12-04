#pragma once

#include <samd.h>

// SAMD21 port abstraction
namespace vendor_samd {
// NOTE ref https://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D21_DA1_Family_DataSheet_DS40001882F.pdf
// Table 7-1.  PORT Function Multiplexing for SAM D21 A/B/C/D Variant Devices and SAM DA1 A/B Variant Devices

enum port_group_e { PGRP_A = 0, PGRP_B = 1 };
enum port_type_e : uint8_t {
	PTYPE_A_EXTINT = 0,
	PTYPE_B_ANALOG,
	PTYPE_C_SERCOM,
	PTYPE_D_SERCOM_ALT,
	PTYPE_E_TC_TCC,
	PTYPE_F_TCC,
	PTYPE_G_COM,
	PTYPE_H_AC_GCLK
};

struct port_info {
	Port *port;
	enum port_group_e group;
	uint32_t n;
};

class port {
protected:
	struct port_info info;

public:
	port(const struct port_info &info) : info(info) {}

	port &set_mux(enum port_type_e mode) {
		auto &pg = this->info.port->Group[this->info.group];
		auto &pmux = pg.PMUX[this->info.n / 2].bit;

		// port number even?
		if (this->info.n % 2 == 0) pmux.PMUXE = mode;
		else pmux.PMUXO = mode;

		// enable muxing
		pg.PINCFG[this->info.n].bit.PMUXEN = true;

		return *this;
	}
};
}

namespace tinyzero::port {
namespace ports {
inline vendor_samd::port
	PA22_AD4_SDA({PORT, vendor_samd::port_group_e::PGRP_A, 22}),
    PA23_AD5_SCL({PORT, vendor_samd::port_group_e::PGRP_A, 23});
}
}
