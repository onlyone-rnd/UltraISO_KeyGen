
#define shift(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~(z))))

#define FF(a ,b ,c ,d ,Mj ,s ,ti)  a = b + (shift((a + F(b, c, d) + Mj + ti) , s))
#define GG(a, b, c, d, Mj, s, ti)  a = b + (shift((a + G(b, c, d) + Mj + ti) , s))
#define HH(a, b, c, d, Mj, s, ti)  a = b + (shift((a + H(b, c, d) + Mj + ti) , s))
#define II(a, b, c, d, Mj, s, ti)  a = b + (shift((a + I(b, c, d) + Mj + ti) , s))

#define A 0x67452301u
#define B 0xefcdab89u
#define C 0x98badcfeu
#define D 0x10325476u

inline ulong md5_512(__private uint* msg)
{
    uint a = A, b = B, c = C, d = D;

    FF(a, b, c, d, msg[0], 7, 0xd76aa478u);
    FF(d, a, b, c, msg[1], 12, 0xe8c7b756u);
    FF(c, d, a, b, msg[2], 17, 0x242070dbu);
    FF(b, c, d, a, msg[3], 22, 0xc1bdceeeu);
    FF(a, b, c, d, msg[4], 7, 0xf57c0fafu);
    FF(d, a, b, c, msg[5], 12, 0x4787c62au);
    FF(c, d, a, b, msg[6], 17, 0xa8304613u);
    FF(b, c, d, a, msg[7], 22, 0xfd469501u);
    FF(a, b, c, d, msg[8], 7, 0x698098d8u);
    FF(d, a, b, c, msg[9], 12, 0x8b44f7afu);
    FF(c, d, a, b, msg[10], 17, 0xffff5bb1u);
    FF(b, c, d, a, msg[11], 22, 0x895cd7beu);
    FF(a, b, c, d, msg[12], 7, 0x6b901122u);
    FF(d, a, b, c, msg[13], 12, 0xfd987193u);
    FF(c, d, a, b, msg[14], 17, 0xa679438eu);
    FF(b, c, d, a, msg[15], 22, 0x49b40821u);

    GG(a, b, c, d, msg[1], 5, 0xf61e2562u);
    GG(d, a, b, c, msg[6], 9, 0xc040b340u);
    GG(c, d, a, b, msg[11], 14, 0x265e5a51u);
    GG(b, c, d, a, msg[0], 20, 0xe9b6c7aau);
    GG(a, b, c, d, msg[5], 5, 0xd62f105du);
    GG(d, a, b, c, msg[10], 9, 0x02441453u);
    GG(c, d, a, b, msg[15], 14, 0xd8a1e681u);
    GG(b, c, d, a, msg[4], 20, 0xe7d3fbc8u);
    GG(a, b, c, d, msg[9], 5, 0x21e1cde6u);
    GG(d, a, b, c, msg[14], 9, 0xc33707d6u);
    GG(c, d, a, b, msg[3], 14, 0xf4d50d87u);
    GG(b, c, d, a, msg[8], 20, 0x455a14edu);
    GG(a, b, c, d, msg[13], 5, 0xa9e3e905u);
    GG(d, a, b, c, msg[2], 9, 0xfcefa3f8u);
    GG(c, d, a, b, msg[7], 14, 0x676f02d9u);
    GG(b, c, d, a, msg[12], 20, 0x8d2a4c8au);

    HH(a, b, c, d, msg[5], 4, 0xfffa3942u);
    HH(d, a, b, c, msg[8], 11, 0x8771f681u);
    HH(c, d, a, b, msg[11], 16, 0x6d9d6122u);
    HH(b, c, d, a, msg[14], 23, 0xfde5380cu);
    HH(a, b, c, d, msg[1], 4, 0xa4beea44u);
    HH(d, a, b, c, msg[4], 11, 0x4bdecfa9u);
    HH(c, d, a, b, msg[7], 16, 0xf6bb4b60u);
    HH(b, c, d, a, msg[10], 23, 0xbebfbc70u);
    HH(a, b, c, d, msg[13], 4, 0x289b7ec6u);
    HH(d, a, b, c, msg[0], 11, 0xeaa127fau);
    HH(c, d, a, b, msg[3], 16, 0xd4ef3085u);
    HH(b, c, d, a, msg[6], 23, 0x04881d05u);
    HH(a, b, c, d, msg[9], 4, 0xd9d4d039u);
    HH(d, a, b, c, msg[12], 11, 0xe6db99e5u);
    HH(c, d, a, b, msg[15], 16, 0x1fa27cf8u);
    HH(b, c, d, a, msg[2], 23, 0xc4ac5665u);

    II(a, b, c, d, msg[0], 6, 0xf4292244u);
    II(d, a, b, c, msg[7], 10, 0x432aff97u);
    II(c, d, a, b, msg[14], 15, 0xab9423a7u);
    II(b, c, d, a, msg[5], 21, 0xfc93a039u);
    II(a, b, c, d, msg[12], 6, 0x655b59c3u);
    II(d, a, b, c, msg[3], 10, 0x8f0ccc92u);
    II(c, d, a, b, msg[10], 15, 0xffeff47du);
    II(b, c, d, a, msg[1], 21, 0x85845dd1u);
    II(a, b, c, d, msg[8], 6, 0x6fa87e4fu);
    II(d, a, b, c, msg[15], 10, 0xfe2ce6e0u);
    II(c, d, a, b, msg[6], 15, 0xa3014314u);
    II(b, c, d, a, msg[13], 21, 0x4e0811a1u);
    II(a, b, c, d, msg[4], 6, 0xf7537e82u);
    II(d, a, b, c, msg[11], 10, 0xbd3af235u);
    II(c, d, a, b, msg[2], 15, 0x2ad7d2bbu);
    II(b, c, d, a, msg[9], 21, 0xeb86d391u);

    a = a + A;
    b = b + B;
    c = c + C;
    d = d + D;

    return (((ulong)b) << 32) | (ulong)a;
}

