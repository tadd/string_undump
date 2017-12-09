#include "string_undump.h"

/* copied from ruby/string.c */
static long
strseq_core(const char *str_ptr, const char *str_ptr_end, long str_len,
	    const char *sub_ptr, long sub_len, long offset, rb_encoding *enc)
{
    const char *search_start = str_ptr;
    long pos, search_len = str_len - offset;

    for (;;) {
	const char *t;
	pos = rb_memsearch(sub_ptr, sub_len, search_start, search_len, enc);
	if (pos < 0) return pos;
	t = rb_enc_right_char_head(search_start, search_start+pos, str_ptr_end, enc);
	if (t == search_start + pos) break;
	search_len -= t - search_start;
	if (search_len <= 0) return -1;
	offset += t - search_start;
	search_start = t;
    }
    return pos + offset;
}

enum undump_source_format {
    UNDUMP_SOURCE_SIMPLE, /* "..." */
    UNDUMP_SOURCE_FORCE_ENCODING, /* "...".force_encoding("...") */
    UNDUMP_SOURCE_INVALID
};

static enum undump_source_format
check_undump_source_format(const char *s, const char *s_end, long len, rb_encoding *enc,
			   VALUE *forced_enc_str, long *forced_enc_str_len)
{
    unsigned int cbeg, cend;
    const char *prev;
    static const long force_encoding_minimum_len = rb_strlen_lit("\"\".force_encoding(\"\")");
    static const char force_encoding_middle_part[] = "\".force_encoding(\"";
    static const long force_encoding_middle_part_len = rb_strlen_lit("\".force_encoding(\"");
    static const char force_encoding_end_part[] = "\")";
    static const long force_encoding_end_part_len = rb_strlen_lit("\")");
    long pos_before_middle_part, pos_before_end_part, pos_after_middle_part;

    if (len < 2) return UNDUMP_SOURCE_INVALID;

    cbeg = rb_enc_mbc_to_codepoint(s, s_end, enc);
    if (cbeg != '"') return UNDUMP_SOURCE_INVALID;

    prev = rb_enc_prev_char(s, s_end, s_end, enc);
    cend = rb_enc_mbc_to_codepoint(prev, s_end, enc);
    if (cend == '"') return UNDUMP_SOURCE_SIMPLE;

    if (cend != ')' || len < force_encoding_minimum_len) {
	return UNDUMP_SOURCE_INVALID;
    }

    /* find '".force_encoding("' */
    pos_before_middle_part = strseq_core(s, s_end, len,
					 force_encoding_middle_part, force_encoding_middle_part_len,
					 0, enc);
    if (pos_before_middle_part <= 0) {
	return UNDUMP_SOURCE_INVALID;
    }

    pos_after_middle_part = pos_before_middle_part + force_encoding_middle_part_len;
    /* find '")' */
    pos_before_end_part = strseq_core(s + pos_after_middle_part, s_end, len - pos_after_middle_part,
				      force_encoding_end_part, force_encoding_end_part_len,
				      0, enc);
    if (pos_before_end_part < 0 || pos_after_middle_part + pos_before_end_part + 2 != len) {
	return UNDUMP_SOURCE_INVALID;
    }

    *forced_enc_str_len = pos_before_end_part;
    *forced_enc_str = rb_str_new(s + pos_after_middle_part, *forced_enc_str_len);
    return UNDUMP_SOURCE_FORCE_ENCODING;
}

static const char *
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

