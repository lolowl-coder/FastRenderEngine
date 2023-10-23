#pragma once

/*
 t - type
 fn - field name
 sn - setter name
 fs - fiels scope
 gs - getter scope
 ss - setter scope
 l - lock
 */

#define GETTER(t,fn,l) \
	virtual t fn(void) const \
	{ \
		t result; \
		{ \
			l; \
			result = m##fn; \
		} \
		return result; \
	}

#define SETTER(t,fn,sn,l) \
	virtual void set##sn(t value) \
	{ \
		l; \
		m##fn = value; \
	}

#define FIELD(t,fn,sn,fs,gs,ss,l) \
	fs: \
		t m##fn; \
	gs: \
		GETTER(t,fn,l) \
	ss: \
		SETTER(const t&,fn,sn,l)

#define FIELD_PTR(t,fn,sn,fs,gs,ss,l) \
		fs: \
			t* m_##fn; \
		gs: \
			GETTER(t*,fn,l) \
		ss: \
			SETTER(t*,fn,sn,l)

#define GETTER_NS(t,fn) \
	virtual t fn(void) const \
	{ \
		t result; \
		{ \
			result = m_##fn; \
		} \
		return result; \
	}

#define SETTER_NS(t,fn,sn) \
	virtual void set##sn(t value) \
	{ \
		m_##fn = value; \
	}

#define FIELD_NS(t,fn,sn,fs,gs,ss) \
	fs: \
		t m_##fn; \
	gs: \
		GETTER_NS(t,fn) \
	ss: \
		SETTER_NS(const t&,fn,sn)

#define FIELD_NS_CS(t,fn,sn,fs,gs,ss) \
	fs: \
		t m_##fn; \
	gs: \
		GETTER_NS(t,fn) \
	ss: \
		SETTER_NS(t,fn,sn)

#define FIELD_PTR_NS(t,fn,sn,fs,gs,ss) \
		fs: \
			t* m_##fn; \
		gs: \
			GETTER_NS(t*,fn) \
		ss: \
			SETTER_NS(t*,fn,sn)
