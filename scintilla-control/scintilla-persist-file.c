/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Devtool Libraries.
 * 
 * Copyright (C) 2000 Dave Camp <dave@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#include <config.h>
#include <gnome.h>
#include <bonobo.h>
#include <gtk/gtk.h>

#include "scintilla/ScintillaWidget.h"
#include "scintilla/Scintilla.h"
#include "scintilla/SciLexer.h"
#include "scintilla-persist-file.h"

#define BLOCK_SIZE 4096

static int
impl_save (BonoboPersistFile *pf,
	   const CORBA_char *filename,
	   CORBA_Environment *ev,
	   void *closure)
{
    ScintillaObject *sci = closure;
    FILE *fp;
    
    fp = fopen (filename, "w");
    
    if (fp) {
	char data[BLOCK_SIZE + 1];
	long length_doc = scintilla_send_message (sci, SCI_GETLENGTH, 0, 0);
	int i;

	for (i = 0; i < length_doc; i += BLOCK_SIZE) {
	    TextRange tr;
	    int grab_size = length_doc - i;
	    if (grab_size > BLOCK_SIZE) 
		grab_size = BLOCK_SIZE;
	    
	    tr.chrg.cpMin = i;
	    tr.chrg.cpMax = i + grab_size;
	    tr.lpstrText = data;
	    
	    scintilla_send_message (sci, SCI_GETTEXTRANGE, 0, (long)&tr);

	    fwrite (data, grab_size, 1, fp);
	}
	fclose (fp);
    }
    return 0;
}

struct ExtensionLexer {
    char *ext;
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
    { ".idl", SCLEX_CPP, idl_keywords },
    { ".c", SCLEX_CPP, c_keywords },
    { ".cpp", SCLEX_CPP, cpp_keywords },
    { ".cxx", SCLEX_CPP, cpp_keywords },
    { ".C", SCLEX_CPP, cpp_keywords },
    { ".h", SCLEX_CPP, c_keywords },
    { ".hpp", SCLEX_CPP, cpp_keywords },
    { ".hxx", SCLEX_CPP, cpp_keywords },
    { ".H", SCLEX_CPP, cpp_keywords },
    { ".html", SCLEX_HTML, NULL },
    { ".xml", SCLEX_XML, NULL },
    { ".pl", SCLEX_PERL, NULL },
    { ".sql", SCLEX_SQL, NULL },
    { ".diff", SCLEX_DIFF, NULL },
    { ".patch", SCLEX_DIFF, NULL },
    { ".py", SCLEX_PYTHON, NULL },
    { ".am", SCLEX_MAKEFILE, NULL },
    { NULL, 0 }
};

static void 
set_language_properties (ScintillaObject *sci, const char *filename)
{
    struct ExtensionLexer *lexer;
    long lex = SCLEX_NULL;
    char *keywords = NULL;
    char *ext = strrchr (filename, '.');
    char *font = "courier";
    if (ext) {
	for (lexer = lexer_map; lexer->ext; lexer++) {
	    if (!strcmp (lexer->ext, ext)) {
		lex = lexer->lexer;
		keywords = lexer->keywords;
		break;
	    }
	}
    }
    if (lex == SCLEX_NULL) {
	ext = strrchr (filename, '/');
	ext = ext ? ext++ : (char*)filename;
	if (ext[0] == 'M' || ext[0] == 'm') {
	    ext++;
	    if (!strcmp (ext, "akefile")) {
		lex = SCLEX_MAKEFILE;
	    }
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

static int
impl_load (BonoboPersistFile *pf,
	   const CORBA_char *filename,
	   CORBA_Environment *ev,
	   void *closure)
{
    ScintillaObject *sci = closure;
    FILE *fp;

    g_return_val_if_fail (IS_SCINTILLA (sci), -1);

    set_language_properties (sci, filename);

    scintilla_send_message (sci, SCI_CLEARALL, 0, 0);
    scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
    scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);

    scintilla_send_message (sci, SCI_CANCEL, 0, 0);
    scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 0, 0);
    
    fp = fopen (filename, "r");
    if (fp) {
	if (fp) {
	    char data[1024];
	    int nread = fread (data, 1, sizeof (data), fp);
	    while (nread > 0) {
		scintilla_send_message (sci, SCI_ADDTEXT, nread, (long)data);
		nread = fread (data, 1, sizeof (data), fp);
		fwrite (data, 1, nread, fp);
	    }
	    fclose (fp);
	}
	scintilla_send_message (sci, SCI_SETUNDOCOLLECTION, 1, 0);
        
	scintilla_send_message (sci, SCI_EMPTYUNDOBUFFER, 0, 0);
	scintilla_send_message (sci, SCI_SETSAVEPOINT, 0, 0);
	scintilla_send_message (sci, SCI_GOTOPOS, 0, 0);

	return 0;
    } else {
	fprintf (stderr, "error\n");
	return -1;
    }
    return 0;
}

BonoboPersistFile *
scintilla_persist_file_new (GtkWidget *sci)
{
    g_return_val_if_fail (IS_SCINTILLA (sci), NULL);
    return bonobo_persist_file_new (impl_load, impl_save, sci);
}
