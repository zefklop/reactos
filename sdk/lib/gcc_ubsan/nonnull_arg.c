#include "ubsan.h"

struct NonNullArgData {
  struct SourceLocation Loc;
  struct SourceLocation AttrLoc;
  int ArgIndex;
};

void __ubsan_handle_nonnull_arg(struct NonNullArgData* Data)
{
    ubsan_fail(&Data->Loc, "null arg", "Arg index: %d, defined at %s:%d:%d",
               Data->ArgIndex, Data->AttrLoc.Filename ? Data->AttrLoc.Filename : "(null)",
               Data->AttrLoc.Line, Data->AttrLoc.Column);
}
