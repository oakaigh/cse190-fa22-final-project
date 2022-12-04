#pragma once

#include <samd.h>

#include "clock.h"
#include "utils.h"


// SAMD21 timer TC handler helper
// user callback function type
typedef void samd_tc_callback_f();

// template for `TC*_Handler`s alike
// acknowledge interrupt MC0/MC1 and call respective user callbacks
#define samd_define_tc_handler(tc_name, tc_obj, cb_mc0, cb_mc1)	\
	void tc_name##_Handler() {	\
		if ((tc_obj)->INTFLAG.bit.MC0) {			\
			(tc_obj)->INTFLAG.bit.MC0 = true;		\
			if ((cb_mc0) != nullptr) (cb_mc0)();	\
		}											\
		if ((tc_obj)->INTFLAG.bit.MC1) {			\
			(tc_obj)->INTFLAG.bit.MC1 = true;		\
			if ((cb_mc1) != nullptr) (cb_mc1)();	\
		}											\
	}

// assignable callback definitions
// TC3
inline samd_tc_callback_f *samd_tc_cb_TC3_mc0 = nullptr;
inline samd_tc_callback_f *samd_tc_cb_TC3_mc1 = nullptr;
samd_define_tc_handler(
	TC3, &TC3->COUNT16,
	samd_tc_cb_TC3_mc0, samd_tc_cb_TC3_mc1
);


// SAMD21 timer abstraction
// implementation partially based on @khoih-prog's TimerInterrupt_Generic library
// ref https://github.com/khoih-prog/TimerInterrupt_Generic
// see https://github.com/khoih-prog/TimerInterrupt_Generic/blob/main/src/SAMDTimerInterrupt_Generic.h
namespace vendor_samd {
struct timer_info {
	Tc *tc;							// base
	enum clock::devid_e clkid;		// clock ID
	IRQn_Type irqn;					// IRQ number
	samd_tc_callback_f *&cb_ref;	// callback reference
};

class timer {
protected:
	struct timer_info info;

	clock *clk = nullptr;

	// NOTE TC timers in different modes share the same set of registers
	// TODO 8 32 bit support
	static constexpr TcCount16 *
	common_of(Tc *s) { return &s->COUNT16; }

	constexpr std::size_t max_count() {
		// TODO 8 32 bit support
		return std::numeric_limits<
			decltype(this->info.tc->COUNT16.COUNT.bit.COUNT)
		>::max();
	}

public:
	enum mode_e : uint16_t {
		MODE_COUNT8BIT  = TC_CTRLA_MODE_COUNT8_Val,
		MODE_COUNT16BIT = TC_CTRLA_MODE_COUNT16_Val,
		MODE_COUNT32BIT = TC_CTRLA_MODE_COUNT32_Val
	};

	enum prescaler_e : std::ptrdiff_t {
		PRESCALER_INVALID = -1,
		PRESCALER_DIV1    = TC_CTRLA_PRESCALER_DIV1_Val,
		PRESCALER_MIN     = PRESCALER_DIV1,
		PRESCALER_DIV2    = TC_CTRLA_PRESCALER_DIV2_Val,
		PRESCALER_DIV4    = TC_CTRLA_PRESCALER_DIV4_Val,
		PRESCALER_DIV8    = TC_CTRLA_PRESCALER_DIV8_Val,
		PRESCALER_DIV16   = TC_CTRLA_PRESCALER_DIV16_Val,
		PRESCALER_DIV64   = TC_CTRLA_PRESCALER_DIV64_Val,
		PRESCALER_DIV256  = TC_CTRLA_PRESCALER_DIV256_Val,
		PRESCALER_DIV1024 = TC_CTRLA_PRESCALER_DIV1024_Val,
		PRESCALER_MAX     = PRESCALER_DIV1024
	};

	static const class prescaler_info_c {
	public:
		static inline const constexpr struct preset_s {
			prescaler_e i;
			std::size_t val;
		} presets[] = {
			[PRESCALER_DIV1   ] = { PRESCALER_DIV1   , 1    },
			[PRESCALER_DIV2   ] = { PRESCALER_DIV2   , 2    },
			[PRESCALER_DIV4   ] = { PRESCALER_DIV4   , 4    },
			[PRESCALER_DIV8   ] = { PRESCALER_DIV8   , 8    },
			[PRESCALER_DIV16  ] = { PRESCALER_DIV16  , 16   },
			[PRESCALER_DIV64  ] = { PRESCALER_DIV64  , 64   },
			[PRESCALER_DIV256 ] = { PRESCALER_DIV256 , 256  },
			[PRESCALER_DIV1024] = { PRESCALER_DIV1024, 1024 }
		};

		constexpr bool in_range(const double &val) const {
			return utils::in_range(val,
				(double)this->presets[prescaler_e::PRESCALER_MIN].val,
				(double)this->presets[prescaler_e::PRESCALER_MAX].val
			);
		};

		prescaler_e query(const double &val) const {
			// range check
			if (!this->in_range(val))
				return prescaler_e::PRESCALER_INVALID;

			// TODO use sliding window
			for (const auto &p : this->presets)
				if (p.val >= val)
					return p.i;

			return prescaler_e::PRESCALER_INVALID;
		}
	} prescaler_info;

	timer(const struct timer_info &info) : info(info) {}

	// TODO unique pointer
	timer &init(clock *clk) {
		//this->reset();

		this->clk = clk;

		// TODO detach?
		this->clk->attach(this->info.clkid);

		// TODO 8 32 bit support
		this->set_mode(mode_e::MODE_COUNT16BIT);

		return *this;
	}

