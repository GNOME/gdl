/*
*
*   Copyright (c) 1998-2001, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for Fortran language
*   files.
*/

/*
*   INCLUDE FILES
*/
#include "general.h"	/* must always come first */

#include <string.h>
#include <limits.h>
#include <ctype.h>	/* to define tolower () */
#include <setjmp.h>

#include "debug.h"
#include "entry.h"
#include "keyword.h"
#include "main.h"
#include "options.h"
#include "parse.h"
#include "read.h"
#include "vstring.h"

/*
*   MACROS
*/
#define isident(c)		(isalnum(c) || (c) == '_')
#define isBlank(c)		(boolean) (c == ' ' || c == '\t')
#define isType(token,t)		(boolean) ((token)->type == (t))
#define isKeyword(token,k)	(boolean) ((token)->keyword == (k))

/*
*   DATA DECLARATIONS
*/

typedef enum eException {
    ExceptionNone, ExceptionEOF, ExceptionFixedFormat
} exception_t;

/*  Used to designate type of line read in fixed source form.
 */
typedef enum eFortranLineType {
    LTYPE_UNDETERMINED,
    LTYPE_INVALID,
    LTYPE_COMMENT,
    LTYPE_CONTINUATION,
    LTYPE_EOF,
    LTYPE_INITIAL,
    LTYPE_SHORT
} lineType;

/*  Used to specify type of keyword.
 */
typedef enum eKeywordId {
    KEYWORD_NONE,
    KEYWORD_allocatable,
    KEYWORD_assignment,
    KEYWORD_block,
    KEYWORD_character,
    KEYWORD_common,
    KEYWORD_complex,
    KEYWORD_contains,
    KEYWORD_data,
    KEYWORD_dimension,
    KEYWORD_do,
    KEYWORD_double,
    KEYWORD_end,
    KEYWORD_entry,
    KEYWORD_equivalence,
    KEYWORD_external,
    KEYWORD_format,
    KEYWORD_function,
    KEYWORD_if,
    KEYWORD_implicit,
    KEYWORD_include,
    KEYWORD_integer,
    KEYWORD_intent,
    KEYWORD_interface,
    KEYWORD_intrinsic,
    KEYWORD_logical,
    KEYWORD_module,
    KEYWORD_namelist,
    KEYWORD_operator,
    KEYWORD_optional,
    KEYWORD_parameter,
    KEYWORD_pointer,
    KEYWORD_precision,
    KEYWORD_private,
    KEYWORD_program,
    KEYWORD_public,
    KEYWORD_real,
    KEYWORD_recursive,
    KEYWORD_save,
    KEYWORD_select,
    KEYWORD_sequence,
    KEYWORD_subroutine,
    KEYWORD_target,
    KEYWORD_type,
    KEYWORD_use,
    KEYWORD_where
} keywordId;

/*  Used to determine whether keyword is valid for the token language and
 *  what its ID is.
 */
typedef struct sKeywordDesc {
    const char *name;
    keywordId id;
} keywordDesc;

typedef enum eTokenType {
    TOKEN_UNDEFINED,
    TOKEN_COMMA,
    TOKEN_DOUBLE_COLON,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_LABEL,
    TOKEN_NUMERIC,
    TOKEN_OPERATOR,
    TOKEN_PAREN_CLOSE,
    TOKEN_PAREN_OPEN,
    TOKEN_STATEMENT_END,
    TOKEN_STRING
} tokenType;

typedef enum eTagType {
    TAG_UNDEFINED = -1,
    TAG_BLOCK_DATA,
    TAG_COMMON_BLOCK,
    TAG_ENTRY_POINT,
    TAG_FUNCTION,
    TAG_INTERFACE,
    TAG_COMPONENT,
    TAG_LABEL,
    TAG_LOCAL,
    TAG_MODULE,
    TAG_NAMELIST,
    TAG_PROGRAM,
    TAG_SUBROUTINE,
    TAG_DERIVED_TYPE,
    TAG_VARIABLE,
    TAG_COUNT		/* must be last */
} tagType;

typedef struct sTokenInfo {
    tokenType type;
    keywordId keyword;
    tagType tag;
    vString* string;
    unsigned long lineNumber;
    fpos_t filePosition;
} tokenInfo;

/*
*   DATA DEFINITIONS
*/

static langType Lang_fortran;
static jmp_buf Exception;
static int Ungetc = '\0';
static unsigned int Column = 0;
static boolean FreeSourceForm = FALSE;
static tokenInfo *Parent = NULL;

/* indexed by tagType */
static kindOption FortranKinds [] = {
    { TRUE,  'b', "block data", "block data"},
    { TRUE,  'c', "common",     "common blocks"},
    { TRUE,  'e', "entry",      "entry points"},
    { TRUE,  'f', "function",   "functions"},
    { TRUE,  'i', "interface",  "interfaces"},
    { TRUE,  'k', "component",  "type components"},
    { TRUE,  'l', "label",      "labels"},
    { FALSE, 'L', "local",      "local and common block variables"},
    { TRUE,  'm', "module",     "modules"},
    { TRUE,  'n', "namelist",   "namelists"},
    { TRUE,  'p', "program",    "programs"},
    { TRUE,  's', "subroutine", "subroutines"},
    { TRUE,  't', "type",       "derived types"},
    { TRUE,  'v', "variable",   "module variables"}
};