static int
undump_after_backslash(VALUE undumped, const char *s, const char *s_end, rb_encoding **penc)
{
    unsigned int c, c2;
    int n, n2, codelen;
    size_t hexlen;
    char buf[6];
    static const rb_encoding *enc_utf8 = NULL;

    c = rb_enc_codepoint_len(s, s_end, &n, *penc);
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
	if (s+1 >= s_end) {
	    rb_raise(rb_eArgError, "invalid Unicode escape");
	}
	if (enc_utf8 == NULL) enc_utf8 = rb_utf8_encoding();
	if (*penc != enc_utf8) {
	    *penc = enc_utf8;
	    rb_enc_associate(undumped, enc_utf8);
	    ENC_CODERANGE_CLEAR(undumped);
	}
	c2 = rb_enc_codepoint_len(s+1, s_end, NULL, *penc);
	if (c2 == '{') { /* handle \u{...} form */
	    const char *hexstr = s + 2;
	    unsigned int hex;
	    static const char* const close_brace = "}";
	    long pos;

	    if (hexstr >= s_end) {
		rb_raise(rb_eArgError, "unterminated Unicode escape");
	    }
	    /* find close brace */
	    pos = strseq_core(hexstr, s_end, s_end - hexstr, close_brace, 1, 0, *penc);
	    if (pos < 0) {
		rb_raise(rb_eArgError, "unterminated Unicode escape");
	    }
	    hex = ruby_scan_hex(hexstr, pos, &hexlen);
	    if (hexlen == 0 || hexlen > 6) {
		rb_raise(rb_eArgError, "invalid Unicode escape");
	    }
	    if (hex > 0x10ffffU) {
		rb_raise(rb_eArgError, "invalid Unicode codepoint (too large)");
	    }
	    if ((hex & 0xfffff800U) == 0xd800U) {
		rb_raise(rb_eArgError, "invalid Unicode codepoint");
	    }
	    codelen = rb_enc_codelen(hex, *penc);
	    rb_enc_mbcput(hex, buf, *penc);
	    rb_str_cat(undumped, buf, codelen);
	    n += rb_strlen_lit("u{}") + hexlen;
	}
	else { /* handle \uXXXX form */
	    unsigned int hex = ruby_scan_hex(s+1, 4, &hexlen);
	    if (hexlen != 4) {
		rb_raise(rb_eArgError, "invalid Unicode escape");
	    }
	    codelen = rb_enc_codelen(hex, *penc);
	    rb_enc_mbcput(hex, buf, *penc);
	    rb_str_cat(undumped, buf, codelen);
	    n += rb_strlen_lit("uXXXX");
	}
	break;
      case 'x':
	if (s+1 >= s_end) {
	    rb_raise(rb_eArgError, "invalid hex escape");
	}
	c2 = ruby_scan_hex(s+1, 2, &hexlen);
	if (hexlen != 2) {
	    rb_raise(rb_eArgError, "invalid hex escape");
	}
	*buf = (char)c2;
	rb_str_cat(undumped, buf, 1L);
	n += rb_strlen_lit("xXX");
	break;
      case '#':
	if (s+1 >= s_end) {
	    rb_str_cat(undumped, s, 1L); /* just '#' */
	    n++;
	    break;
	}
	n2 = rb_enc_mbclen(s+1, s_end, *penc);
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

/*
 *  call-seq:
 *     str.undump_roughly   -> new_str
 *
 *  Produces unescaped version of +str+.
 *  See also String#dump because String#undump_roughly does inverse of String#dump.
 *
 *    "\"hello \\n ''\"".undump_roughly #=> "hello \n ''"
 */

static VALUE
str_undump_roughly(VALUE str)
{
    const char *s = RSTRING_PTR(str);
    const char *s_end = RSTRING_END(str);
    long len = RSTRING_LEN(str);
    rb_encoding *enc = rb_enc_get(str), *forced_enc;
    int n;
    unsigned int c;
    enum undump_source_format source_format;
    VALUE undumped = rb_enc_str_new(s, 0L, enc);
    VALUE forced_enc_str;
    long forced_enc_str_len;

    rb_must_asciicompat(str);

    source_format = check_undump_source_format(s, s_end, len, enc,
					       &forced_enc_str, &forced_enc_str_len);
    if (source_format == UNDUMP_SOURCE_INVALID) {
	rb_raise(rb_eArgError, "not wrapped with '\"' nor '\"...\".force_encoding(\"...\")' form");
    }
    if (source_format == UNDUMP_SOURCE_FORCE_ENCODING) {
	forced_enc = rb_to_encoding(forced_enc_str);
    }

    /* strip '"' at the start */
    s++;
    if (source_format == UNDUMP_SOURCE_SIMPLE) {
	/* strip '"' at the end */
	s_end--;
    } else { /* source_format == UNDUMP_SOURCE_FORCE_ENCODING */
	/* strip '".force_encoding("...")' */
	s_end -= rb_strlen_lit("\".force_encoding(\"\")") + forced_enc_str_len;
    }

    for (; s < s_end; s += n) {
	c = rb_enc_codepoint_len(s, s_end, &n, enc);
	if (c == '\\') {
	    if (s+1 >= s_end) {
		rb_raise(rb_eArgError, "invalid escape");
	    }
	    n = undump_after_backslash(undumped, s+1, s_end, &enc);
	}
	else {
	    rb_str_cat(undumped, s, n);
	}
    }

    if (source_format == UNDUMP_SOURCE_FORCE_ENCODING) {
	rb_enc_associate(undumped, forced_enc);
	ENC_CODERANGE_CLEAR(undumped);
    }
    OBJ_INFECT(undumped, str);
    return undumped;
}

void
Init_string_undump(void)
{
    rb_define_method(rb_cString, "undump_roughly", str_undump_roughly, 0);
}
