/*
*
*   Copyright (c) 2001, Nick Hibma <n_hibma@qubesoft.com>
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for YACC language files.
*/

/*
*   INCLUDE FILES
*/
#include "general.h"	/* must always come first */

#include <string.h>
#include "parse.h"

/*
*   FUNCTION DEFINITIONS
*/

static void installYaccRegex (const langType language)
{
    addTagRegex (language,
        "^([A-Za-z][A-Za-z_0-9]+)[ \t]+:", "\\1", "l,label", NULL);
}

extern parserDefinition* YaccParser ()
{
    static const char *const extensions [] = { "y", NULL };
    parserDefinition* const def = parserNew ("YACC");
    def->extensions = extensions;
    def->initialize = installYaccRegex;
    def->regex      = TRUE;
    return def;
}

/* vi:set tabstop=8 shiftwidth=4: */
