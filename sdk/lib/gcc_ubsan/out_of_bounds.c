#include "ubsan.h"

void __ubsan_handle_out_of_bounds(struct SourceLocation* SourceLoc, void* Index)
{
    ubsan_fail(SourceLoc, "out of bounds access", "Index: %p", Index);
}
