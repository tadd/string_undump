#include "string_undump.h"

/* is +str+ wrapped with '"'? */
static int
is_wrapped(const char *s, const char *s_end, rb_encoding *enc)
{
    unsigned int cbeg, cend;
    const char *prev;

    cbeg = rb_enc_mbc_to_codepoint(s, s_end, enc);
    if (cbeg != '"') return FALSE;

    prev = rb_enc_prev_char(s, s_end, s_end, enc);
    cend = rb_enc_mbc_to_codepoint(prev, s_end, enc);
    return cend == '"';
}

static inline const char *
unescape_ascii(unsigned int c)
{
    switch (c) {
      case 'n':
	return "\n";
      case 'r':
	return "\r";
      case 't':
	return "\t";
      case 'f':
	return "\f";
      case 'v':
	return "\v";
      case 'b':
	return "\b";
      case 'a':
	return "\a";
      case 'e':
	return "\e";
      default:
	UNREACHABLE;
    }
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

    rb_must_asciicompat(str);

    if (is_wrapped(s, s_end, enc)) {
	/* strip '"' at the begin and the end */
	s++;
	s_end--;
	len -= 2;
    }

    for (; s < s_end; s += n) {
	c = rb_enc_codepoint_len(s, s_end, &n, enc);
	if (c == '\\' && !got_backslash) {
	    got_backslash = TRUE;
	    continue;
	}
	if (got_backslash) {
	    unsigned int c2;
	    int n2, codelen;
	    size_t hexlen;
	    char buf[6];

	    switch (c) {
	      case '\\':
	      case '"':
		break; /* don't add backslash */
	      case 'n':
	      case 'r':
	      case 't':
	      case 'f':
	      case 'v':
	      case 'b':
	      case 'a':
	      case 'e':
		rb_str_cat(undumped, unescape_ascii(c), 1L);
		n = 1;
		got_backslash = FALSE;
		continue;
	      case 'u':
		c2 = ruby_scan_hex(s+1, 4, &hexlen);
		if (hexlen != 4) {
		    rb_raise(rb_eArgError, "invalid Unicode escape");
		}
		codelen = rb_enc_codelen(c2, enc);
		rb_enc_mbcput(c2, buf, enc);
		rb_str_cat(undumped, buf, codelen);
		n = 5; /* strlen("uXXXX") */
		got_backslash = FALSE;
		continue;
	      case 'x':
		c2 = ruby_scan_hex(s+1, 2, &hexlen);
		if (hexlen != 2) {
		    rb_raise(rb_eArgError, "invalid hex escape");
		}
		*buf = (char)c2;
		rb_str_cat(undumped, buf, 1L);
		n = 3; /* strlen("xXX") */
		got_backslash = FALSE;
		continue;
	      case '#':
		n2 = rb_enc_mbclen(s+1, s_end, enc);
		if (n2 == 1 && IS_EVSTR(s+1, s_end)) break;
		/* fall through */
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
