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

#define idl_keywords "allocate appobject async auto_handle bindable boolean broadcast byte byte_count call_as callback char coclass code comm_status const context_handle control cpp_quote decode default defaultbind defaultvalue dispinterface displaybind dllname double dual enable_allocate encode endpoint entry enum error_status_t explicit_handle fault_status first_is float handle_t heap helpcontext helpfile helpstring hidden hyper id idempotent ignore iid_is immediatebind implicit_handle import importlib in include in_line int __int64 interface last_is lcid length_is library licensed local long max_is maybe message midl_user_allocate midl_user_free min_is module ms_union ncacn_at_dsp ncacn_dnet_nsp ncacn_http ncacn_ip_tcp ncacn_nb_ipx ncacn_nb_nb ncacn_nb_tcp ncacn_np ncacn_spx ncacn_vns_spp ncadg_ip_udp ncadg_ipx ncadg_mq ncalrpc nocode nonextensible notify object odl oleautomation optimize optional out out_of_line pipe pointer_default propget propput propputref public ptr readonly ref represent_as requestedit restricted retval shape short signed size_is small source string struct switch switch_is switch_type transmit_as typedef"

#define c_keywords "asm auto break case catch char class const const_cast continue default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable register return short signed sizeof static struct switch typedef union unsigned void volatile wchar_t while"

#define cpp_keywords "asm auto bool break case catch char class const const_cast continue default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new operator private protected public register reinterpret_cast return short signed sizeof static static_cast struct switch template this throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while"

#define html_keywords "abbr accept-charset accept accesskey action align alink alt archive axis background bgcolor border cellpadding cellspacing char charoff charset checked cite class classid clear codebase codetype color cols colspan compact content coords data datafld dataformatas datapagesize datasrc datetime declare defer dir disabled enctype event face for frame frameborder headers height href hreflang hspace http-equiv id ismap label lang language leftmargin link longdesc marginwidth marginheight maxlength media method multiple name nohref noresize noshade nowrap object onblur onchange onclick ondblclick onfocus onkeydown onkeypress onkeyup onload onmousedown onmousemove onmouseover onmouseout onmouseup onreset onselect onsubmit onunload profile prompt readonly rel rev rows rowspan rules scheme scope selected shape size span src standby start style summary tabindex target text title topmargin type usemap valign value valuetype version vlink vspace width text password checkbox radio submit reset file hidden image"

#define python_keywords "and assert break class continue def del elif else except exec finally for from global if import in is lambda None not or pass print raise return try while"

struct ExtensionLexer lexer_map[] = {
    { "text/x-idl", SCLEX_CPP, idl_keywords },
    { "text/x-c", SCLEX_CPP, c_keywords },
    { "text/x-c++", SCLEX_CPP, cpp_keywords },
    { "text/html", SCLEX_HTML, html_keywords },
    { "text/xml", SCLEX_XML, NULL },
    { "text/x-perl", SCLEX_PERL, NULL },
    { "text/x-sql", SCLEX_SQL, NULL },
    { "text/x-python", SCLEX_PYTHON, python_keywords },
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

	if (keywords)
		scintilla_send_message (sci, SCI_SETKEYWORDS, 0, (long)keywords);
	
	/* Whitespace */ 
	scintilla_send_message (sci, SCI_STYLESETFONT,0,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,0,0x808080);
	
	/* Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,1,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,1,0x007F00);
	scintilla_send_message (sci, SCI_STYLESETITALIC,1,1);
	
	/* Line Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,2,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,2,0x007F00);
	scintilla_send_message (sci, SCI_STYLESETITALIC,2,1);
	
	/* Doc Comment */
	scintilla_send_message (sci, SCI_STYLESETFONT,3,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,3,0x3F703F);
	scintilla_send_message (sci, SCI_STYLESETITALIC,3,1);
	
	/* Number */
	scintilla_send_message (sci, SCI_STYLESETFONT,4,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,4,0x007F7F);

	/* Keyword */
	scintilla_send_message (sci, SCI_STYLESETFONT,5,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,5,0x00007F);
	scintilla_send_message (sci, SCI_STYLESETBOLD,5,1);
        
	/* Double quoted string */
	scintilla_send_message (sci, SCI_STYLESETFONT,6,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,6,0x7F007F);
                
	/* Single quoted string */
	scintilla_send_message (sci, SCI_STYLESETFONT,7,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,7,0x7F007F);
                
	/* UUID */
	/* Preprocessor */
	scintilla_send_message (sci, SCI_STYLESETFONT,9,(long)font);
	scintilla_send_message (sci, SCI_STYLESETFORE,9,0x747F00);
                
	/* Operators */
	scintilla_send_message (sci, SCI_STYLESETFONT,10,(long)font);
                
	/* Identifiers */
	scintilla_send_message (sci, SCI_STYLESETFONT,11,(long)font);

	/* Indentation guides */
	scintilla_send_message (sci, SCI_STYLESETFORE,37,0xC0C0C0);

	scintilla_send_message (sci, SCI_COLOURISE, 0, -1);
}
