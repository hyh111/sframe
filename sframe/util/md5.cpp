
#include "memory.h"
#include "md5.h"

using namespace sframe;

MD5::MD5()
{
    memset(_result_str, 0, 33);
}

MD5::MD5(const std::string & input)
{
    memset(_result_str, 0, 33);
    Digest((const unsigned char *)input.c_str(), (uint32_t)input.length());
}

MD5::MD5(const char * input, uint32_t len)
{
    memset(_result_str, 0, 33);
    Digest((const unsigned char *)input, len);
}

// 计算MD5摘要
void MD5::Digest(const unsigned char * str, uint32_t len)
{
    _result[0] = A;
    _result[1] = B;
    _result[2] = C;
    _result[3] = D;

    uint32_t group_num = len / 64;

    // 以每组64字节（512位）分组处理
    for (uint32_t i = 0; i < group_num; i++)
    {
        TransGroup(str + i * 64);
    }

    // 还有多少数据没有处理
    uint32_t surplus = len % 64;
    unsigned char buf[64];

    if (surplus < 56)
    {
        // 将剩余的数据拷到buf中
        for (uint32_t i = 0; i < surplus; i++)
        {
            buf[i] = str[len - surplus + i];
        }

        buf[surplus] = 0x80;
        for (uint32_t i = 1; i < 56 - surplus; i++)
        {
            buf[surplus + i] = 0x00;
        }

        // 加入数据长度
        uint64_t in_len = (uint64_t)(len << 3);
        for (int i = 0; i < 8; i++){
            buf[56 + i] = (char)(in_len & 0xFFL);
            in_len = in_len >> 8;
        }

        // 处理组
        TransGroup(buf);
    }
    else
    {
        // 拷贝数据
        for (uint32_t i = 0; i < surplus; i++)
        {
            buf[i] = str[len - surplus + i];
        }
        // 填充
        buf[surplus] = 0x80;
        for (int i = surplus + 1; i < 64; i++)
        {
            buf[i] = 0x00;
        }
        // 处理分组
        TransGroup(buf);

        // 再次填充56个0
        for (int i = 0; i < 56; i++)
        {
            buf[i] = 0x00;
        }

        // 填入长度
        uint64_t in_len = (uint64_t)(len << 3);
        for (int i = 0; i < 8; i++){
            buf[56 + i] = (char)(in_len & 0xFFL);
            in_len = in_len >> 8;
        }

        // 处理组
        TransGroup(buf);
    }
}

// 获取结果
const char * MD5::GetResult()
{
    if (_result_str[0] == '\0')
    {
        for (int i = 0; i < 4; i++)
        {
            sprintf(_result_str + i * 8, "%02x", _result[i] & 0x000000ff);
            sprintf(_result_str + i * 8 + 2, "%02x", (_result[i] >> 8) & 0x000000ff);
            sprintf(_result_str + i * 8 + 4, "%02x", (_result[i] >> 16) & 0x000000ff);
            sprintf(_result_str + i * 8 + 6, "%02x", (_result[i] >> 24) & 0x000000ff);
        }
    }

    return _result_str;
}




