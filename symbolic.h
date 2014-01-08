#ifndef _SYMBOLIC_H
#define _SYMBOLIC_H

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>

struct Symbol {
	const char *m_name;
	int m_idx;

	Symbol()
	{
	}
	Symbol(const char *name, int idx): m_name(name), m_idx(idx)
	{
	}

	bool operator <(const Symbol& v) const
	{
		int c = strcmp(m_name, v.m_name);
		if (c == 0)
			return m_idx < v.m_idx;
		else
			return c < 0;
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		return stm << m_name << "[" << m_idx << "]";
	}
};

std::ostream& operator <<(std::ostream& stm, const Symbol& v)
{
	return v.to_stream(stm);
}

struct Negation
{
	Symbol m_symbol;
	bool m_neg;

	Negation()
	{
	}
	Negation(const Symbol& symbol, bool neg = false): m_symbol(symbol), m_neg(neg)
	{
	}

	Negation operator ~() const
	{
		return Negation(m_symbol, !m_neg);
	}

	bool operator <(const Negation& v) const
	{
		return m_symbol < v.m_symbol ||
			(!(v.m_symbol < m_symbol) && m_neg < v.m_neg);

	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		if (m_neg)
			stm << "~";
		return stm << m_symbol;
	}
};

std::ostream& operator <<(std::ostream& stm, const Negation& v)
{
	return v.to_stream(stm);
}

struct Expression;
struct Conjunction
{
	std::set<Negation> m_neg;

public:
	Conjunction()
	{
	}
	Conjunction(const Negation& v)
	{
		m_neg.insert(v);
	}

	Expression operator ~() const;

	Conjunction& operator &=(const Conjunction& con)
	{
		m_neg.insert(con.m_neg.begin(), con.m_neg.end());
		return *this;
	}

	bool operator <(const Conjunction& v) const
	{
		return std::lexicographical_compare(m_neg.begin(), m_neg.end(),
						    v.m_neg.begin(), v.m_neg.end());
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		for (auto it = m_neg.begin(); it != m_neg.end(); ++it) {
			if (it != m_neg.begin())
				stm << " & ";
			stm << *it;
		}
		return stm;
	}
};

Conjunction operator &(Conjunction a, const Conjunction& b)
{
	return a &= b;
}

std::ostream& operator <<(std::ostream& stm, const Conjunction& v)
{
	return v.to_stream(stm);
}

struct Expression
{
	std::set<Conjunction> m_dis;

	void reduce()
	{
	}
public:
	Expression()
	{
	}
	Expression(const Negation& v)
	{
		m_dis.insert(Conjunction(v));
	}

	Expression operator ~() const
	{
		Expression rv;
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			if (it == m_dis.begin())
				rv = ~*it;
			else
				rv &= ~*it;
		}
		return rv;
	}

	Expression& operator |=(const Expression& con)
	{
		m_dis.insert(con.m_dis.begin(), con.m_dis.end());
		return *this;
	}

	Expression& operator &=(const Expression& con)
	{
		std::set<Conjunction> rv;

		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			for (auto jt = con.m_dis.begin(); jt != con.m_dis.end(); ++jt) {
				rv.insert(*it & *jt);
			}
		}
		std::swap(m_dis, rv);
		return *this;
	}

	bool operator <(const Expression& v) const
	{
		return std::lexicographical_compare(m_dis.begin(), m_dis.end(),
						    v.m_dis.begin(), v.m_dis.end());
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			if (it != m_dis.begin())
				stm << " | ";
			stm << "(" << *it << ")";
		}
		return stm;
	}
};

Expression operator |(Expression a, const Expression& b)
{
	return a |= b;
}

Expression operator &(Expression a, const Expression& b)
{
	return a &= b;
}


std::ostream& operator <<(std::ostream& stm, const Expression& v)
{
	return v.to_stream(stm);
}

Expression Conjunction::operator ~() const
{
	Expression rv;
	for (auto it = m_neg.begin(); it != m_neg.end(); ++it) {
		rv |= ~*it;
	}
	return rv;
}


class Symbolic
{
	enum Type {
		SYMBOLIC_CONST,
		SYMBOLIC_EXPRESSION,
	};

	Type m_type;
	bool m_const;
	Expression m_expression;

public:
	Symbolic(bool c = false): m_type(SYMBOLIC_CONST), m_const(c)
	{
	}
	Symbolic(const char *name, int idx):
		m_type(SYMBOLIC_EXPRESSION), m_expression(Symbol(name, idx))
	{
	}
	Symbolic(const Expression& expression):
		m_type(SYMBOLIC_EXPRESSION), m_expression(expression)
	{
	}

	Symbolic operator ~() const
	{
		switch (m_type) {
		case SYMBOLIC_CONST:
			return Symbolic(!m_const);

		case SYMBOLIC_EXPRESSION:
			return Symbolic(~m_expression);
		}
	}

	Symbolic operator |(const Symbolic& b) const
	{
		if (m_type == SYMBOLIC_CONST) {
			if (m_const)
				return Symbolic(true);
			else
				return b;
		} else if (b.m_type == SYMBOLIC_CONST) {
			if (b.m_const)
				return Symbolic(true);
			else
				return *this;
		} else {
			return Symbolic(m_expression | b.m_expression);
		}
	}

	Symbolic operator &(const Symbolic& b) const
	{
		if (m_type == SYMBOLIC_CONST) {
			if (!m_const)
				return Symbolic(false);
			else
				return b;
		} else if (b.m_type == SYMBOLIC_CONST) {
			if (!b.m_const)
				return Symbolic(false);
			else
				return *this;
		} else {
			return Symbolic(m_expression & b.m_expression);
		}
	}

	Symbolic operator ^(const Symbolic& b) const
	{
		return (*this & ~b) | (~*this & b);
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		switch (m_type) {
		case SYMBOLIC_CONST:
			return stm << (m_const ? "1" : "0");

		case SYMBOLIC_EXPRESSION:
			return stm << m_expression;
		}
		return stm;
	}
};

std::ostream& operator <<(std::ostream& stm, const Symbolic& v)
{
	return v.to_stream(stm);
}

#endif /* _SYMBOLIC_H */