static const keywordDesc FortranKeywordTable [] = {
    /* keyword		keyword ID */
    { "allocatable",	KEYWORD_allocatable	},
    { "assignment",	KEYWORD_assignment	},
    { "block",		KEYWORD_block		},
    { "character",	KEYWORD_character	},
    { "common",		KEYWORD_common		},
    { "complex",	KEYWORD_complex		},
    { "contains",	KEYWORD_contains	},
    { "data",		KEYWORD_data		},
    { "dimension",	KEYWORD_dimension	},
    { "do",		KEYWORD_do		},
    { "double",		KEYWORD_double		},
    { "end",		KEYWORD_end		},
    { "entry",		KEYWORD_entry		},
    { "equivalence",	KEYWORD_equivalence	},
    { "external",	KEYWORD_external	},
    { "format",		KEYWORD_format		},
    { "function",	KEYWORD_function	},
    { "if",		KEYWORD_if		},
    { "implicit",	KEYWORD_implicit	},
    { "include",	KEYWORD_include		},
    { "instrinsic",	KEYWORD_intrinsic	},
    { "integer",	KEYWORD_integer		},
    { "intent",		KEYWORD_intent		},
    { "interface",	KEYWORD_interface	},
    { "logical",	KEYWORD_logical		},
    { "module",		KEYWORD_module		},
    { "namelist",	KEYWORD_namelist	},
    { "operator",	KEYWORD_operator	},
    { "optional",	KEYWORD_optional	},
    { "parameter",	KEYWORD_parameter	},
    { "pointer",	KEYWORD_pointer		},
    { "precision",	KEYWORD_precision	},
    { "private",	KEYWORD_private		},
    { "program",	KEYWORD_program		},
    { "public",		KEYWORD_public		},
    { "real",		KEYWORD_real		},
    { "recursive",	KEYWORD_recursive	},
    { "save",		KEYWORD_save		},
    { "select",		KEYWORD_select		},
    { "sequence",	KEYWORD_sequence	},
    { "subroutine",	KEYWORD_subroutine	},
    { "target",		KEYWORD_target		},
    { "type",		KEYWORD_type		},
    { "use",		KEYWORD_use		},
    { "where",		KEYWORD_where		}
};

static struct {
    unsigned int count;
    unsigned int max;
    tokenInfo* list;
} Ancestors = { 0, 0, NULL };

/*
*   FUNCTION PROTOTYPES
*/
static void parseDerivedTypeDef (tokenInfo *const token);
static void parseFunctionSubprogram (tokenInfo *const token);
static void parseSubroutineSubprogram (tokenInfo *const token);

/*
*   FUNCTION DEFINITIONS
*/

static void ancestorPush (tokenInfo *const token)
{
    enum { incrementalIncrease = 10 };
    if (Ancestors.list == NULL)
    {
	Assert (Ancestors.max == 0);
	Ancestors.count = 0;
	Ancestors.max   = incrementalIncrease;
	Ancestors.list  = xMalloc (Ancestors.max, tokenInfo);
    }
    else if (Ancestors.count == Ancestors.max)
    {
	Ancestors.max += incrementalIncrease;
	Ancestors.list = xRealloc (Ancestors.list, Ancestors.max, tokenInfo);
    }
    Ancestors.list [Ancestors.count] = *token;
    Ancestors.list [Ancestors.count].string = vStringNewCopy (token->string);
    Ancestors.count++;
}

static void ancestorPop (void)
{
    Assert (Ancestors.count > 0);
    --Ancestors.count;
    vStringDelete (Ancestors.list [Ancestors.count].string);

    Ancestors.list [Ancestors.count].type       = TOKEN_UNDEFINED;
    Ancestors.list [Ancestors.count].keyword    = KEYWORD_NONE;
    Ancestors.list [Ancestors.count].tag        = TAG_UNDEFINED;
    Ancestors.list [Ancestors.count].string     = NULL;
    Ancestors.list [Ancestors.count].lineNumber = 0L;
}

static const tokenInfo* ancestorTop (void)
{
    Assert (Ancestors.count > 0);
    return &Ancestors.list [Ancestors.count - 1];
}

#define ancestorCount() (Ancestors.count)

static void ancestorClear (void)
{
    while (Ancestors.count > 0)
	ancestorPop ();
    if (Ancestors.list != NULL)
	eFree (Ancestors.list);
    Ancestors.list = NULL;
    Ancestors.count = 0;
    Ancestors.max = 0;
}

static void buildFortranKeywordHash (void)
{
    const size_t count = sizeof (FortranKeywordTable) /
			 sizeof (FortranKeywordTable [0]);
    size_t i;
    for (i = 0  ;  i < count  ;  ++i)
    {
	const keywordDesc* const p = &FortranKeywordTable [i];
	addKeyword (p->name, Lang_fortran, (int) p->id);
    }
}

/*
*   Tag generation functions
*/

static boolean isFileScope (const tagType type)
{
    return (boolean) (type == TAG_LABEL || type == TAG_LOCAL);
}

static boolean includeTag (const tagType type)
{
    boolean include;
    Assert (type != TAG_UNDEFINED);
    include = FortranKinds [(int) type].enabled;
    if (include && isFileScope (type))
	include = Option.include.fileScope;
    return include;
}

static void makeFortranTag (tokenInfo *const token, tagType tag)
{
    token->tag = tag;
    if (includeTag (token->tag))
    {
	const char *const name = vStringValue (token->string);
	tagEntryInfo e;

	initTagEntry (&e, name);

	if (token->tag == TAG_COMMON_BLOCK)
	    e.lineNumberEntry = (boolean) (Option.locate != EX_PATTERN);

	e.lineNumber	= token->lineNumber;
	e.filePosition	= token->filePosition;
	e.isFileScope	= isFileScope (token->tag);
	e.kindName	= FortranKinds [token->tag].name;
	e.kind		= FortranKinds [token->tag].letter;
	e.truncateLine	= (boolean) (token->tag != TAG_LABEL);

	if (ancestorCount () > 0)
	{
	    const tokenInfo* const parent = ancestorTop ();
	    e.extensionFields.scope [0] = FortranKinds [parent->tag].name;
	    e.extensionFields.scope [1] = vStringValue (parent->string);
	}
	makeTagEntry (&e);
    }
}

/*
*   Parsing functions
*/

static int skipLine (void)
{
    int c;

    do
	c = fileGetc ();
    while (c != EOF  &&  c != '\n');

    return c;
}

static void makeLabelTag (vString *const label)
{
    tokenInfo token;

    token.type		= TOKEN_LABEL;
    token.keyword	= KEYWORD_NONE;
    token.tag		= TAG_LABEL;
    token.string	= label;
    token.lineNumber	= getSourceLineNumber ();
    token.filePosition	= getInputFilePosition ();

    makeFortranTag (&token, TAG_LABEL);
}

