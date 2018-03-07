
#ifndef SFRAME__MD5_H
#define SFRAME__MD5_H

#include <stdint.h>
#include <string>

#define MD5F(X,Y,Z) (((X) & (Y)) | ((~(X)) & (Z)))
#define MD5G(X,Y,Z) (((X) & (Z)) | ((Y) & (~(Z))))
#define MD5H(X,Y,Z) ((X) ^ (Y) ^ (Z))
#define MD5I(X,Y,Z) ((Y) ^ ((X) | (~(Z))))

namespace sframe {

// MD5计算
class MD5
{
public:
	MD5();
	MD5(const std::string & input);
	MD5(const char * input, uint32_t len);

	// 计算MD5摘要
	void Digest(const unsigned char * str, uint32_t len);
	void Digest(const std::string & str)
	{
		Digest((const unsigned char *)str.c_str(), (uint32_t)str.length());
	}

	// 获取结果
	const char * GetResult();

private:
	// 处理一组数据(默认长度为64字节)
	void TransGroup(const unsigned char * groups);

	uint32_t FF(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s,
		uint32_t ac) {
		a += (MD5F(b, c, d) & 0xFFFFFFFFL) + x + ac;
		a = ((a & 0xFFFFFFFFL) << s) | ((a & 0xFFFFFFFFL) >> (32 - s));
		a += b;
		return (a & 0xFFFFFFFFL);
	}

	uint32_t GG(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s,
		uint32_t ac) {
		a += (MD5G(b, c, d) & 0xFFFFFFFFL) + x + ac;
		a = ((a & 0xFFFFFFFFL) << s) | ((a & 0xFFFFFFFFL) >> (32 - s));
		a += b;
		return (a & 0xFFFFFFFFL);
	}

	uint32_t HH(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s,
		uint32_t ac) {
		a += (MD5H(b, c, d) & 0xFFFFFFFFL) + x + ac;
		a = ((a & 0xFFFFFFFFL) << s) | ((a & 0xFFFFFFFFL) >> (32 - s));
		a += b;
		return (a & 0xFFFFFFFFL);
	}

	uint32_t II(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s,
		uint32_t ac) {
		a += (MD5I(b, c, d) & 0xFFFFFFFFL) + x + ac;
		a = ((a & 0xFFFFFFFFL) << s) | ((a & 0xFFFFFFFFL) >> (32 - s));
		a += b;
		return (a & 0xFFFFFFFFL);
	}


private:
	uint32_t _result[4];
	char _result_str[33];

private:
	// 四轮运算用到的
	static const uint32_t S11 = 7;
	static const uint32_t S12 = 12;
	static const uint32_t S13 = 17;
	static const uint32_t S14 = 22;
	static const uint32_t S21 = 5;
	static const uint32_t S22 = 9;
	static const uint32_t S23 = 14;
	static const uint32_t S24 = 20;
	static const uint32_t S31 = 4;
	static const uint32_t S32 = 11;
	static const uint32_t S33 = 16;
	static const uint32_t S34 = 23;
	static const uint32_t S41 = 6;
	static const uint32_t S42 = 10;
	static const uint32_t S43 = 15;
	static const uint32_t S44 = 21;
	// 标准幻数
	static const uint32_t A = 0x67452301;
	static const uint32_t B = 0xefcdab89L;
	static const uint32_t C = 0x98badcfeL;
	static const uint32_t D = 0x10325476L;

};

}

#endif