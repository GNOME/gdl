#include <config.h>
#include <gnome.h>
#include <bonobo.h>
#include <gtk/gtk.h>

#include "scintilla/ScintillaWidget.h"
#include "scintilla/Scintilla.h"
#include "scintilla/SciLexer.h"

struct ExtensionLexer {
    char *mime_type;
    int lexer;
    char *keywords;
};

/* FIXME: All this lexer stuff should be moved out into configuration files
 * of some sort.  But I'm too lazy at the moment, so it is all hard-coded 
 * in. */

#define idl_keywords "switch union raises interface module in out void long boolean short string struct enum"
#define c_keywords "asm auto break case catch char class const const_cast continue default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable register return short signed sizeof static struct switch typedef union unsigned void volatile wchar_t while"
#define cpp_keywords "asm auto bool break case catch char class const const_cast continue default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new operator private protected public register reinterpret_cast return short signed sizeof static static_cast struct switch template this throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while"

struct ExtensionLexer lexer_map[] = {
    { "text/x-idl", SCLEX_CPP, idl_keywords },
    { "text/x-c", SCLEX_CPP, c_keywords },
    { "text/x-c++", SCLEX_CPP, cpp_keywords },
    { "text/html", SCLEX_HTML, NULL },
    { "text/xml", SCLEX_XML, NULL },
    { "text/x-perl", SCLEX_PERL, NULL },
    { "text/x-sql", SCLEX_SQL, NULL },
    { "text/x-python", SCLEX_PYTHON, NULL },
    { "text/x-makefile", SCLEX_MAKEFILE, NULL },
    { NULL, 0 }
};

void 
set_language_properties (ScintillaObject *sci, const char *mime_type)
{
    struct ExtensionLexer *lexer;
    long lex = SCLEX_NULL;
    char *keywords = NULL;
    char *font = "courier";

    for (lexer = lexer_map; lexer->mime_type; lexer++) {
      if (!strcmp (lexer->mime_type, mime_type)) {
	lex = lexer->lexer;
	keywords = lexer->keywords;
	break;
      }
    }

    if ((lex == SCLEX_HTML)||(lex == SCLEX_XML)) 
	scintilla_send_message (sci, SCI_SETSTYLEBITS, 7, 0);
    else
	scintilla_send_message (sci, SCI_SETSTYLEBITS, 5, 0);
    scintilla_send_message (sci, SCI_SETLEXER, lex, 0);

    scintilla_send_message (sci, SCI_STYLERESETDEFAULT, 0, 0);
    scintilla_send_message (sci, SCI_STYLECLEARALL, 0, 0);

    if (keywords) {
	scintilla_send_message (sci, SCI_SETKEYWORDS, 0, (long)keywords);
	
	/* Whitespace */ 
	scintilla_send_message (sci, SCI_STYLESETFONT,0,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,0,0x808080);
	
	/* Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,1,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,1,0x0000FF);
	scintilla_send_message (sci, SCI_STYLESETITALIC,1,1);
	
	/* Line Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,2,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,2,0x0000AA);
	scintilla_send_message (sci, SCI_STYLESETITALIC,2,1);
	
	/* Doc Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,3,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,3,0x0000AA);
	scintilla_send_message (sci, SCI_STYLESETITALIC,3,1);
	
	/* Number */
	scintilla_send_message (sci, SCI_STYLESETFONT,4,(long)font);

	/* Keyword */
	scintilla_send_message (sci, SCI_STYLESETFONT,5,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,5,0xFF0000);
	scintilla_send_message (sci, SCI_STYLESETBOLD,5,1);
        
	/* Double quoted string */
	scintilla_send_message (sci, SCI_STYLESETFONT,6,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,6,0x00CC00);
                
	/* Single quoted string */
	scintilla_send_message (sci, SCI_STYLESETFONT,7,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,7,0x00FF00);
                
	/* UUID */
	/* Preprocessor */
	scintilla_send_message (sci, SCI_STYLESETFONT,9,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,9,0x7F7F00);
                
	/* Operators */
	scintilla_send_message (sci, SCI_STYLESETFONT,10,(long)font);
                
	/* Identifiers */
	scintilla_send_message (sci, SCI_STYLESETFONT,11,(long)font);

    }
    scintilla_send_message (sci, SCI_COLOURISE, 0, -1);


}