static lineType getLineType (void)
{
    static vString *label = NULL;
    int column = 0;
    lineType type = LTYPE_UNDETERMINED;

    if (label == NULL)
	label = vStringNew ();

    do		/* read in first 6 "margin" characters */
    {
	int c = fileGetc ();

	/* 3.2.1  Comment_Line.  A comment line is any line that contains
	 * a C or an asterisk in column 1, or contains only blank characters
	 * in  columns 1 through 72.  A comment line that contains a C or
	 * an asterisk in column 1 may contain any character capable  of
	 * representation in the processor in columns 2 through 72.
	 */
	/*  EXCEPTION! Some compilers permit '!' as a commment character here.
	 *
	 *  Treat '#' in column 1 as comment to permit preprocessor directives.
	 */
	if (column == 0  &&  strchr ("*Cc!#", c) != NULL)
	    type = LTYPE_COMMENT;
	else if (c == '\t')  /* EXCEPTION! Some compilers permit a tab here */
	{
	    column = 8;
	    type = LTYPE_INITIAL;
	}
	else if (column == 5)
	{
	    /* 3.2.2  Initial_Line.  An initial line is any line that is not
	     * a comment line and contains the character blank or the digit 0
	     * in column 6.  Columns 1 through 5 may contain a statement label
	     * (3.4), or each of the columns 1 through 5 must contain the
	     * character blank.
	     */
	    if (c == ' '  ||  c == '0')
		type = LTYPE_INITIAL;

	    /* 3.2.3  Continuation_Line.  A continuation line is any line that
	     * contains any character of the FORTRAN character set other than
	     * the character blank or the digit 0 in column 6 and contains
	     * only blank characters in columns 1 through 5.
	     */
	    else if (vStringLength (label) == 0)
		type = LTYPE_CONTINUATION;
	    else
		type = LTYPE_INVALID;
	}
	else if (c == ' ')
	    ;
	else if (c == EOF)
	    type = LTYPE_EOF;
	else if (c == '\n')
	    type = LTYPE_SHORT;
	else if (isdigit (c))
	    vStringPut (label, c);
	else
	    type = LTYPE_INVALID;

	++column;
    } while (column < 6  &&  type == LTYPE_UNDETERMINED);

    Assert (type != LTYPE_UNDETERMINED);

    if (vStringLength (label) > 0)
    {
	vStringTerminate (label);
	makeLabelTag (label);
	vStringClear (label);
    }
    return type;
}

static int getFixedFormChar (void)
{
    boolean newline = FALSE;
    lineType type;
    int c = '\0';

    if (Column > 0)
    {
#ifdef STRICT_FIXED_FORM
	/*  EXCEPTION! Some compilers permit more than 72 characters per line.
	 */
	if (Column > 71)
	    c = skipLine ();
	else
#endif
	{
	    c = fileGetc ();
	    ++Column;
	}
	if (c == '\n')
	{
	    newline = TRUE;	/* need to check for continuation line */
	    Column = 0;
	}
	else if (c == '&')	/* check for free source form */
	{
	    const int c2 = fileGetc ();
	    if (c2 == '\n')
		longjmp (Exception, (int) ExceptionFixedFormat);
	    else
		fileUngetc (c2);
	}
    }
    while (Column == 0)
    {
	type = getLineType ();
	switch (type)
	{
	    case LTYPE_UNDETERMINED:
	    case LTYPE_INVALID:
		longjmp (Exception, (int) ExceptionFixedFormat);
		break;

	    case LTYPE_SHORT: break;
	    case LTYPE_COMMENT: skipLine (); break;

	    case LTYPE_EOF:
		Column = 6;
		if (newline)
		    c = '\n';
		else
		    c = EOF;
		break;

	    case LTYPE_INITIAL:
		if (newline)
		{
		    c = '\n';
		    Column = 6;
		    break;
		}
		/* fall through to next case */
	    case LTYPE_CONTINUATION:
		Column = 5;
		do
		{
		    c = fileGetc ();
		    ++Column;
		} while (isBlank (c));
		if (c == '\n')
		    Column = 0;
		else if (Column > 6)
		{
		    fileUngetc (c);
		    c = ' ';
		}
		break;

	    default:
		Assert ("Unexpected line type" == NULL);
	}
    }
    return c;
}

static int skipToNextLine (void)
{
    int c = skipLine ();
    if (c != EOF)
	c = fileGetc ();
    return c;
}

static int getFreeFormChar (void)
{
    static boolean newline = TRUE;
    boolean recurse = FALSE;
    int c = fileGetc ();

    if (c == '&')		/* handle line continuation */
    {
	recurse = TRUE;
	c = fileGetc ();
    }
    else if (newline && (c == '!' || c == '#'))
	recurse = TRUE;
    while (recurse)
    {
	while (isspace (c))
	    c = fileGetc ();
	while (c == '!' || (newline && c == '#'))
	{
	    c = skipToNextLine ();
	    newline = TRUE;
	}
	if (c == '&')
	    c = fileGetc ();
	else
	    recurse = FALSE;
    }
    newline = (boolean) (c == '\n');
    return c;
}

static int getChar (void)
{
    int c;

    if (Ungetc != '\0')
    {
	c = Ungetc;
	Ungetc = '\0';
    }
    else if (FreeSourceForm)
	c = getFreeFormChar ();
    else
	c = getFixedFormChar ();

    return c;
}

static void ungetChar (const int c)
{
    Ungetc = c;
}

/*  If a numeric is passed in 'c', this is used as the first digit of the
 *  numeric being parsed.
 */
static vString *parseInteger (int c)
{
    static vString *string = NULL;

    if (string == NULL)
	string = vStringNew ();
    vStringClear (string);

    if (c == '-')
    {
	vStringPut (string, c);
	c = getChar ();
    }
    else if (! isdigit (c))
	c = getChar ();
    while (c != EOF  &&  isdigit (c))
    {
	vStringPut (string, c);
	c = getChar ();
    }
    vStringTerminate (string);

    if (c == '_')
    {
	do
	    c = getChar ();
	while (c != EOF  &&  isalpha (c));
    }
    ungetChar (c);

    return string;
}

