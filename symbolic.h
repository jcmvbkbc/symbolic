#ifndef _SYMBOLIC_H
#define _SYMBOLIC_H

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>

enum Type {
	SYMBOLIC_CONST,
	SYMBOLIC_EXPRESSION,
};

struct Symbol {
	const char *m_name;
	int m_idx;

	Symbol()
	{
	}
	Symbol(const char *name, int idx): m_name(name), m_idx(idx)
	{
	}

	bool operator ==(const Symbol& v) const
	{
		return m_idx == v.m_idx && strcmp(m_name, v.m_name) == 0;
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

	bool operator ==(const Negation& v) const
	{
		return m_neg == v.m_neg && m_symbol == v.m_symbol;
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
	Type m_type;
	bool m_const;
	std::set<Negation> m_neg;

	void reduce()
	{
		if (m_type == SYMBOLIC_CONST) {
			m_neg.clear();
			return;
		}

		for (auto it = m_neg.begin(), jt = it; it != m_neg.end(); jt = it, ++it)
			if (it != m_neg.begin() && *it == ~*jt) {
				m_type = SYMBOLIC_CONST;
				m_const = false;
				break;
			}
	}
public:
	Conjunction(bool v = true):
		m_type(SYMBOLIC_CONST), m_const(v)
	{
	}
	Conjunction(const Negation& v):
		m_type(SYMBOLIC_EXPRESSION), m_neg(&v, &v + 1)
	{
		m_neg.insert(v);
	}

	Expression operator ~() const;

	Conjunction& operator &=(const Conjunction& con)
	{
		if (*this == false || con == true)
			return *this;
		if (*this == true || con == false)
			return *this = con;

		m_type = SYMBOLIC_EXPRESSION;

		m_neg.insert(con.m_neg.begin(), con.m_neg.end());
		reduce();
		return *this;
	}

	bool operator ==(bool v) const
	{
		return m_type == SYMBOLIC_CONST && m_const == v;
	}
	bool operator <(const Conjunction& v) const
	{
		if (m_type != v.m_type)
			return m_type < v.m_type;
		if (m_type == SYMBOLIC_CONST)
			return m_const < v.m_const;
		else
			return std::lexicographical_compare(m_neg.begin(), m_neg.end(),
							    v.m_neg.begin(), v.m_neg.end());
	}
	bool includes(const Conjunction& v) const
	{
		if (m_type != SYMBOLIC_EXPRESSION || v.m_type != SYMBOLIC_EXPRESSION)
			return false;
		return std::includes(m_neg.begin(), m_neg.end(), v.m_neg.begin(), v.m_neg.end());
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		if (m_type == SYMBOLIC_CONST)
			return stm << (m_const ? "1" : "0");

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
	Type m_type;
	bool m_const;
	std::set<Conjunction> m_dis;

	void reduce_neg()
	{
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			Expression v = ~*it;

			if (v.m_dis.size() == 1 && m_dis.find(*v.m_dis.begin()) != m_dis.end()) {
				*this = Expression(true);
				break;
			}
		}
	}
	void reduce_prefix()
	{
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			for (auto jt = m_dis.begin(); jt != m_dis.end();)
				if (it != jt && jt->includes(*it))
					m_dis.erase(jt++);
				else
					++jt;
		}
	}
	void reduce()
	{
		if (m_type == SYMBOLIC_CONST) {
			m_dis.clear();
			return;
		}

		std::set<Conjunction> rv;
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			Conjunction c = *it;

			c.reduce();
			if (c == true) {
				m_dis.clear();
				m_type = SYMBOLIC_CONST;
				m_const = true;
				break;
			}
			if (!(c == false))
				rv.insert(c);
		}
		std::swap(m_dis, rv);
		if (m_dis.empty()) {
			m_type = SYMBOLIC_CONST;
			m_const = false;
		} else {
			reduce_neg();
			reduce_prefix();
		}
	}
public:
	Expression(bool v = false):
		m_type(SYMBOLIC_CONST), m_const(v)
	{
	}
	Expression(const Negation& v):
		m_type(SYMBOLIC_EXPRESSION)
	{
		m_dis.insert(Conjunction(v));
	}

	Expression operator ~() const
	{
		if (*this == true || *this == false)
			return Expression(!m_const);

		Expression rv;
		rv.m_type = SYMBOLIC_EXPRESSION;
		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			if (it == m_dis.begin())
				rv = ~*it;
			else
				rv &= ~*it;
		}
		rv.reduce();
		return rv;
	}

	Expression& operator |=(const Expression& con)
	{
		if (*this == false || con == true)
			return *this = con;
		if (*this == true || con == false)
			return *this;

		m_dis.insert(con.m_dis.begin(), con.m_dis.end());
		reduce();
		return *this;
	}

	Expression& operator &=(const Expression& con)
	{
		if (*this == false || con == true)
			return *this;
		if (*this == true || con == false)
			return *this = con;

		std::set<Conjunction> rv;

		for (auto it = m_dis.begin(); it != m_dis.end(); ++it) {
			for (auto jt = con.m_dis.begin(); jt != con.m_dis.end(); ++jt) {
				rv.insert(*it & *jt);
			}
		}
		std::swap(m_dis, rv);
		reduce();
		return *this;
	}

	bool operator ==(bool v) const
	{
		return m_type == SYMBOLIC_CONST && m_const == v;
	}
	bool operator <(const Expression& v) const
	{
		return std::lexicographical_compare(m_dis.begin(), m_dis.end(),
						    v.m_dis.begin(), v.m_dis.end());
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		if (m_type == SYMBOLIC_CONST)
			return stm << (m_const ? "1" : "0");

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
	Expression m_expression;

public:
	Symbolic(bool c = false): m_expression(c)
	{
	}
	Symbolic(const char *name, int idx): m_expression(Symbol(name, idx))
	{
	}
	Symbolic(const Expression& expression): m_expression(expression)
	{
	}

	Symbolic operator ~() const
	{
		return Symbolic(~m_expression);
	}

	Symbolic operator |(const Symbolic& b) const
	{
		return Symbolic(m_expression | b.m_expression);
	}

	Symbolic operator &(const Symbolic& b) const
	{
		return Symbolic(m_expression & b.m_expression);
	}

	Symbolic operator ^(const Symbolic& b) const
	{
		return (*this & ~b) | (~*this & b);
	}

	std::ostream& to_stream(std::ostream& stm) const
	{
		return stm << m_expression;
	}
};

std::ostream& operator <<(std::ostream& stm, const Symbolic& v)
{
	return v.to_stream(stm);
}

#endif /* _SYMBOLIC_H */
