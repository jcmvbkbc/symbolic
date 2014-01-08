#include <algorithm>
#include <inttypes.h>
#include <iostream>
#include <memory>
#include <vector>

#include "symbolic.h"

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
				w[i] |= T32(m_buf[i * 4 + 3 - j]) << (j * 8);
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
		std::copy(h, h + 5, m_h);
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

template<int W>
class Wide
{
	Symbolic m_data[W];

public:
	Wide(uint32_t v = 0)
	{
		for (int i = 0; i < W; ++i, v >>= 1)
			m_data[i] = v & 1;
	}
	template<int Z>
	Wide(const Wide<Z>& v)
	{
		for (int i = 0; i < std::min(W, Z); ++i)
			m_data[i] = v.get(i);
	}

	const Symbolic& get(int i) const
	{
		return m_data[i];
	}
	void set(int i, const Symbolic& v)
	{
		m_data[i] = v;
	}

	Wide operator ~() const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			w.m_data[i] = ~m_data[i];
		return w;
	}

	Wide operator |(const Wide& v) const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			w.m_data[i] = m_data[i] | v.m_data[i];
		return w;
	}
	Wide& operator |=(const Wide& v)
	{
		for (int i = 0; i < W; ++i)
			m_data[i] = m_data[i] | v.m_data[i];
		return *this;
	}

	Wide operator &(const Wide& v) const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			w.m_data[i] = m_data[i] & v.m_data[i];
		return w;
	}

	Wide operator ^(const Wide& v) const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			w.m_data[i] = m_data[i] ^ v.m_data[i];
		return w;
	}

	Wide operator <<(int sz) const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			if (i < sz)
				w.m_data[i] = 0;
			else
				w.m_data[i] = m_data[i - sz];
		return w;
	}

	Wide operator >>(int sz) const
	{
		Wide w;

		for (int i = 0; i < W; ++i)
			if (i + sz < W)
				w.m_data[i] = m_data[i + sz];
			else
				w.m_data[i] = 0;
		return w;
	}

	Wide operator +(const Wide& v) const
	{
		Wide w;
		Symbolic c;

		for (int i = 0; i < W; ++i) {
			Symbolic a = m_data[i];
			Symbolic b = v.m_data[i];

			w.m_data[i] = a ^ b ^ c;
			c = (a & b) | (a & c) | (b & c);
		}
		return w;
	}
	Wide& operator +=(const Wide& v)
	{
		*this = *this + v;
		return *this;
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		stm << "{" << std::endl;
		for (int i = 0; i < W; ++i) {
			stm << "    [" << i << "] = {" << m_data[i] << "}," << std::endl;
		}
		stm << "}";
		return stm;
	}
};

template<int W>
std::ostream& operator <<(std::ostream& stm, const Wide<W>& v)
{
	return v.to_stream(stm);
}

int main()
{
	Sha1<Wide<8>, Wide<32> > s;
	Wide<32> h[5];
	Wide<8> data[1];

	data[0].set(0, Symbolic("x", 0));
	data[0].set(1, Symbolic("x", 1));
	data[0].set(2, Symbolic("x", 2));
	data[0].set(3, Symbolic("x", 3));
	//data[0].set(4, Symbolic("x", 4));
	//data[0].set(5, Symbolic("x", 5));
	//data[0].set(6, Symbolic("x", 6));
	//data[0].set(7, Symbolic("x", 7));
	s.add(data, data + ARRAY_SIZE(data));
	s.terminate(h);
	std::cout << std::hex << h[0] << ":" << h[1] << ":" << h[2] << ":" << h[3] << ":" << h[4] << std::endl;
	return 0;
}
