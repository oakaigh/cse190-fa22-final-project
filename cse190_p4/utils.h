#pragma once

#include "arduino.h"

#include <cstddef>
#include <algorithm>
#include <limits>
#include <climits>
#include <cmath>
#include <cstring>


#define sizeof_bit(n)	\
	(sizeof(n) * __CHAR_BIT__)

#define bit_op(n, pos, op)	((n) op (1L << (pos)))
#define bit_set(n, pos)	(bit_op((n), (pos), |=))
#define bit_clear(n, pos)	(bit_op((n), (pos), &=~))
#define bit_read(n, pos)	(bit_op((n), (pos), &))
#define bit_write(n, pos, v)	\
	(v) ? bit_set((n), (pos)) : bit_clear((n), (pos))
#define bit_toggle(n, pos)	(bit_op((n), (pos), ^=))

// credit @Motti
template<class T, size_t N>
constexpr size_t size(T (&)[N]) { return N; }

template <typename val_type>
struct overflow_protected_max {
	val_type val;

	overflow_protected_max() {}

	overflow_protected_max(val_type v) : val(v) {}

	operator val_type&() { return this->val; }

	template <typename rhs_type>
	overflow_protected_max
	&operator=(const rhs_type &rhs)
	{
		this->val = std::min(
			rhs,
			(rhs_type)std::numeric_limits<val_type>::max()
		);
		return *this;
	}

	template <typename rhs_type>
	overflow_protected_max
	operator+(const rhs_type &rhs)
	{
		val_type val_n;
		if (__builtin_add_overflow(this->val, rhs, &val_n))
			return *this;
		return val_n;
	}

	template <typename rhs_type>
	overflow_protected_max
	&operator+=(const rhs_type &rhs)
	{
		this->val = this->operator+(rhs);
		return *this;
	}

	overflow_protected_max
	&operator++()
	{
		this->operator+=(1);
		return *this;
	}

	overflow_protected_max
	operator++(int)
	{
		overflow_protected_max res(*this);
		this->operator++();
		return res;
	}

	// TODO rest of the operators
};

namespace utils {
// improved version of arduino's `map` function
// ref packages/TinyCircuits/hardware/samd/1.1.0/cores/arduino/WMath.cpp
template <typename num_type>
constexpr num_type linear_map(
	const num_type &x,
	const num_type &in_min, const num_type &in_max,
	const num_type &out_min, const num_type &out_max
) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

template <typename num_type>
constexpr bool in_range(
	const num_type &x,
	const num_type &low, const num_type &high
) { return low <= x && x <= high; }	

inline std::ptrdiff_t bytes_compare(
	const char *lhs, std::size_t len_lhs, 
	const char *rhs, std::size_t len_rhs
) {
	return std::strncmp(lhs, rhs, std::min(len_lhs, len_rhs));
}

inline bool bytes_equal(
	const char *lhs, std::size_t len_lhs, 
	const char *rhs, std::size_t len_rhs
) { return bytes_compare(lhs, len_lhs, rhs, len_rhs) == 0; }

}