inline ulong add_mod(ulong a, ulong b, ulong m)
{
    ulong s = a + b;

    ulong overflow = (s < a) ? (ulong)1 : (ulong)0;
    ulong ge_m     = (s >= m) ? (ulong)1 : (ulong)0;

    s -= ge_m * m;

    s += overflow * (ulong)0x58F0709D5591682FUL;

    return s;
}

inline ulong mul_mod(ulong a, ulong b, ulong m)
{
    ulong res = 0;
    ulong tmp = a % m;
    while (b) {
        if (b & 1)
            res = add_mod(res, tmp, m);
        b >>= 1;
        tmp = add_mod(tmp, tmp, m);
    }
    return res;
}

inline ulong exp_mod(ulong c, ulong e, ulong n)
{
    c %= n;
    ulong result = (ulong)1;
    while (e > 0) {
        if (e & 1)
            result = mul_mod(result, c, n);
        e >>= 1;
        c = mul_mod(c, c, n);
    }
    return result;
}

__constant char hex2str_map[] = "0123456789ABCDEF";

__kernel void KGkernel(__global ulong* data,
                       __global const char* msg_,
                       int len)
{
    size_t i = get_global_id(0);
    ulong pat = data[i];

    ulong num = exp_mod(pat, (ulong)0x58C5D3F7u, (ulong)0xF513E783u);
    pat       = exp_mod(pat, (ulong)0xAC3A102Bu, (ulong)0xAE818F1Bu);

    num = mul_mod(num, (ulong)0x5C4AF104DA37C96DUL, (ulong)0xA70F8F62AA6E97D1UL);
    pat = mul_mod(pat, (ulong)0x4AC49E5DD036CE65UL, (ulong)0xA70F8F62AA6E97D1UL);
    num = add_mod(num, pat, (ulong)0xA70F8F62AA6E97D1UL);

    uchar msg[64];
    #pragma unroll 16
    for (int k = 0; k < 64; ++k)
        msg[k] = (uchar)0;

    #pragma unroll 16
    for (int k = 0; k < len; ++k)
        msg[k] = (uchar)msg_[k];

    *(__private uint*)(&msg[56]) = (uint)((len + 16) * 8);
    msg[len + 16] = (uchar)0x80;

    (&msg[len])[0]  = hex2str_map[(num >> (15 * 4)) & 0xF];
    (&msg[len])[1]  = hex2str_map[(num >> (14 * 4)) & 0xF];
    (&msg[len])[2]  = hex2str_map[(num >> (13 * 4)) & 0xF];
    (&msg[len])[3]  = hex2str_map[(num >> (12 * 4)) & 0xF];
    (&msg[len])[4]  = hex2str_map[(num >> (11 * 4)) & 0xF];
    (&msg[len])[5]  = hex2str_map[(num >> (10 * 4)) & 0xF];
    (&msg[len])[6]  = hex2str_map[(num >>  (9 * 4)) & 0xF];
    (&msg[len])[7]  = hex2str_map[(num >>  (8 * 4)) & 0xF];
    (&msg[len])[8]  = hex2str_map[(num >>  (7 * 4)) & 0xF];
    (&msg[len])[9]  = hex2str_map[(num >>  (6 * 4)) & 0xF];
    (&msg[len])[10] = hex2str_map[(num >>  (5 * 4)) & 0xF];
    (&msg[len])[11] = hex2str_map[(num >>  (4 * 4)) & 0xF];
    (&msg[len])[12] = hex2str_map[(num >>  (3 * 4)) & 0xF];
    (&msg[len])[13] = hex2str_map[(num >>  (2 * 4)) & 0xF];
    (&msg[len])[14] = hex2str_map[(num >>  (1 * 4)) & 0xF];
    (&msg[len])[15] = hex2str_map[(num >>  (0 * 4)) & 0xF];

    ulong res = md5_512((__private uint*)msg);
    data[i] = res;
}
