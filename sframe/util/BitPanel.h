
#ifndef SFRAME_PANEL_H
#define SFRAME_PANEL_H

#include <assert.h>
#include <inttypes.h>
#include <type_traits>

namespace sframe {

template<int32_t Bit_Count>
struct BitCountToByteCount1
{
	static const int32_t kByteCount = Bit_Count / 8 + 1;
};

template<int32_t Bit_Count>
struct BitCountToByteCount2
{
	static const int32_t kByteCount = Bit_Count / 8;
};

static const uint8_t kByteBitMask_Positive[8] = {
	0x01, 0x02, 0x04, 0x08,0x10, 0x20 ,0x40, 0x80
};

static const uint8_t kByteBitMask_Negative[8] = {
	0xfe, 0xfd, 0xfb, 0xf7,0xef, 0xdf ,0xbf, 0x7f
};

// 封装按位运算的开关(位索引从0开始)
template<int32_t Bit_Count>
class BitPanel
{
	static const int32_t kByteArraySize = std::conditional<
		Bit_Count % 8 == 0 ? false : true,
		BitCountToByteCount1<Bit_Count>,
		BitCountToByteCount2<Bit_Count>>::type::kByteCount;

public:
	BitPanel(bool set_all = false)
	{
		static_assert(Bit_Count > 0, "BitPanel Count must big than 0");
		uint8_t set_byte = set_all ? 0xff : 0;
		memset(_arr, set_byte, sizeof(_arr));
	}

	bool Test(int32_t index)
	{
		int32_t arr_index = index / 8;
		int32_t bit_num = index % 8;
		assert(arr_index < kByteArraySize && bit_num < 8);
		return (_arr[arr_index] & kByteBitMask_Positive[bit_num]) > 0;
	}

	void Set(int32_t index, bool on)
	{
		int32_t arr_index = index / 8;
		int32_t bit_num = index % 8;
		assert(arr_index < kByteArraySize && bit_num < 8);
		on ? _arr[arr_index] |= kByteBitMask_Positive[bit_num] : _arr[arr_index] &= kByteBitMask_Negative[bit_num];
	}

private:
	uint8_t _arr[kByteArraySize];
};

}

#endif