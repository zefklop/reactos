
#include "ubsan.h"

void __ubsan_handle_builtin_unreachable(struct SourceLocation* SourceLoc)
{
    ubsan_fail(SourceLoc, "Unreachable code was reached", "");
    __builtin_unreachable();
}