static vString *parseNumeric (int c)
{
    static vString *string = NULL;

    if (string == NULL)
	string = vStringNew ();
    vStringCopy (string, parseInteger (c));

    c = getChar ();
    if (c == '.')
    {
	vStringPut (string, c);
	vStringCat (string, parseInteger ('\0'));
	c = getChar ();
    }
    if (tolower (c) == 'e')
    {
	vStringPut (string, c);
	vStringCat (string, parseInteger ('\0'));
    }
    else
	ungetChar (c);

    vStringTerminate (string);

    return string;
}

static void parseString (vString *const string, const int delimeter)
{
    const unsigned long inputLineNumber = getInputLineNumber ();
    int c = getChar ();

    while (c != delimeter  &&  c != '\n'  &&  c != EOF)
    {
	vStringPut (string, c);
	c = getChar ();
    }
    if (c == '\n'  ||  c == EOF)
    {
	verbose ("%s: unterminated character string at line %lu\n",
		getInputFileName (), inputLineNumber);
	if (c == EOF)
	    longjmp (Exception, (int) ExceptionEOF);
	else if (! FreeSourceForm)
	    longjmp (Exception, (int) ExceptionFixedFormat);
    }
    vStringTerminate (string);
}

/*  Read a C identifier beginning with "firstChar" and places it into "name".
 */
static void parseIdentifier (vString *const string, const int firstChar)
{
    int c = firstChar;

    do
    {
	vStringPut (string, c);
	c = getChar ();
    } while (isident (c));

    vStringTerminate (string);
    ungetChar (c);		/* unget non-identifier character */
}

static tokenInfo *newToken (void)
{
    tokenInfo *const token = xMalloc (1, tokenInfo);

    token->type	   = TOKEN_UNDEFINED;
    token->keyword = KEYWORD_NONE;
    token->tag	   = TAG_UNDEFINED;
    token->string  = vStringNew ();
    token->lineNumber   = getSourceLineNumber ();
    token->filePosition	= getInputFilePosition ();

    return token;
}

static void deleteToken (tokenInfo *const token)
{
    vStringDelete (token->string);
    eFree (token);
}

/*  Analyzes the identifier contained in a statement described by the
 *  statement structure and adjusts the structure according the significance
 *  of the identifier.
 */
static keywordId analyzeToken (vString *const name)
{
    static vString *keyword = NULL;
    keywordId id;

    if (keyword == NULL)
	keyword = vStringNew ();
    vStringCopyToLower (keyword, name);
    id = (keywordId) lookupKeyword (vStringValue (keyword), Lang_fortran);

    return id;
}

static void checkForLabel (void)
{
    tokenInfo* token = NULL;
    int length;
    int c;

    do
	c = getChar ();
    while (isBlank (c));

    for (length = 0  ;  isdigit (c)  &&  length < 5  ;  ++length)
    {
	if (token == NULL)
	{
	    token = newToken ();
	    token->type = TOKEN_LABEL;
	}
	vStringPut (token->string, c);
	c = getChar ();
    }
    if (length > 0)
    {
	Assert (token != NULL);
	vStringTerminate (token->string);
	makeFortranTag (token, TAG_LABEL);
	deleteToken (token);
    }
    ungetChar (c);
}

static void readToken (tokenInfo *const token)
{
    int c;

    token->type    = TOKEN_UNDEFINED;
    token->tag     = TAG_UNDEFINED;
    token->keyword = KEYWORD_NONE;
    vStringClear (token->string);

getNextChar:
    token->lineNumber	= getSourceLineNumber ();
    token->filePosition	= getInputFilePosition ();

    c = getChar ();

    switch (c)
    {
	case EOF:  longjmp (Exception, (int) ExceptionEOF);	break;
	case ' ':  goto getNextChar;
	case '\t': goto getNextChar;
	case ',':  token->type = TOKEN_COMMA;			break;
	case '(':  token->type = TOKEN_PAREN_OPEN;		break;
	case ')':  token->type = TOKEN_PAREN_CLOSE;		break;

	case '*':
	case '/':
	case '+':
	case '-':
	case '=':
	case '<':
	case '>':
	{
	    const char *const operatorChars = "*/+-=<>";

	    do {
		vStringPut (token->string, c);
		c = getChar ();
	    } while (strchr (operatorChars, c) != NULL);
	    ungetChar (c);
	    vStringTerminate (token->string);
	    token->type = TOKEN_OPERATOR;
	    break;
	}

	case '!':
	    if (FreeSourceForm)
	    {
		do
		   c = getChar ();
		while (c != '\n');
	    }
	    else
	    {
		skipLine ();
		Column = 0;
	    }
	    /* fall through to newline case */
	case '\n':
	    token->type = TOKEN_STATEMENT_END;
	    if (FreeSourceForm)
		checkForLabel ();
	    break;

	case '.':
	    parseIdentifier (token->string, c);
	    c = getChar ();
	    if (c == '.')
	    {
		vStringPut (token->string, c);
		vStringTerminate (token->string);
		token->type = TOKEN_OPERATOR;
	    }
	    else
	    {
		ungetChar (c);
		token->type = TOKEN_UNDEFINED;
	    }
	    break;

	case ':':
	    if (getChar () == ':')
		token->type = TOKEN_DOUBLE_COLON;
	    else
		token->type = TOKEN_UNDEFINED;
	    break;

	default:
	    if (isalpha (c))
	    {
		parseIdentifier (token->string, c);
		token->keyword = analyzeToken (token->string);
		if (isKeyword (token, KEYWORD_NONE))
		    token->type = TOKEN_IDENTIFIER;
		else
		    token->type = TOKEN_KEYWORD;
	    }
	    else if (isdigit (c))
	    {
		vStringCat (token->string, parseNumeric (c));
		token->type = TOKEN_NUMERIC;
	    }
	    else if (c == '"'  ||  c == '\'')
	    {
		parseString (token->string, c);
		token->type = TOKEN_STRING;
	    }
	    else if (c == ';'  &&  FreeSourceForm)
		token->type = TOKEN_STATEMENT_END;
	    else
		token->type = TOKEN_UNDEFINED;
	    break;
    }
}

