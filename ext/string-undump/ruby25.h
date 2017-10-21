#ifndef RUBY25_H_INCLUDED
#define RUBY25_H_INCLUDED

#include <ruby/encoding.h>

/*
 * copied from ruby/internal.h
 */

rb_encoding *rb_enc_get_from_index(int index);

/*
 * copied from ruby/encindex.h
 */

enum ruby_preserved_encindex {
    RUBY_ENCINDEX_ASCII,
    RUBY_ENCINDEX_UTF_8,
    RUBY_ENCINDEX_US_ASCII,

    /* preserved indexes */
    RUBY_ENCINDEX_UTF_16BE,
    RUBY_ENCINDEX_UTF_16LE,
    RUBY_ENCINDEX_UTF_32BE,
    RUBY_ENCINDEX_UTF_32LE,
    RUBY_ENCINDEX_UTF_16,
    RUBY_ENCINDEX_UTF_32,
    RUBY_ENCINDEX_UTF8_MAC,

    /* for old options of regexp */
    RUBY_ENCINDEX_EUC_JP,
    RUBY_ENCINDEX_Windows_31J,

    RUBY_ENCINDEX_BUILTIN_MAX
};

#define ENCINDEX_ASCII       RUBY_ENCINDEX_ASCII
#define ENCINDEX_UTF_8       RUBY_ENCINDEX_UTF_8
#define ENCINDEX_US_ASCII    RUBY_ENCINDEX_US_ASCII
#define ENCINDEX_UTF_16BE    RUBY_ENCINDEX_UTF_16BE
#define ENCINDEX_UTF_16LE    RUBY_ENCINDEX_UTF_16LE
#define ENCINDEX_UTF_32BE    RUBY_ENCINDEX_UTF_32BE
#define ENCINDEX_UTF_32LE    RUBY_ENCINDEX_UTF_32LE
#define ENCINDEX_UTF_16      RUBY_ENCINDEX_UTF_16
#define ENCINDEX_UTF_32      RUBY_ENCINDEX_UTF_32
#define ENCINDEX_UTF8_MAC    RUBY_ENCINDEX_UTF8_MAC
#define ENCINDEX_EUC_JP      RUBY_ENCINDEX_EUC_JP
#define ENCINDEX_Windows_31J RUBY_ENCINDEX_Windows_31J
#define ENCINDEX_BUILTIN_MAX RUBY_ENCINDEX_BUILTIN_MAX

/*
 * copied from ruby/string.c
 */

static rb_encoding *
get_actual_encoding(const int encidx, VALUE str)
{
    const unsigned char *q;

    switch (encidx) {
      case ENCINDEX_UTF_16:
	if (RSTRING_LEN(str) < 2) break;
	q = (const unsigned char *)RSTRING_PTR(str);
	if (q[0] == 0xFE && q[1] == 0xFF) {
	    return rb_enc_get_from_index(ENCINDEX_UTF_16BE);
	}
	if (q[0] == 0xFF && q[1] == 0xFE) {
	    return rb_enc_get_from_index(ENCINDEX_UTF_16LE);
	}
	return rb_ascii8bit_encoding();
      case ENCINDEX_UTF_32:
	if (RSTRING_LEN(str) < 4) break;
	q = (const unsigned char *)RSTRING_PTR(str);
	if (q[0] == 0 && q[1] == 0 && q[2] == 0xFE && q[3] == 0xFF) {
	    return rb_enc_get_from_index(ENCINDEX_UTF_32BE);
	}
	if (q[3] == 0 && q[2] == 0 && q[1] == 0xFE && q[0] == 0xFF) {
	    return rb_enc_get_from_index(ENCINDEX_UTF_32LE);
	}
	return rb_ascii8bit_encoding();
    }
    return rb_enc_from_index(encidx);
}

static rb_encoding *
get_encoding(VALUE str)
{
    return get_actual_encoding(ENCODING_GET(str), str);
}

#define STR_ENC_GET(str) get_encoding(str)

static inline int
single_byte_optimizable(VALUE str)
{
    rb_encoding *enc;

    /* Conservative.  It may be ENC_CODERANGE_UNKNOWN. */
    if (ENC_CODERANGE(str) == ENC_CODERANGE_7BIT)
        return 1;

    enc = STR_ENC_GET(str);
    if (rb_enc_mbmaxlen(enc) == 1)
        return 1;

    /* Conservative.  Possibly single byte.
     * "\xa1" in Shift_JIS for example. */
    return 0;
}

static inline int
enumerator_element(VALUE ary, VALUE e)
{
    if (ary) {
	rb_ary_push(ary, e);
	return 0;
    }
    else {
	rb_yield(e);
	return 1;
    }
}

#define ENUM_ELEM(ary, e) enumerator_element(ary, e)

static VALUE
rb_str_enumerate_bytes(VALUE str, VALUE ary)
{
    long i;

    for (i=0; i<RSTRING_LEN(str); i++) {
	ENUM_ELEM(ary, INT2FIX(RSTRING_PTR(str)[i] & 0xff));
    }
    if (ary)
	return ary;
    else
	return str;
}

static VALUE
str_enumerate_codepoints(VALUE str, VALUE ary)
{
    VALUE orig = str;
    int n;
    unsigned int c;
    const char *ptr, *end;
    rb_encoding *enc;

    if (single_byte_optimizable(str))
	return rb_str_enumerate_bytes(str, ary);

    str = rb_str_new_frozen(str);
    ptr = RSTRING_PTR(str);
    end = RSTRING_END(str);
    enc = STR_ENC_GET(str);

    while (ptr < end) {
	c = rb_enc_codepoint_len(ptr, end, &n, enc);
	ENUM_ELEM(ary, UINT2NUM(c));
	ptr += n;
    }
    RB_GC_GUARD(str);
    if (ary)
	return ary;
    else
	return orig;
}

/*
 * copy ends here
 */

#endif
