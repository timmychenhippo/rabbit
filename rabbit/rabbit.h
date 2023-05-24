#pragma once
#ifndef CRYPTO_RABBIT_H
#define CRYPTO_RABBIT_H
#endif

#include <stdint.h>
#include <string.h>

static inline uint32_t G_func(uint32_t x);
static inline void clean(void* dest, size_t size);
static inline void u32t8le(const uint32_t v, uint8_t p[4]);
static inline uint32_t u8t32le(const uint8_t p[4]);
static inline void PutWord(uint8_t* block, uint32_t value);

class Rabbit
{
private:
	/**
	* \Key:32-byte(256-bit)
	* \IV:12-byte(96-bit)
	* \counter:4-byte(32-bit)
	*/
	typedef struct __Key_tools
	{
		const uint8_t key[16] = { 0x29, 0x46, 0x42, 0x79, 0x24, 0x39, 0x76, 0x36,
							0x37, 0x73, 0x30, 0x59, 0x6B, 0x75, 0x67, 0x53 };
		const size_t keySize = 16;
		const uint8_t iv[8] = { 0x01, 0x40, 0x00, 0x00, 0x80, 0x00, 0x70, 0x4a };
		const size_t ivSize = 8;

	}Ktools;

	const Ktools tool;

	uint32_t m_mx[8];
	uint32_t m_mc[8];
	uint32_t m_wx[8];
	uint32_t m_wc[8];
	uint32_t m_mcy, m_wcy;

public:
	explicit Rabbit();
	~Rabbit();
	uint32_t NextState(uint32_t c[8], uint32_t x[8], uint32_t carry);
	void initBlock();
	void setKey(const Ktools* tools);
	void setIV(const Ktools* tools);
	void encrypt(uint8_t* output, const uint8_t* input, uint32_t len);
	void decrypt(uint8_t* output, const uint8_t* input, uint32_t len);
};