/*
*   Scanning functions
*/

static void skipToToken (tokenInfo *const token, tokenType type)
{
    while (! isType (token, type) && ! isType (token, TOKEN_STATEMENT_END))
	readToken (token);
}

static void skipPast (tokenInfo *const token, tokenType type)
{
    skipToToken (token, type);
    if (! isType (token, TOKEN_STATEMENT_END))
	readToken (token);
}

static void skipToNextStatement (tokenInfo *const token)
{
    do
    {
	skipToToken (token, TOKEN_STATEMENT_END);
	readToken (token);
    } while (isType (token, TOKEN_STATEMENT_END));
}

static boolean isTypeSpec (tokenInfo *const token)
{
    boolean result;
    switch (token->keyword)
    {
	case KEYWORD_integer:
	case KEYWORD_real:
	case KEYWORD_double:
	case KEYWORD_complex:
	case KEYWORD_character:
	case KEYWORD_logical:
	case KEYWORD_type:
	    result = TRUE;
	    break;
	default:
	    result = FALSE;
	    break;
    }
    return result;
}

/*  type-spec
 *      is INTEGER [kind-selector]
 *      or REAL [kind-selector] is ( etc. )
 *      or DOUBLE PRECISION
 *      or COMPLEX [kind-selector]
 *      or CHARACTER [kind-selector]
 *      or LOGICAL [kind-selector]
 *      or TYPE ( type-name )
 *
 *  Note that INTEGER and REAL may be followed by "*N" where "N" is an integer
 */
static void parseTypeSpec (tokenInfo *const token)
{
    /* parse type-spec, leaving `token' at first token following type-spec */
    Assert (isTypeSpec (token));
    switch (token->keyword)
    {
	case KEYWORD_integer:
	case KEYWORD_real:
	case KEYWORD_complex:
	case KEYWORD_character:
	case KEYWORD_logical:
	    readToken (token);
	    if (isType (token, TOKEN_PAREN_OPEN))
		skipPast (token, TOKEN_PAREN_CLOSE); /* skip kind-selector */
	    else if (isType (token, TOKEN_OPERATOR) &&
		     strcmp (vStringValue (token->string), "*") == 0)
	    {
		readToken (token);
		readToken (token);
	    }
	    break;

	case KEYWORD_double:
	    readToken (token);
	    if (! isKeyword (token, KEYWORD_precision))
		skipToToken (token, TOKEN_STATEMENT_END);
	    break;

	case KEYWORD_type:
	    readToken (token);
	    if (isType (token, TOKEN_PAREN_OPEN))
		skipPast (token, TOKEN_PAREN_CLOSE); /* skip type-name */
	    else
		parseDerivedTypeDef (token);
	    break;

	default:
	    skipToToken (token, TOKEN_STATEMENT_END);
	    break;
    }
}

/* skip over parenthesis enclosed contents starting at next token.
 * Token refers to first token following closing parenthesis. If an opening
 * parenthesis is not found, `token' is moved to the end of the statement.
 */
static void skipOverParens (tokenInfo *const token)
{
    if (isType (token, TOKEN_PAREN_OPEN))
	skipPast (token, TOKEN_PAREN_CLOSE);
}

static boolean skipStatementIfKeyword (tokenInfo *const token, keywordId keyword)
{
    boolean result = FALSE;
    if (isKeyword (token, keyword))
    {
	result = TRUE;
	skipToNextStatement (token);
    }
    return result;
}

static boolean isMatchingEnd (tokenInfo *const token, keywordId keyword)
{
    boolean result = FALSE;
    if (isKeyword (token, KEYWORD_end))
    {
	readToken (token);
	result = (boolean) (isKeyword (token, KEYWORD_NONE) ||
			    isKeyword (token, keyword));
    }
    return result;
}

/* parse a list of qualifying specifiers, leaving `token' at first token
 * following list. Examples of such specifiers are:
 *      [[, attr-spec] ::]
 *      [[, component-attr-spec-list] ::]
 *
 *  attr-spec
 *      is PARAMETER
 *      or access-spec (is PUBLIC or PRIVATE)
 *      or ALLOCATABLE
 *      or DIMENSION ( array-spec )
 *      or EXTERNAL
 *      or INTENT ( intent-spec )
 *      or INTRINSIC
 *      or OPTIONAL
 *      or POINTER
 *      or SAVE
 *      or TARGET
 * 
 *  component-attr-spec
 *      is POINTER
 *      or DIMENSION ( component-array-spec )
 */
static void parseQualifierSpecList (tokenInfo *const token)
{
    do
    {
	readToken (token);	/* should be an attr-spec */
	switch (token->keyword)
	{
	    case KEYWORD_parameter:
	    case KEYWORD_allocatable:
	    case KEYWORD_external:
	    case KEYWORD_intrinsic:
	    case KEYWORD_optional:
	    case KEYWORD_private:
	    case KEYWORD_pointer:
	    case KEYWORD_public:
	    case KEYWORD_save:
	    case KEYWORD_target:
		readToken (token);
		break;

	    case KEYWORD_dimension:
	    case KEYWORD_intent:
		readToken (token);
		skipOverParens (token);
		break;

	    default: skipToToken (token, TOKEN_STATEMENT_END); break;
	}
    } while (isType (token, TOKEN_COMMA));
    if (! isType (token, TOKEN_DOUBLE_COLON))
	skipToToken (token, TOKEN_STATEMENT_END);
}

static boolean localVariableScope (void)
{
    boolean result = TRUE;
    if (ancestorCount () > 0)
    {
	const tokenInfo* const parent = ancestorTop ();
	result = (boolean) (parent->tag != TAG_MODULE);
    }
    return result;
}

/*  type-declaration-stmt is
 *      type-spec [[, attr-spec] ... ::] entity-decl-list
 */
