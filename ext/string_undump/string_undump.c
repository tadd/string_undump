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

/* copied from ruby/string.c:rb_strseq_index */
static inline const char *
find_close_brace(const char *s, const char *s_end, rb_encoding *enc)
{
    const char *search_start;
    long search_len = s_end - s, pos;

    search_start = s;
    for (;;) {
	const char *t;
	pos = rb_memsearch("}", 1, search_start, search_len, enc);
	if (pos < 0) return NULL;
	t = rb_enc_right_char_head(search_start, search_start+pos, s_end, enc);
	if (t == search_start + pos) break;
	search_len -= t - search_start;
	if (search_len <= 0) return NULL;
	search_start = t;
    }
    return s + pos;
}

/* definition copied from ruby/string.c */
#define IS_EVSTR(p,e) ((p) < (e) && (*(p) == '$' || *(p) == '@' || *(p) == '{'))

static int
undump_after_backslash(VALUE undumped, const char *s, const char *s_end, rb_encoding *enc)
{
    unsigned int c, c2;
    int n, n2, codelen;
    size_t hexlen;
    char buf[6];

    c = rb_enc_codepoint_len(s, s_end, &n, enc);
    switch (c) {
      case '\\':
      case '"':
	rb_str_cat(undumped, s, n); /* cat itself */
	n++;
	break;
      case 'n':
      case 'r':
      case 't':
      case 'f':
      case 'v':
      case 'b':
      case 'a':
      case 'e':
	rb_str_cat(undumped, unescape_ascii(c), n);
	n++;
	break;
      case 'u':
	c2 = rb_enc_codepoint_len(s+1, s_end, NULL, enc);
	if (c2 == '{') { /* handle \u{...} form */
	    const char *p = find_close_brace(s+2, s_end, enc);
	    unsigned int hex;
	    if (p == NULL) {
		rb_raise(rb_eArgError, "unterminated Unicode escape");
	    }
	    hex = ruby_scan_hex(s+2, p-(s+2)+1, &hexlen);
	    if (hexlen == 0 || hexlen > 6) {
		rb_raise(rb_eArgError, "invalid Unicode escape");
	    }
	    if (hex > 0x10ffffU) {
		rb_raise(rb_eArgError, "invalid Unicode codepoint (too large)");
	    }
	    if ((hex & 0xfffff800U) == 0xd800U) {
		rb_raise(rb_eArgError, "invalid Unicode codepoint");
	    }
	    codelen = rb_enc_codelen(hex, enc);
	    rb_enc_mbcput(hex, buf, enc);
	    rb_str_cat(undumped, buf, codelen);
	    n += 3 + hexlen;/* strlen("u{...}") */
	}
	else { /* handle \uXXXX form */
	    unsigned int hex = ruby_scan_hex(s+1, 4, &hexlen);
	    if (hexlen != 4) {
		rb_raise(rb_eArgError, "invalid Unicode escape");
	    }
	    codelen = rb_enc_codelen(hex, enc);
	    rb_enc_mbcput(hex, buf, enc);
	    rb_str_cat(undumped, buf, codelen);
	    n += 5; /* strlen("uXXXX") */
	}
	break;
      case 'x':
	c2 = ruby_scan_hex(s+1, 2, &hexlen);
	if (hexlen != 2) {
	    rb_raise(rb_eArgError, "invalid hex escape");
	}
	*buf = (char)c2;
	rb_str_cat(undumped, buf, 1L);
	n += 3; /* strlen("xXX") */
	break;
      case '#':
	n2 = rb_enc_mbclen(s+1, s_end, enc);
	if (n2 == 1 && IS_EVSTR(s+1, s_end)) {
	    rb_str_cat(undumped, s, n);
	    n += n2;
	}
	break;
      default:
	rb_str_cat(undumped, "\\", 1L); /* keep backslash */
    }

    return n;
}

static VALUE
str_undump_roughly(VALUE str)
{
    const char *s = StringValuePtr(str);
    const char *s_end = RSTRING_END(str);
    rb_encoding *enc = rb_enc_get(str);
    int n;
    unsigned int c;
    VALUE undumped = rb_enc_str_new(s, 0L, enc);

    rb_must_asciicompat(str);

    if (is_wrapped(s, s_end, enc)) {
	/* strip '"' at the begin and the end */
	s++;
	s_end--;
    }

    for (; s < s_end; s += n) {
	c = rb_enc_codepoint_len(s, s_end, &n, enc);
	if (c == '\\' && s+1 < s_end) {
	    n = undump_after_backslash(undumped, s+1, s_end, enc);
	}
	else {
	    rb_str_cat(undumped, s, n);
	}
    }

    return undumped;
}

void
Init_string_undump(void)
{
    rb_define_method(rb_cString, "undump_roughly", str_undump_roughly, 0);
}
