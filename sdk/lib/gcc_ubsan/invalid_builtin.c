
#include "ubsan.h"

struct InvalidBuiltinData {
  struct SourceLocation Loc;
  unsigned char Kind;
};

void __ubsan_handle_invalid_builtin(struct InvalidBuiltinData* Data)
{
    ubsan_fail(&Data->Loc, "Invalid builtin", "passing zero to %s, which is not a valid argument", (Data->Kind == 0) ? "ctz()" : "clz()");
}
