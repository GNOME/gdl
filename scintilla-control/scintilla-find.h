#ifndef __SCINTILLA_FIND_H__
#define __SCINTILLA_FIND_H__

#include "scintilla/Scintilla.h"
#include "scintilla/ScintillaWidget.h"

typedef struct _ScintillaFindDialog ScintillaFindDialog;
typedef struct _ScintillaReplaceDialog ScintillaReplaceDialog;

void run_find_dialog (ScintillaObject *sci);
void find_again (ScintillaObject *sci);
void scintilla_find_dialog_destroy (ScintillaFindDialog *dialog);

void run_replace_dialog (ScintillaObject *sci);
void scintilla_replace_dialog_destroy (ScintillaReplaceDialog *dialog);

#endif