// 处理一组数据(默认长度为64字节)
void MD5::TransGroup(const unsigned char * groups)
{
    uint32_t a = _result[0], b = _result[1], c = _result[2], d = _result[3];

    uint32_t groupdata[16];

    for (int i = 0; i < 16; i++)
    {
        groupdata[i] = ((uint32_t)groups[i * 4] |
            ((uint32_t)(groups[i * 4 + 1]) << 8) |
            ((uint32_t)(groups[i * 4 + 2]) << 16) |
            ((uint32_t)(groups[i * 4 + 3]) << 24));
    }

    a = FF(a, b, c, d, groupdata[0], S11, 0xd76aa478L); /* 1 */
    d = FF(d, a, b, c, groupdata[1], S12, 0xe8c7b756L); /* 2 */
    c = FF(c, d, a, b, groupdata[2], S13, 0x242070dbL); /* 3 */
    b = FF(b, c, d, a, groupdata[3], S14, 0xc1bdceeeL); /* 4 */
    a = FF(a, b, c, d, groupdata[4], S11, 0xf57c0fafL); /* 5 */
    d = FF(d, a, b, c, groupdata[5], S12, 0x4787c62aL); /* 6 */
    c = FF(c, d, a, b, groupdata[6], S13, 0xa8304613L); /* 7 */
    b = FF(b, c, d, a, groupdata[7], S14, 0xfd469501L); /* 8 */
    a = FF(a, b, c, d, groupdata[8], S11, 0x698098d8L); /* 9 */
    d = FF(d, a, b, c, groupdata[9], S12, 0x8b44f7afL); /* 10 */
    c = FF(c, d, a, b, groupdata[10], S13, 0xffff5bb1L); /* 11 */
    b = FF(b, c, d, a, groupdata[11], S14, 0x895cd7beL); /* 12 */
    a = FF(a, b, c, d, groupdata[12], S11, 0x6b901122L); /* 13 */
    d = FF(d, a, b, c, groupdata[13], S12, 0xfd987193L); /* 14 */
    c = FF(c, d, a, b, groupdata[14], S13, 0xa679438eL); /* 15 */
    b = FF(b, c, d, a, groupdata[15], S14, 0x49b40821L); /* 16 */

    /*第二轮*/
    a = GG(a, b, c, d, groupdata[1], S21, 0xf61e2562L); /* 17 */
    d = GG(d, a, b, c, groupdata[6], S22, 0xc040b340L); /* 18 */
    c = GG(c, d, a, b, groupdata[11], S23, 0x265e5a51L); /* 19 */
    b = GG(b, c, d, a, groupdata[0], S24, 0xe9b6c7aaL); /* 20 */
    a = GG(a, b, c, d, groupdata[5], S21, 0xd62f105dL); /* 21 */
    d = GG(d, a, b, c, groupdata[10], S22, 0x2441453L); /* 22 */
    c = GG(c, d, a, b, groupdata[15], S23, 0xd8a1e681L); /* 23 */
    b = GG(b, c, d, a, groupdata[4], S24, 0xe7d3fbc8L); /* 24 */
    a = GG(a, b, c, d, groupdata[9], S21, 0x21e1cde6L); /* 25 */
    d = GG(d, a, b, c, groupdata[14], S22, 0xc33707d6L); /* 26 */
    c = GG(c, d, a, b, groupdata[3], S23, 0xf4d50d87L); /* 27 */
    b = GG(b, c, d, a, groupdata[8], S24, 0x455a14edL); /* 28 */
    a = GG(a, b, c, d, groupdata[13], S21, 0xa9e3e905L); /* 29 */
    d = GG(d, a, b, c, groupdata[2], S22, 0xfcefa3f8L); /* 30 */
    c = GG(c, d, a, b, groupdata[7], S23, 0x676f02d9L); /* 31 */
    b = GG(b, c, d, a, groupdata[12], S24, 0x8d2a4c8aL); /* 32 */

    /*第三轮*/
    a = HH(a, b, c, d, groupdata[5], S31, 0xfffa3942L); /* 33 */
    d = HH(d, a, b, c, groupdata[8], S32, 0x8771f681L); /* 34 */
    c = HH(c, d, a, b, groupdata[11], S33, 0x6d9d6122L); /* 35 */
    b = HH(b, c, d, a, groupdata[14], S34, 0xfde5380cL); /* 36 */
    a = HH(a, b, c, d, groupdata[1], S31, 0xa4beea44L); /* 37 */
    d = HH(d, a, b, c, groupdata[4], S32, 0x4bdecfa9L); /* 38 */
    c = HH(c, d, a, b, groupdata[7], S33, 0xf6bb4b60L); /* 39 */
    b = HH(b, c, d, a, groupdata[10], S34, 0xbebfbc70L); /* 40 */
    a = HH(a, b, c, d, groupdata[13], S31, 0x289b7ec6L); /* 41 */
    d = HH(d, a, b, c, groupdata[0], S32, 0xeaa127faL); /* 42 */
    c = HH(c, d, a, b, groupdata[3], S33, 0xd4ef3085L); /* 43 */
    b = HH(b, c, d, a, groupdata[6], S34, 0x4881d05L); /* 44 */
    a = HH(a, b, c, d, groupdata[9], S31, 0xd9d4d039L); /* 45 */
    d = HH(d, a, b, c, groupdata[12], S32, 0xe6db99e5L); /* 46 */
    c = HH(c, d, a, b, groupdata[15], S33, 0x1fa27cf8L); /* 47 */
    b = HH(b, c, d, a, groupdata[2], S34, 0xc4ac5665L); /* 48 */

    /*第四轮*/
    a = II(a, b, c, d, groupdata[0], S41, 0xf4292244L); /* 49 */
    d = II(d, a, b, c, groupdata[7], S42, 0x432aff97L); /* 50 */
    c = II(c, d, a, b, groupdata[14], S43, 0xab9423a7L); /* 51 */
    b = II(b, c, d, a, groupdata[5], S44, 0xfc93a039L); /* 52 */
    a = II(a, b, c, d, groupdata[12], S41, 0x655b59c3L); /* 53 */
    d = II(d, a, b, c, groupdata[3], S42, 0x8f0ccc92L); /* 54 */
    c = II(c, d, a, b, groupdata[10], S43, 0xffeff47dL); /* 55 */
    b = II(b, c, d, a, groupdata[1], S44, 0x85845dd1L); /* 56 */
    a = II(a, b, c, d, groupdata[8], S41, 0x6fa87e4fL); /* 57 */
    d = II(d, a, b, c, groupdata[15], S42, 0xfe2ce6e0L); /* 58 */
    c = II(c, d, a, b, groupdata[6], S43, 0xa3014314L); /* 59 */
    b = II(b, c, d, a, groupdata[13], S44, 0x4e0811a1L); /* 60 */
    a = II(a, b, c, d, groupdata[4], S41, 0xf7537e82L); /* 61 */
    d = II(d, a, b, c, groupdata[11], S42, 0xbd3af235L); /* 62 */
    c = II(c, d, a, b, groupdata[2], S43, 0x2ad7d2bbL); /* 63 */
    b = II(b, c, d, a, groupdata[9], S44, 0xeb86d391L); /* 64 */

    /*加入到之前计算的结果当中*/
    _result[0] += a;
    _result[1] += b;
    _result[2] += c;
    _result[3] += d;
}