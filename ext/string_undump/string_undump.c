#include "string_undump.h"

static VALUE
str_undump_roughly(VALUE str)
{
    return str;
}

void
Init_string_undump(void)
{
    rb_define_method(rb_cString, "undump_roughly", str_undump_roughly, 0);
}
