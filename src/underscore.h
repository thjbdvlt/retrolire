/* underscore commands
 * -------------------
 *
 * commands used by other commands, such as 'quote', or
 * functions prefixed by an underscore, such as '_quote'.
 *
 * */

#ifndef _UNDERSCORE_H
#define _UNDERSCORE_H

#include "sizes.h"

/* add tags or file to an entry. */
int
underscore_add(char* pos[VAL_SIZE]);

/* parse underscore arguments. */
void
do_underscore(char* argv[]);

#endif