static void parseTypeDeclarationStmt (tokenInfo *const token)
{
    const tagType tag = localVariableScope () ? TAG_LOCAL : TAG_VARIABLE;
    Assert (isTypeSpec (token));
    parseTypeSpec (token);
    if (isType (token, TOKEN_COMMA))
	parseQualifierSpecList (token);
    if (isType (token, TOKEN_DOUBLE_COLON))
	readToken (token);
    do
    {
	if (isType (token, TOKEN_IDENTIFIER))
	    makeFortranTag (token, tag);
	skipPast (token, TOKEN_COMMA);
    } while (! isType (token, TOKEN_STATEMENT_END));
    skipToNextStatement (token);
}

static void parseParenName (tokenInfo *const token)
{
    readToken (token);
    if (isType (token, TOKEN_PAREN_OPEN))
	readToken (token);
}

/*  common-stmt is
 *      COMMON [/[common-block-name]/] common-block-object-list [[,]/[common-block-name]/ common-block-object-list] ...
 *
 *  common-block-object is
 *      variable-name [ ( explicit-shape-spec-list ) ]
 */
static void parseCommonStmt (tokenInfo *const token)
{
    Assert (isKeyword (token, KEYWORD_common));
    readToken (token);
    do
    {
	if (isType (token, TOKEN_OPERATOR))
	{
	    readToken (token);
	    if (isType (token, TOKEN_IDENTIFIER))
		makeFortranTag (token, TAG_COMMON_BLOCK);
	    skipPast (token, TOKEN_OPERATOR);
	}
	if (isType (token, TOKEN_IDENTIFIER))
	    makeFortranTag (token, TAG_LOCAL);
	skipPast (token, TOKEN_COMMA);
    } while (! isType (token, TOKEN_STATEMENT_END));
    skipToNextStatement (token);
}

static void tagSlashName (tokenInfo *const token, const tagType type)
{
    readToken (token);
    if (isType (token, TOKEN_OPERATOR))
    {
	readToken (token);
	if (isType (token, TOKEN_IDENTIFIER))
	    makeFortranTag (token, type);
    }
}

/*  specification-stmt
 *      is access-stmt      (is access-spec [[::] access-id-list)
 *      or allocatable-stmt (is ALLOCATABLE [::] array-name etc.)
 *      or common-stmt      (is COMMON [ / [common-block-name] /] etc.)
 *      or data-stmt        (is DATA data-stmt-list [[,] data-stmt-set] ...)
 *      or dimension-stmt   (is DIMENSION [::] array-name etc.)
 *      or equivalence-stmt (is EQUIVALENCE equivalence-set-list)
 *      or external-stmt    (is EXTERNAL etc.)
 *      or intent-stmt      (is INTENT ( intent-spec ) [::] etc.)
 *      or instrinsic-stmt  (is INTRINSIC etc.)
 *      or namelist-stmt    (is NAMELIST / namelist-group-name / etc.)
 *      or optional-stmt    (is OPTIONAL [::] etc.)
 *      or pointer-stmt     (is POINTER [::] object-name etc.)
 *      or save-stmt        (is SAVE etc.)
 *      or target-stmt      (is TARGET [::] object-name etc.)
 *
 *  access-spec is PUBLIC or PRIVATE
 */
static boolean parseSpecificationStmt (tokenInfo *const token)
{
    boolean result = TRUE;
    switch (token->keyword)
    {
	case KEYWORD_common: parseCommonStmt (token); break;

	case KEYWORD_namelist:
	    tagSlashName (token, TAG_NAMELIST);
	    skipToNextStatement (token);
	    break;

	case KEYWORD_allocatable:
	case KEYWORD_data:
	case KEYWORD_dimension:
	case KEYWORD_equivalence:
	case KEYWORD_external:
	case KEYWORD_intent:
	case KEYWORD_intrinsic:
	case KEYWORD_optional:
	case KEYWORD_pointer:
	case KEYWORD_private:
	case KEYWORD_public:
	case KEYWORD_save:
	case KEYWORD_target:
	    skipToNextStatement (token);
	    break;

	default:
	    result = FALSE;
	    break;
    }
    return result;
}

/*  component-def-stmt is
 *      type-spec [[, component-attr-spec-list] ::] component-decl-list
 *
 *  component-decl is
 *      component-name [ ( component-array-spec ) ] [ * char-length ]
 */
static void parseComponentDefStmt (tokenInfo *const token)
{
    Assert (isTypeSpec (token));
    parseTypeSpec (token);
    if (isType (token, TOKEN_COMMA))
	parseQualifierSpecList (token);
    if (isType (token, TOKEN_DOUBLE_COLON))
	readToken (token);
    do
    {
	if (isType (token, TOKEN_IDENTIFIER))
	    makeFortranTag (token, TAG_COMPONENT);
	skipPast (token, TOKEN_COMMA);
    } while (! isType (token, TOKEN_STATEMENT_END));
    readToken (token);
}

/*  derived-type-def is
 *      derived-type-stmt is (TYPE [[, access-spec] ::] type-name
 *          [private-sequence-stmt] ... (is PRIVATE or SEQUENCE)
 *          component-def-stmt
 *          [component-def-stmt] ...
 *          end-type-stmt
 */
static void parseDerivedTypeDef (tokenInfo *const token)
{
    if (isType (token, TOKEN_COMMA))
	parseQualifierSpecList (token);
    if (isType (token, TOKEN_DOUBLE_COLON))
	readToken (token);
    if (isType (token, TOKEN_IDENTIFIER))
	makeFortranTag (token, TAG_DERIVED_TYPE);
    ancestorPush (token);
    skipToNextStatement (token);
    if (isKeyword (token, KEYWORD_private) ||
	isKeyword (token, KEYWORD_sequence))
    {
	skipToNextStatement (token);
    }
    while (! isMatchingEnd (token, KEYWORD_type))
    {
	if (isTypeSpec (token))
	    parseComponentDefStmt (token);
	else
	    skipToNextStatement (token);
    }
    ancestorPop ();
}

