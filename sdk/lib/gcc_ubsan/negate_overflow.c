
#include "ubsan.h"

void __ubsan_handle_negate_overflow(struct SourceLocation* SourceLoc, void* OldValue)
{
    ubsan_fail(SourceLoc, "Integer negate overflow", "Oldvalue: %p", OldValue);
}
