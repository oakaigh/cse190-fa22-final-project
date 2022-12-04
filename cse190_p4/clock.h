#pragma once

#include <samd.h>

#include "utils.h"


// SAMD21 clock abstraction
namespace vendor_samd {
// TODO
class clock_src {};

struct clock_info {
	Gclk *clk;
	uint16_t id;
};

class clock {
protected:
	struct clock_info info;

public:
	enum srcid_e : uint32_t {
		CLKSRCID_XOSC      = GCLK_GENCTRL_SRC_XOSC_Val,
		CLKSRCID_GCLKIN    = GCLK_GENCTRL_SRC_GCLKIN_Val, 
		CLKSRCID_GCLKGEN1  = GCLK_GENCTRL_SRC_GCLKGEN1_Val, 
		CLKSRCID_OSCULP32K = GCLK_GENCTRL_SRC_OSCULP32K_Val, 
		CLKSRCID_OSC32K    = GCLK_GENCTRL_SRC_OSC32K_Val, 
		CLKSRCID_XOSC32K   = GCLK_GENCTRL_SRC_XOSC32K_Val, 
		CLKSRCID_OSC8M     = GCLK_GENCTRL_SRC_OSC8M_Val,
		CLKSRCID_DFLL48M   = GCLK_GENCTRL_SRC_DFLL48M_Val, 
		CLKSRCID_FDPLL     = GCLK_GENCTRL_SRC_FDPLL_Val
	};

	static const struct src_info {
		static inline const constexpr struct preset_s {
			srcid_e i;
			std::size_t freq;
		} presets[] = {
			[CLKSRCID_XOSC     ] = { CLKSRCID_XOSC     , 0        },
			[CLKSRCID_GCLKIN   ] = { CLKSRCID_GCLKIN   , 0        },
			[CLKSRCID_GCLKGEN1 ] = { CLKSRCID_GCLKGEN1 , 0        },
			[CLKSRCID_OSCULP32K] = { CLKSRCID_OSCULP32K, 32768    },
			[CLKSRCID_OSC32K   ] = { CLKSRCID_OSC32K   , 32768    },
			[CLKSRCID_XOSC32K  ] = { CLKSRCID_XOSC32K  , 32768    },
			[CLKSRCID_OSC8M    ] = { CLKSRCID_OSC8M    , 8000000  },
			[CLKSRCID_DFLL48M  ] = { CLKSRCID_DFLL48M  , 48000000 },
			[CLKSRCID_FDPLL    ] = { CLKSRCID_FDPLL    , 0        }
		};
	} src_info;

	enum devid_e : uint16_t {
		CLKDEVID_SERCOM0 = GCLK_CLKCTRL_ID_SERCOM0_CORE_Val,
		CLKDEVID_SERCOM3 = GCLK_CLKCTRL_ID_SERCOM3_CORE_Val,
		CLKDEVID_TC3     = GCLK_CLKCTRL_ID_TCC2_TC3_Val,
		// TODO implement rest
	};

	clock(const struct clock_info &info) : info(info) {}

	clock &set_defaults() {
		this->info.clk->CLKCTRL.reg = GCLK_CLKCTRL_RESETVALUE;
		return *this;
	}

	clock &init() {
		return *this;
	}

	// TODO prescaler
	clock &set_src(enum srcid_e id, bool standby = false) {
		auto &gendiv = this->info.clk->GENDIV.bit;
		gendiv.ID = this->info.id;
		gendiv.DIV = 1;
		
		// 15.8.4 Generic Clock Generator Control
		auto &genctrl = this->info.clk->GENCTRL.bit;
		genctrl.ID = this->info.id;

		// must be disabled before switching clock source
		genctrl.GENEN = false;
		this->wait_sync();

		genctrl.SRC = id;
		// Bit 20 – DIVSEL Divide Selection
		/* If the clock source should not be divided, 
			the DIVSEL bit must be zero and the GENDIV.DIV value for the corresponding generic clock
			generator must be zero or one */
		genctrl.DIVSEL = 0;
		genctrl.RUNSTDBY = standby;

		genctrl.GENEN = true;

		this->wait_sync();
		return *this;
	}

	enum srcid_e get_src() {
		auto &genctrl = this->info.clk->GENCTRL.bit;
		genctrl.ID = this->info.id;
		return (enum srcid_e)genctrl.SRC;
	}

	// NOTE frequency in Hz
	std::size_t get_freq() {
		// TODO div!!!!!!!!!!!!!!!!!
		return this->src_info.presets[this->get_src()].freq;
	}

	clock &attach(enum devid_e id) {
		//this->info.clk->CLKCTRL.reg = GCLK_CLKCTRL_RESETVALUE;
		auto &clkctrl = this->info.clk->CLKCTRL.bit;
		clkctrl.ID = id;
		clkctrl.GEN = this->info.id;
		clkctrl.CLKEN = true;
		clkctrl.WRTLOCK = false;
		this->wait_sync();
		return *this;
	}

	clock &detach(enum devid_e id) {
		//this->info.clk->CLKCTRL.reg = GCLK_CLKCTRL_RESETVALUE;
		auto &clkctrl = this->info.clk->CLKCTRL.bit;
		clkctrl.ID = id;
		clkctrl.GEN = this->info.id;
		clkctrl.CLKEN = false;
		clkctrl.WRTLOCK = false;
		this->wait_sync();
		return *this;
	}

	clock &wait_sync() {
		while (this->info.clk->STATUS.bit.SYNCBUSY);
		return *this;
	}

	// TODO
	/*clock &enable() {
		this->info.clk->CLKCTRL.bit.CLKEN = true;
		return *this;
	}

	clock &disable() {
		this->info.clk->CLKCTRL.bit.CLKEN = false;
		return *this;
	}*/
};
}

namespace tinyzero {
namespace clocks {
inline vendor_samd::clock clk0({GCLK, GCLK_CLKCTRL_GEN_GCLK0_Val});
inline vendor_samd::clock clk2({GCLK, GCLK_CLKCTRL_GEN_GCLK2_Val});
inline vendor_samd::clock clk4({GCLK, GCLK_CLKCTRL_GEN_GCLK4_Val});
}
}
