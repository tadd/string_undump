#ifndef EXT_STRING_UNDUMP_H_INCLUDED
#define EXT_STRING_UNDUMP_H_INCLUDED 1

#include <ruby.h>
#include <ruby/encoding.h>

#ifndef FALSE
# define FALSE 0
#elif FALSE
# error FALSE must be false
#endif
#ifndef TRUE
# define TRUE 1
#elif !TRUE
# error TRUE must be true
#endif

#endif /* EXT_STRING_UNDUMP_H_INCLUDED */
