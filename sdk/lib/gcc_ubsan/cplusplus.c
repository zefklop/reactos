
#include "ubsan.h"

ULONG_PTR __ubsan_vptr_type_cache[128];

void __ubsan_handle_dynamic_type_cache_miss(struct SourceLocation* SourceLoc, void* Pointer, ULONG_PTR Hash)
{
    /* Do nothing for now, this is not trivial */
}
