#include "string_undump.h"

/* is +str+ wrapped with '"'? */
static int
is_wrapped(const char *s, const char *s_end, rb_encoding *enc)
{
    unsigned int cbeg, cend;
    const char *prev;

    cbeg = rb_enc_mbc_to_codepoint(s, s_end, enc);
    if (cbeg != '"'){
	return FALSE;
    }

    prev = rb_enc_prev_char(s, s_end, s_end, enc);
    cend = rb_enc_mbc_to_codepoint(prev, s_end, enc);
    return cend == '"';
}

static VALUE
str_undump_roughly(VALUE str)
{
    const char *s = StringValuePtr(str);
    const char *s_end = RSTRING_END(str);
    long len = RSTRING_LEN(str);
    rb_encoding *enc = rb_enc_get(str);
#if 0
    int n;
    unsigned int c;
    VALUE undumped = rb_enc_str_new(s, 0L, enc);
#endif

    if (is_wrapped(s, s_end, enc)) {
	/* strip '"' at the begin and the end */
	s++;
	s_end--;
	len -= 2;
    }

#if 0
    while (s < s_end) {
	c = rb_enc_codepoint_len(s, s_end, &n, enc);
	s += n;
    }
#endif

    return rb_enc_str_new(s, len, enc);
}

void
Init_string_undump(void)
{
    rb_define_method(rb_cString, "undump_roughly", str_undump_roughly, 0);
}