/*  interface-block
 *      interface-stmt (is INTERFACE [generic-spec])
 *          [interface-body]
 *          [module-procedure-stmt] ...
 *          end-interface-stmt (is END INTERFACE)
 *
 *  generic-spec
 *      is generic-name
 *      or OPERATOR ( defined-operator )
 *      or ASSIGNMENT ( = )
 *
 *  interface-body
 *      is function-stmt
 *          [specification-part]
 *          end-function-stmt
 *      or subroutine-stmt
 *          [specification-part]
 *          end-subroutine-stmt
 *
 *  module-procedure-stmt is
 *      MODULE PROCEDURE procedure-name-list
 */
static void parseInterfaceBlock (tokenInfo *const token)
{
    readToken (token);
    if (isType (token, TOKEN_IDENTIFIER))
	makeFortranTag (token, TAG_INTERFACE);
    else if (isKeyword (token, KEYWORD_assignment) ||
	     isKeyword (token, KEYWORD_operator))
    {
	parseParenName (token);

	if (isType (token, TOKEN_OPERATOR))
	    makeFortranTag (token, TAG_INTERFACE);
    }
    while (! isMatchingEnd (token, KEYWORD_interface))
	readToken (token);
    skipToNextStatement (token);
}

/*  entry-stmt is
 *      ENTRY entry-name [ ( dummy-arg-list ) ]
 */
static void parseEntryStmt (tokenInfo *const token)
{
    Assert (isKeyword (token, KEYWORD_entry));
    readToken (token);
    if (isType (token, TOKEN_IDENTIFIER))
	makeFortranTag (token, TAG_ENTRY_POINT);
    skipToNextStatement (token);
}

 /*  stmt-function-stmt is
  *      function-name ([dummy-arg-name-list]) = scalar-expr
  */
static boolean parseStmtFunctionStmt (tokenInfo *const token)
{
    boolean result = FALSE;
    Assert (isType (token, TOKEN_IDENTIFIER));
#if 0	    /* cannot reliably parse this yet */
    makeFortranTag (token, TAG_FUNCTION);
#endif
    readToken (token);
    if (isType (token, TOKEN_PAREN_OPEN))
    {
	skipOverParens (token);
	result = (boolean) (isType (token, TOKEN_OPERATOR) &&
	    strcmp (vStringValue (token->string), "=") == 0);
    }
    skipToNextStatement (token);
    return result;
}

/*  declaration-construct
 *      [derived-type-def]
 *      [interface-block]
 *      [type-declaration-stmt]
 *      [specification-stmt]
 *      [parameter-stmt] (is PARAMETER ( named-constant-def-list )
 *      [format-stmt]    (is FORMAT format-specification)
 *      [entry-stmt]
 *      [stmt-function-stmt]
 */
static boolean parseDeclarationConstruct (tokenInfo *const token)
{
    boolean result = TRUE;
    switch (token->keyword)
    {
	case KEYWORD_entry:	parseEntryStmt (token);      break;
	case KEYWORD_interface:	parseInterfaceBlock (token); break;
	case KEYWORD_format:    skipToNextStatement (token); break;
	case KEYWORD_parameter: skipToNextStatement (token); break;
	case KEYWORD_include:   skipToNextStatement (token); break;
	/* derived type handled by parseTypeDeclarationStmt(); */

	default:
	    if (isTypeSpec (token))
	    {
		parseTypeDeclarationStmt (token);
		result = TRUE;
	    }
	    else if (isType (token, TOKEN_IDENTIFIER))
		result = parseStmtFunctionStmt (token);
	    else
		result = parseSpecificationStmt (token);
	    break;
    }
    return result;
}

/*  implicit-part-stmt
 *      is [implicit-stmt] (is IMPLICIT etc.)
 *      or [parameter-stmt] (is PARAMETER etc.)
 *      or [format-stmt] (is FORMAT etc.)
 *      or [entry-stmt] (is ENTRY entry-name etc.)
 */
static boolean parseImplicitPartStmt (tokenInfo *const token)
{
    boolean result = TRUE;
    switch (token->keyword)
    {
	case KEYWORD_entry: parseEntryStmt (token); break;

	case KEYWORD_implicit:
	case KEYWORD_include:
	case KEYWORD_parameter:
	case KEYWORD_format:
	    skipToNextStatement (token);
	    break;

	default: result = FALSE; break;
    }
    return result;
}

/*  specification-part is
 *      [use-stmt] ... (is USE module-name etc.)
 *      [implicit-part] (is [implicit-part-stmt] ... [implicit-stmt])
 *      [declaration-construct] ...
 */
static void parseSpecificationPart (tokenInfo *const token)
{
    while (skipStatementIfKeyword (token, KEYWORD_use))
	;
    while (parseImplicitPartStmt (token))
	;
    while (parseDeclarationConstruct (token))
	;
}

/*  block-data is
 *      block-data-stmt (is BLOCK DATA [block-data-name]
 *          [specification-part]
 *          end-block-data-stmt (is END [BLOCK DATA [block-data-name]])
 */
static void parseBlockData (tokenInfo *const token)
{
    Assert (isKeyword (token, KEYWORD_block));
    readToken (token);
    if (isKeyword (token, KEYWORD_data))
    {
	readToken (token);
	makeFortranTag (token, TAG_BLOCK_DATA);
    }
    ancestorPush (token);
    skipToNextStatement (token);
    parseSpecificationPart (token);
    while (! isMatchingEnd (token, KEYWORD_block))
	readToken (token);
    skipToNextStatement (token);
    ancestorPop ();
}

/*  internal-subprogram-part is
 *      contains-stmt (is CONTAINS)
 *          internal-subprogram
 *          [internal-subprogram] ...
 *
 *  internal-subprogram
 *      is function-subprogram
 *      or subroutine-subprogram
 */
static void parseInternalSubprogramPart (tokenInfo *const token)
{
    boolean done = FALSE;
    Assert (isKeyword (token, KEYWORD_contains));
    skipToNextStatement (token);
    do
    {
	switch (token->keyword)
	{
	    case KEYWORD_function:   parseFunctionSubprogram (token);   break;
	    case KEYWORD_subroutine: parseSubroutineSubprogram (token); break;
	    case KEYWORD_recursive:  readToken (token); break;

	    default:
		if (isTypeSpec (token))
		    parseTypeSpec (token);
		else
		    done = TRUE;
		break;
	}
    } while (! done);
}

