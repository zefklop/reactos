
#include "ubsan.h"

void __ubsan_handle_load_invalid_value(struct SourceLocation* SourceLoc, void* Value)
{
    ubsan_fail(SourceLoc, "Loading invalid value", "Value: %p", Value);
}
