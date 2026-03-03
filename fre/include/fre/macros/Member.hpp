#pragma once

/*
 t - type
 fn - field name
 fs - field scope
 gs - getter scope
 ss - setter scope
 l - lock
 */

#define GETTER_SETTER(type, fn)\
void set##fn(const type& value)\
{\
	m##fn = value;\
}\
type get##fn() const\
{\
	return m##fn;\
}

#define GETTER(t,fn,l) \
	virtual t get##fn(void) const \
	{ \
		t result; \
		{ \
			l; \
			result = m##fn; \
		} \
		return result; \
	}

#define SETTER(t,fn,l) \
	virtual void set##fn(t value) \
	{ \
		l; \
		m##fn = value; \
	}

#define FIELD(t,fn,fs,gs,ss,l) \
	fs: \
		t m##fn; \
	gs: \
		GETTER(t,fn,l) \
	ss: \
		SETTER(const t&,fn,l)

#define FIELD_PTR(t,fn,fs,gs,ss,l) \
		fs: \
			t* m##fn; \
		gs: \
			GETTER(t*,fn,l) \
		ss: \
			SETTER(t*,fn,l)

#define GETTER_NS(t,fn) \
	virtual t get##fn(void) const \
	{ \
		t result; \
		{ \
			result = m##fn; \
		} \
		return result; \
	}

#define SETTER_NS(t,fn) \
	virtual void set##fn(t value) \
	{ \
		m##fn = value; \
	}

#define FIELD_NS(t,fn,fs,gs,ss) \
	fs: \
		t m##fn; \
	gs: \
		GETTER_NS(t,fn) \
	ss: \
		SETTER_NS(const t&,fn)

#define FIELD_NS_CS(t,fn,fs,gs,ss) \
	fs: \
		t m##fn; \
	gs: \
		GETTER_NS(t,fn) \
	ss: \
		SETTER_NS(t,fn)

#define FIELD_PTR_NS(t,fn,fs,gs,ss) \
		fs: \
			t* m##fn; \
		gs: \
			GETTER_NS(t*,fn) \
		ss: \
			SETTER_NS(t*,fn)
