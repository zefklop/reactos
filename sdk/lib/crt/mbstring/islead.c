#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
#undef isleadbyte
int isleadbyte(int c)
{
    return _isctype( c, _LEADBYTE );

}
