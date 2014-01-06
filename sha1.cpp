#include <algorithm>
#include <inttypes.h>
#include <iostream>
#include <vector>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

template<typename T32>
T32 rol(T32 v, int amount)
{
	return (v << amount) | (v >> (32 - amount));
}

template<typename T8, typename T32>
class Sha1
{
	T32 m_h[5];
	T8 m_buf[64];
	size_t m_buf_idx;
	uint64_t m_size;

	void process()
	{
		T32 w[80] = {0};
		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 4; ++j)
				w[i] |= m_buf[i * 4 + 3 - j] << (j * 8);
		}
		for (int i = 16; i < 80; ++i) {
			w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		}

		T32 a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3], e = m_h[4];

		for (int i = 0; i < 80; ++i) {
			T32 f, k, temp;

			if (i < 20) {
				f = (b & c) | (~b & d);
				k = 0x5A827999;
			} else if (i < 40) {
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			} else if (i < 60) {
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			} else {
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			temp = rol(a, 5) + f + e + k + w[i];
			e = d;
			d = c;
			c = rol(b, 30);
			b = a;
			a = temp;
		}
		m_h[0] += a;
		m_h[1] += b;
		m_h[2] += c;
		m_h[3] += d;
		m_h[4] += e;
	}
public:
	Sha1(): m_buf_idx(0), m_size(0)
	{
		static const uint32_t h[5] = {
			0x67452301,
			0xEFCDAB89,
			0x98BADCFE,
			0x10325476,
			0xC3D2E1F0,
		};
		for (int i = 0; i < ARRAY_SIZE(h); ++i)
			m_h[i] = h[i];
	}

	template<typename Iterator>
	void add(Iterator first, Iterator last)
	{
		while (first != last) {
			m_buf[m_buf_idx++] = *first++;
			++m_size;
			if (m_buf_idx == ARRAY_SIZE(m_buf)) {
				process();
				m_buf_idx = 0;
			}
		}
	}
	void terminate(T32 (&h)[5])
	{
		uint64_t size = m_size * 8;
		uint8_t t[1] = {0x80};
		uint8_t z[64] = {0};

		add(t, t + 1);
		add(z, z + ((56u - m_size) & 0x3f));
		for (int i = 0; i < 8; ++i) {
			uint8_t *p = ((uint8_t*)&size) + 7 - i;
			add(p, p + 1);
		}
		std::copy(m_h, m_h + 5, h);
	}
};

int main()
{
	Sha1<uint8_t, uint32_t> s;
	uint32_t h[5];
	s.terminate(h);
	std::cout << std::hex << h[0] << ":" << h[1] << ":" << h[2] << ":" << h[3] << ":" << h[4] << std::endl;
	return 0;
}
