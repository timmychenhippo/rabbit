#include "rabbit.h"
#include <iostream>
#include <bitset>
// Generic left rotate.
#define leftRotate(a, bits) ((a << (bits)) | (a >> (32 - (bits))))

static inline uint32_t G_func(uint32_t x)
{

    uint64_t z = x;
    z *= x;
    return static_cast<uint32_t>((z >> 32) ^ z);

}
static inline void clean(void* dest, size_t size)
{
    // Force the use of volatile so that we actually clear the memory.
    // Otherwise the compiler might optimise the entire contents of this
    // function away, which will not be secure.
    volatile uint8_t* d = (volatile uint8_t*)dest;
    while (size > 0) {
        *d++ = 0;
        --size;
    }
}

static inline void u32t8le(const uint32_t v, uint8_t p[4]) {
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

static inline uint32_t u8t32le(const uint8_t p[4]) {
    uint32_t value = p[3];

    value = (value << 8) | p[2];
    value = (value << 8) | p[1];
    value = (value << 8) | p[0];

    return value;
}
static inline void PutWord(uint8_t* block, uint32_t value)
{
    uint32_t T = value;
    memcpy(block, &T, sizeof(uint8_t));
}
static inline void xorbuf(uint8_t* buf, const uint8_t* mask, uint32_t count)
{
    if (count == 0) return;

    while (count >= 4)
    {
        uint32_t r, b, m;
        std::memcpy(&b, buf, 4); std::memcpy(&m, mask, 4);

        r = b ^ m;
        std::memcpy(buf, &r, 4);

        buf += 4; mask += 4; count -= 4;
    }

    for (size_t i = 0; i < count; i++)
        buf[i] ^= mask[i];
}
Rabbit::Rabbit()
{
    memset(m_mx, 0, sizeof(uint32_t) * 8);
    memset(m_mc, 0, sizeof(uint32_t) * 8);
    memset(m_wx, 0, sizeof(uint32_t) * 8);
    memset(m_wc, 0, sizeof(uint32_t) * 8);

}

Rabbit::~Rabbit()
{
    clean(m_mx, sizeof(uint32_t) * 8);
    clean(m_mc, sizeof(uint32_t) * 8);
    clean(m_wx, sizeof(uint32_t) * 8);
    clean(m_wc, sizeof(uint32_t) * 8);
}
uint32_t Rabbit::NextState(uint32_t c[8], uint32_t x[8], uint32_t carry)
{
    /* Temporary variables */
    uint32_t g[8], c_old[8];

    /* Save old counter values */
    for (uint32_t i = 0; i < 8; i++)
    {
        c_old[i] = c[i];

    }

    /* Calculate new counter values */
    c[0] = static_cast<uint32_t>(c[0] + 0x4D34D34D + carry);
    c[1] = static_cast<uint32_t>(c[1] + 0xD34D34D3 + (c[0] < c_old[0]));
    c[2] = static_cast<uint32_t>(c[2] + 0x34D34D34 + (c[1] < c_old[1]));
    c[3] = static_cast<uint32_t>(c[3] + 0x4D34D34D + (c[2] < c_old[2]));
    c[4] = static_cast<uint32_t>(c[4] + 0xD34D34D3 + (c[3] < c_old[3]));
    c[5] = static_cast<uint32_t>(c[5] + 0x34D34D34 + (c[4] < c_old[4]));
    c[6] = static_cast<uint32_t>(c[6] + 0x4D34D34D + (c[5] < c_old[5]));
    c[7] = static_cast<uint32_t>(c[7] + 0xD34D34D3 + (c[6] < c_old[6]));
    carry = (c[7] < c_old[7]);

    /* Calculate the g-values */
    for (uint32_t i = 0; i < 8; i++)
    {
        g[i] = G_func(static_cast<uint32_t>(x[i] + c[i]));
    }
    /* Calculate new state values */
    x[0] = static_cast<uint32_t>(g[0] + leftRotate(g[7], 16) + leftRotate(g[6], 16));
    x[1] = static_cast<uint32_t>(g[1] + leftRotate(g[0], 8) + g[7]);
    x[2] = static_cast<uint32_t>(g[2] + leftRotate(g[1], 16) + leftRotate(g[0], 16));
    x[3] = static_cast<uint32_t>(g[3] + leftRotate(g[2], 8) + g[1]);
    x[4] = static_cast<uint32_t>(g[4] + leftRotate(g[3], 16) + leftRotate(g[2], 16));
    x[5] = static_cast<uint32_t>(g[5] + leftRotate(g[4], 8) + g[3]);
    x[6] = static_cast<uint32_t>(g[6] + leftRotate(g[5], 16) + leftRotate(g[4], 16));
    x[7] = static_cast<uint32_t>(g[7] + leftRotate(g[6], 8) + g[5]);

    return carry;
}
void Rabbit::initBlock()
{
    setKey(&tool);
    setIV(&tool);
}
void Rabbit::setKey(const Ktools* tool)
{
    uint32_t key32[4];
    memset(key32, 0, sizeof(uint32_t)*4);

    for (uint32_t i = 0; i < (tool->keySize/4) ; i++)
    {
        key32[i] = u8t32le(tool->key + i * 4);
    }

    /* Generate initial state variables */
    m_mx[0] = key32[0];
    m_mx[2] = key32[1];
    m_mx[4] = key32[2];
    m_mx[6] = key32[3];
    m_mx[1] = static_cast<uint32_t>(key32[3] << 16) | (key32[2] >> 16);
    m_mx[3] = static_cast<uint32_t>(key32[0] << 16) | (key32[3] >> 16);
    m_mx[5] = static_cast<uint32_t>(key32[1] << 16) | (key32[0] >> 16);
    m_mx[7] = static_cast<uint32_t>(key32[2] << 16) | (key32[1] >> 16);

    /* Generate initial counter values */
    m_mc[0] = leftRotate(key32[2],16);
    m_mc[2] = leftRotate(key32[3],16);
    m_mc[4] = leftRotate(key32[0],16);
    m_mc[6] = leftRotate(key32[1],16);
    m_mc[1] = (key32[0] & 0xFFFF0000) | (key32[1] & 0xFFFF);
    m_mc[3] = (key32[1] & 0xFFFF0000) | (key32[2] & 0xFFFF);
    m_mc[5] = (key32[2] & 0xFFFF0000) | (key32[3] & 0xFFFF);
    m_mc[7] = (key32[3] & 0xFFFF0000) | (key32[0] & 0xFFFF);

    /* Clear carry bit */
    m_mcy = 0;

    /* Iterate the system four times */
    for (uint32_t i = 0; i < 4; i++)
    {
        m_mcy = NextState(m_mc, m_mx, m_mcy);
    }
    /* Modify the counters */
    for (uint32_t i = 0; i < 8; i++)
    {
        m_mc[i] ^= m_mx[(i + 4) & 0x7];
    }
    /* Copy master instance to work instance */
    for (uint32_t i = 0; i < 8; i++)
    {
        m_wx[i] = m_mx[i];
        m_wc[i] = m_mc[i];
    }
    m_wcy = m_mcy;

}
void Rabbit::setIV(const Ktools* tool)
{
    uint32_t IV32[2];
    uint32_t temp[4];
    memset(IV32, 0, sizeof(uint32_t) * 2);
    memset(temp, 0, sizeof(uint32_t) * 4);

    for (uint32_t i = 0; i < (tool->ivSize/4); i++)
    {
        IV32[i] = u8t32le(tool->iv + i * 4);
    }

    /* Generate four subvectors */
    temp[0] = IV32[0];
    temp[2] = IV32[1];
    temp[1] = (temp[0] >> 16) | (temp[2] & 0xFFFF0000);
    temp[3] = (temp[2] >> 16) | (temp[0] & 0xFFFF0000);

    /* Modify counter values */
    m_wc[0] = m_mc[0] ^ temp[0];
    m_wc[1] = m_mc[1] ^ temp[1];
    m_wc[2] = m_mc[2] ^ temp[2];
    m_wc[3] = m_mc[3] ^ temp[3];
    m_wc[4] = m_mc[4] ^ temp[0];
    m_wc[5] = m_mc[5] ^ temp[1];
    m_wc[6] = m_mc[6] ^ temp[2];
    m_wc[7] = m_mc[7] ^ temp[3];

    /* Copy state variables */
    for (uint32_t i = 0; i < 8; i++)
    {
        m_wx[i] = m_mx[i];
    }

    m_wcy = m_mcy;

    /* Iterate the system four times */
    for (uint32_t i = 0; i < 4; i++)
    {
        m_wcy = NextState(m_wc, m_wx, m_wcy);
    }

}
void Rabbit::encrypt(uint8_t* output, const uint8_t* input, uint32_t len)
{
    initBlock();

    uint8_t* out = output;
    uint32_t length = len/16 ;
    //uint32_t j = 0;
     for (uint32_t i = 0; i < length ; i++)
    {
        /* Iterate the system */
        m_wcy = NextState(m_wc, m_wx, m_wcy);
       
        /*Encrypt/decrypt 16 bytes of data */
        PutWord(out + 0, m_wx[0] ^ (m_wx[5] >> 16) ^ (m_wx[3] << 16));
        PutWord(out + 4, m_wx[2] ^ (m_wx[7] >> 16) ^ (m_wx[5] << 16));
        PutWord(out + 8, m_wx[4] ^ (m_wx[1] >> 16) ^ (m_wx[7] << 16));
        PutWord(out + 12, m_wx[6] ^ (m_wx[3] >> 16) ^ (m_wx[1] << 16));
        out += 16;
    }

    /*XOR of the input with the keystream*/

   xorbuf(output, input, len);

}
void Rabbit::decrypt(uint8_t* output, const uint8_t* input, uint32_t len)
{
    encrypt(output, input, len);
}