	// TODO
	enum status_e {
		STATUS_SUCCESS = 0,
		STATUS_INVLARG
	};

	timer &reset() {
		common_of(this->info.tc)->CTRLA.bit.SWRST = true;
		while (common_of(this->info.tc)->CTRLA.bit.SWRST);
		return *this;
	}

	timer &enable(bool standby = false) {
		common_of(this->info.tc)->CTRLA.bit.ENABLE = true;
		common_of(this->info.tc)->CTRLA.bit.RUNSTDBY = standby;
		this->wait_sync();
		return *this;
	}

	timer &disable() {
		common_of(this->info.tc)->CTRLA.bit.ENABLE = false;
		this->wait_sync();
		return *this;
	}

	timer &wait_sync() {
		while (common_of(this->info.tc)->STATUS.bit.SYNCBUSY);
		return *this;
	}

	// TODO
	timer &set_mode(mode_e m) {
		// TODO
		this->disable();

		common_of(this->info.tc)->CTRLA.bit.MODE = m;
		// TODO
		common_of(this->info.tc)->CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_MFRQ_Val;
		this->wait_sync();
		return *this;
	}

	status_e set_prescaler(enum prescaler_e p) {
		if (p == prescaler_e::PRESCALER_INVALID)
			return status_e::STATUS_INVLARG;

		common_of(this->info.tc)->CTRLA.bit.PRESCALER = (uint16_t)p;
		this->wait_sync();
		return status_e::STATUS_SUCCESS;
	}

	enum prescaler_e set_prescaler(const double &n) {
		auto res = this->prescaler_info.query((double)n);
		if (res == prescaler_e::PRESCALER_INVALID)
			return res;

		if (this->set_prescaler(res) != status_e::STATUS_SUCCESS)
			return prescaler_e::PRESCALER_INVALID;
		return res;
	}

	status_e set_compare(std::size_t val, bool enhanced = false) {
		// TODO 8 32 bit support cc channel
		auto &t = this->info.tc->COUNT16;

		// range check
		if (val > this->max_count())
			return status_e::STATUS_INVLARG;

		// enhanced counter handling
		/* Make sure the count is in a proportional position to where it was
			to prevent any jitter or disconnect when changing the compare value.
				- @EHbtj @khoih-prog */
		if (enhanced) {
			t.COUNT.bit.COUNT = utils::linear_map<std::size_t>(
				t.COUNT.bit.COUNT, 0,
				t.CC[0].bit.CC, 0,
				val
			);
		}
		t.CC[0].bit.CC = val;

		this->wait_sync();

		return status_e::STATUS_SUCCESS;
	}

	// interval calc priority setting
	enum intvlprior_e {
		INTVLPRIOR_ACC,  // prefer accuracy
		INTVLPRIOR_RANGE // prefer range
	};
	
	// NOTE interval in sec
	status_e set_interval(
		const double &intvl, 
		enum intvlprior_e calc_prior = INTVLPRIOR_ACC,
		bool enhanced = false
	) {
		/* period_val = counter_val
			/ (increment_speed = clock_speed / prescaler) */
		class eqn_c {
		protected:
			double freq_clock;
			double period;

		public:
			eqn_c(const double &f, const double &p) 
				: freq_clock(f), period(p) {}

			constexpr double prescaler(const double &count) const {
				return this->freq_clock / (count / this->period);
			}

			constexpr double count(const double &prescaler) const {
				return this->freq_clock / (prescaler / this->period);
			}
		};

		enum status_e s = status_e::STATUS_INVLARG;

		eqn_c eqn(this->clk->get_freq(), intvl);

		enum prescaler_e prescaler = prescaler_e::PRESCALER_INVALID;
		switch (calc_prior) {
		case intvlprior_e::INTVLPRIOR_ACC: {
			prescaler = this->prescaler_info.query(
				eqn.prescaler(this->max_count())
			);
		} break;
		case intvlprior_e::INTVLPRIOR_RANGE: {
			prescaler = this->PRESCALER_MAX;
		} break;
		default:
			return s;
		}

		// TODO rm
		/*while (true) {
			//SerialUSB.println(eqn.prescaler(compr_val));
			//SerialUSB.println(
		//		this->prescaler_info.presets[prescaler].val
			//);
			SerialUSB.println(eqn.count(
				this->prescaler_info.presets[prescaler].val
			));
			delay(2000);
		}*/

		s = this->set_prescaler(prescaler);
		if (s != STATUS_SUCCESS)
			return s;

		s = this->set_compare(
			eqn.count(
				this->prescaler_info.presets[prescaler].val
			)
		);
		if (s != STATUS_SUCCESS)
			return s;

		return s;
	}

	// NOTE interval in sec
	template <typename cb_type>
	timer &listen(cb_type cb) {
		this->info.cb_ref = cb;

		// TODO events
		common_of(this->info.tc)->INTENSET.reg = TC_INTENSET_RESETVALUE;

		if (this->info.cb_ref == nullptr) {
			NVIC_DisableIRQ(this->info.irqn);
			common_of(this->info.tc)->INTENSET.bit.MC0 = false;
			return *this;
		}

		// TODO channel id
		common_of(this->info.tc)->INTENSET.bit.MC0 = true;
		NVIC_EnableIRQ(this->info.irqn);
		return *this;
	}
};

// available timers
namespace timers {
inline vendor_samd::timer tc3({
	TC3,
	vendor_samd::clock::devid_e::CLKDEVID_TC3,
	TC3_IRQn,
	samd_tc_cb_TC3_mc0
});
}
}


namespace tinyzero {
namespace timers = vendor_samd::timers;
}
