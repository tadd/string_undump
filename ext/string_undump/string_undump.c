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

/* definition copied from ruby/string.c */
#define IS_EVSTR(p,e) ((p) < (e) && (*(p) == '$' || *(p) == '@' || *(p) == '{'))

static VALUE
str_undump_roughly(VALUE str)
{
    const char *s = StringValuePtr(str);
    const char *s_end = RSTRING_END(str);
    long len = RSTRING_LEN(str);
    rb_encoding *enc = rb_enc_get(str);
    int n;
    unsigned int c;
    VALUE undumped = rb_enc_str_new(s, 0L, enc);
    int got_backslash = FALSE;

    if (is_wrapped(s, s_end, enc)) {
	/* strip '"' at the begin and the end */
	s++;
	s_end--;
	len -= 2;
    }

    for (; s < s_end; s += n) {
	c = rb_enc_codepoint_len(s, s_end, &n, enc);
	if (c == '\\' && !got_backslash)
	{
	    got_backslash = TRUE;
	    continue;
	}
	if (got_backslash)
	{
	    int n2;

	    switch (c)
	    {
	      case '#':
		n2 = rb_enc_mbclen(s+1, s_end, enc);
		if (n2 == 1 && IS_EVSTR(s+1, s_end))
		{
		    break;
		}
		/* fall through */
	      case '\\':
	      case '"':
		break; /* don't add backslash */
	      default:
		rb_str_cat(undumped, "\\", 1L); /* keep backslash */
	    }
	    got_backslash = FALSE;
	}

	rb_str_cat(undumped, s, n);
    }

    return undumped;
}

void
Init_string_undump(void)
{
    rb_define_method(rb_cString, "undump_roughly", str_undump_roughly, 0);
}
