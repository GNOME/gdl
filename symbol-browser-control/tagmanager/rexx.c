/*
*
*   Copyright (c) 2001, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for the REXX language
*   (http://www.rexxla.org, http://www2.hursley.ibm.com/rexx).
*/

/*
*   INCLUDE FILES
*/
#include "general.h"    /* always include first */
#include "parse.h"      /* always include */

/*
*   FUNCTION DEFINITIONS
*/

static void installRexxRegex (const langType language)
{
    addTagRegex (language, "^([A-Za-z0-9@#$\\.!?_]+)[ \t]*:",
	"\\1", "s,subroutine", NULL);
}

extern parserDefinition* RexxParser (void)
{
    static const char *const extensions [] = { "cmd", "rexx", "rx", NULL };
    parserDefinition* const def = parserNew ("REXX");
    def->extensions = extensions;
    def->initialize = installRexxRegex;
    def->regex      = TRUE;
    return def;
}

/* vi:set tabstop=8 shiftwidth=4: */