/*  module is
 *      mudule-stmt (is MODULE module-name)
 *          [specification-part]
 *          [module-subprogram-part]
 *          end-module-stmt (is END [MODULE [module-name]])
 *
 *  module-subprogram-part
 *      contains-stmt (is CONTAINS)
 *          module-subprogram
 *          [module-subprogram] ...
 *
 *  module-subprogram
 *      is function-subprogram
 *      or subroutine-subprogram
 */
static void parseModule (tokenInfo *const token)
{
    Assert (isKeyword (token, KEYWORD_module));
    readToken (token);
    if (isType (token, TOKEN_IDENTIFIER))
	makeFortranTag (token, TAG_MODULE);
    ancestorPush (token);
    skipToNextStatement (token);
    parseSpecificationPart (token);
    if (isKeyword (token, KEYWORD_contains))
	parseInternalSubprogramPart (token);
    while (! isMatchingEnd (token, KEYWORD_module))
	readToken (token);
    skipToNextStatement (token);
    ancestorPop ();
}

/*  execution-part
 *      executable-contstruct
 *
 *  executable-contstruct is
 *      execution-part-construct [execution-part-construct]
 *
 *  execution-part-construct
 *      is executable-construct
 *      or format-stmt
 *      or data-stmt
 *      or entry-stmt
 */
static void parseExecutionPart (tokenInfo *const token, const keywordId keyword)
{
    while (! isMatchingEnd (token, keyword))
    {
	readToken (token);
	if (isKeyword (token, KEYWORD_contains))
	    parseInternalSubprogramPart (token);
	else if (isKeyword (token, KEYWORD_entry))
	    parseEntryStmt (token);
	skipOverParens (token);
    }
    skipToNextStatement (token);
}

static void parseSubprogram (tokenInfo *const token,
			     const keywordId keyword, const tagType tag)
{
    Assert (isKeyword (token, keyword));
    readToken (token);
    if (isType (token, TOKEN_IDENTIFIER))
	makeFortranTag (token, tag);
    ancestorPush (token);
    skipToNextStatement (token);
    parseSpecificationPart (token);
    parseExecutionPart (token, keyword);
    ancestorPop ();
}


/*  function-subprogram is
 *      function-stmt (is [prefix] FUNCTION function-name etc.)
 *          [specification-part]
 *          [execution-part]
 *          [internal-subprogram-part]
 *          end-function-stmt (is END [FUNCTION [function-name]])
 *
 *  prefix
 *      is type-spec [RECURSIVE]
 *      or [RECURSIVE] type-spec
 */
static void parseFunctionSubprogram (tokenInfo *const token)
{
    parseSubprogram (token, KEYWORD_function, TAG_FUNCTION);
}

/*  subroutine-subprogram is
 *      subroutine-stmt (is [RECURSIVE] SUBROUTINE subroutine-name etc.)
 *          [specification-part]
 *          [execution-part]
 *          [internal-subprogram-part]
 *          end-subroutine-stmt (is END [SUBROUTINE [function-name]])
 */
static void parseSubroutineSubprogram (tokenInfo *const token)
{
    parseSubprogram (token, KEYWORD_subroutine, TAG_SUBROUTINE);
}

/*  main-program is
 *      [program-stmt] (is PROGRAM program-name)
 *          [specification-part]
 *          [execution-part]
 *          [internal-subprogram-part ]
 *          end-program-stmt
 */
static void parseMainProgram (tokenInfo *const token)
{
    parseSubprogram (token, KEYWORD_program, TAG_PROGRAM);
}

/*  program-unit
 *      is main-program
 *      or external-subprogram (is function-subprogram or subroutine-subprogram)
 *      or module
 *      or block-data
 */
static void parseProgramUnit (tokenInfo *const token)
{
    readToken (token);
    do
    {
	if (isType (token, TOKEN_STATEMENT_END))
	    readToken (token);
	else switch (token->keyword)
	{
	    case KEYWORD_block:      parseBlockData (token);            break;
	    case KEYWORD_function:   parseFunctionSubprogram (token);   break;
	    case KEYWORD_module:     parseModule (token);               break;
	    case KEYWORD_program:    parseMainProgram (token);          break;
	    case KEYWORD_subroutine: parseSubroutineSubprogram (token); break;
	    case KEYWORD_recursive:  readToken (token);                 break;
	    default:
		if (isTypeSpec (token))
		    parseTypeSpec (token);
		else
		{
		    parseSpecificationPart (token);
		    parseExecutionPart (token, KEYWORD_NONE);
		}
		break;
	}
    } while (TRUE);
}

static boolean findFortranTags (const unsigned int passCount)
{
    tokenInfo *token;
    exception_t exception;
    boolean retry;

    Assert (passCount < 3);
    Parent = newToken ();
    token = newToken ();
    FreeSourceForm = (boolean) (passCount > 1);
    Column = 0;
    exception = (exception_t) setjmp (Exception);
    if (exception == ExceptionEOF)
	retry = FALSE;
    else if (exception == ExceptionFixedFormat  &&  ! FreeSourceForm)
    {
	verbose ("%s: not fixed source form; retry as free source form\n",
		getInputFileName ());
	retry = TRUE;
    }
    else
    {
	parseProgramUnit (token);
	retry = FALSE;
    }
    ancestorClear ();
    deleteToken (token);
    deleteToken (Parent);

    return retry;
}

static void initialize (const langType language)
{
    Lang_fortran = language;
    buildFortranKeywordHash ();
}

extern parserDefinition* FortranParser (void)
{
    static const char *const extensions [] = {
	"f", "for", "ftn", "f77", "f90", "f95",
#ifndef CASE_INSENSITIVE_FILENAMES
	"F", "FOR", "FTN", "F77", "F90", "F95",
#endif
	NULL
    };
    parserDefinition* def = parserNew ("Fortran");
    def->kinds      = FortranKinds;
    def->kindCount  = KIND_COUNT (FortranKinds);
    def->extensions = extensions;
    def->parser2    = findFortranTags;
    def->initialize = initialize;
    return def;
}

/* vi:set tabstop=8 shiftwidth=4: */
