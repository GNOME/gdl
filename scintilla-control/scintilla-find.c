/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 */

#include <gnome.h>
#include "scintilla-find.h"


/* ----------------------------------------------------------------------
 * Data Structures 
 * ---------------------------------------------------------------------- */

struct _ScintillaFindDialog {
    GnomeDialog *dialog;
    GtkWidget *entry;

    GtkWidget *case_sensitive, *whole_word, *word_start;
    GtkWidget *forward, *backward;

    ScintillaObject *sci;
};


struct _ScintillaReplaceDialog {
    GnomeDialog *dialog;
    GnomeDialog *ask_dialog;
    GtkWidget *entry_find, *entry_replace;

    GtkWidget *case_sensitive, *whole_word, *word_start;
    GtkWidget *selection_only;

    struct CharacterRange search_range;
    ScintillaObject *sci;
};


/* ----------------------------------------------------------------------
 * Search functions 
 * ---------------------------------------------------------------------- */

static void
do_search (GtkWidget *widget, ScintillaFindDialog *d)
{
    struct TextToFind ttf;
    long pos, flags = 0;

    ttf.lpstrText = gtk_entry_get_text (GTK_ENTRY (d->entry));
    if (strlen (ttf.lpstrText) == 0)
        return;

    /* search options */
    if (GTK_TOGGLE_BUTTON (d->case_sensitive)->active)
        flags |= SCFIND_MATCHCASE;
    if (GTK_TOGGLE_BUTTON (d->whole_word)->active)
        flags |= SCFIND_WHOLEWORD;
    if (GTK_TOGGLE_BUTTON (d->word_start)->active)
        flags |= SCFIND_WORDSTART;

    /* calculate search range */
    if (GTK_TOGGLE_BUTTON (d->backward)->active) {
        ttf.chrg.cpMin = MAX (0, scintilla_send_message 
                              (d->sci, SCI_GETCURRENTPOS, 0, 0) - 1);
        ttf.chrg.cpMax = 0;
    } else {
        ttf.chrg.cpMin = scintilla_send_message (d->sci, SCI_GETCURRENTPOS, 0, 0);
        ttf.chrg.cpMax = scintilla_send_message (d->sci, SCI_GETLENGTH, 0, 0);
    };

    pos = scintilla_send_message (d->sci, SCI_FINDTEXT, flags, (long) &ttf);
    if (pos >= 0) {
        /* mark search result */
        if (GTK_TOGGLE_BUTTON (d->backward)->active)
            scintilla_send_message (d->sci, SCI_SETSEL, 
                                    ttf.chrgText.cpMax, 
                                    ttf.chrgText.cpMin);
        else
            scintilla_send_message (d->sci, SCI_SETSEL, 
                                    ttf.chrgText.cpMin, 
                                    ttf.chrgText.cpMax);
    };
}


/* called when the user presses ENTER in the find textbox */
static void
entry_activate (GtkWidget *entry, ScintillaFindDialog *d)
{
    gnome_dialog_close (d->dialog);
    do_search (NULL, d);
}


static ScintillaFindDialog *
scintilla_find_dialog_new (ScintillaObject *sci)
{
    GtkWidget *hbox, *label, *frame;
    GSList *radio_group = NULL;
    ScintillaFindDialog *dialog;
    
    dialog = g_new0 (ScintillaFindDialog, 1);

    dialog->sci = sci;

    /* create the dialog */
    dialog->dialog = GNOME_DIALOG 
        (gnome_dialog_new (_("Find"), 
                           _("Find"), 
                           GNOME_STOCK_BUTTON_CANCEL, 
                           NULL));

    /* find textbox hbox */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Find:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    /* create find entry textbox */
    dialog->entry = gtk_entry_new_with_max_length (20);
    gtk_signal_connect (GTK_OBJECT (dialog->entry), "activate",
                        entry_activate, dialog);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->entry, TRUE, TRUE, 0);

    /* search options hbox */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), hbox, TRUE, TRUE, 0);

    dialog->case_sensitive = gtk_check_button_new_with_label (_("Case sensitive"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->case_sensitive, FALSE, FALSE, 0);

    dialog->whole_word = gtk_check_button_new_with_label (_("Whole word"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->whole_word, FALSE, FALSE, 0);

    dialog->word_start = gtk_check_button_new_with_label (_("Match word start"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->word_start, FALSE, FALSE, 0);

    /* search direction radio group */
    frame = gtk_frame_new (_("Direction"));
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

    dialog->forward = gtk_radio_button_new_with_label (radio_group, _("Forward"));
    radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (dialog->forward));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->forward, FALSE, TRUE, 0);

    dialog->backward = gtk_radio_button_new_with_label (radio_group, _("Backward"));
    radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (dialog->backward));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->backward, FALSE, TRUE, 0);

    gtk_widget_show_all (dialog->dialog->vbox);

    /* set dialog options */
    gnome_dialog_button_connect (dialog->dialog, 0, do_search, dialog);
    gnome_dialog_close_hides (dialog->dialog, TRUE);
    gnome_dialog_set_close (dialog->dialog, TRUE);
    gnome_dialog_set_default (dialog->dialog, 0);

    return dialog;
}


void
scintilla_find_dialog_destroy (ScintillaFindDialog *dialog)
{
    gtk_widget_destroy (GTK_WIDGET (dialog->dialog));
    g_free (dialog);
}


void
run_find_dialog (ScintillaObject *sci)
{
    ScintillaFindDialog *dialog;

    dialog = gtk_object_get_data (GTK_OBJECT (sci), "find_dialog");
    if (!dialog) {
        dialog = scintilla_find_dialog_new (sci);
        gtk_object_set_data (GTK_OBJECT (sci), "find_dialog", dialog);
    };
    
    if (dialog) {
        gtk_widget_show (GTK_WIDGET (dialog->dialog));
        gdk_window_raise (GTK_WIDGET (dialog->dialog)->window);
        gtk_widget_grab_focus (dialog->entry);
    };
}


void 
find_again (ScintillaObject *sci)
{
    ScintillaFindDialog *dialog;

    dialog = gtk_object_get_data (GTK_OBJECT (sci), "find_dialog");
    if (dialog)
        do_search (NULL, dialog);
}


/* ----------------------------------------------------------------------
 * Replace functions 
 * ---------------------------------------------------------------------- */

static long
search_next (ScintillaReplaceDialog *d)
{
    struct TextToFind ttf;
    long pos, flags = 0;

    /* search options */
    if (GTK_TOGGLE_BUTTON (d->case_sensitive)->active)
        flags |= SCFIND_MATCHCASE;
    if (GTK_TOGGLE_BUTTON (d->whole_word)->active)
        flags |= SCFIND_WHOLEWORD;
    if (GTK_TOGGLE_BUTTON (d->word_start)->active)
        flags |= SCFIND_WORDSTART;

    ttf.chrg.cpMin = d->search_range.cpMin;
    ttf.chrg.cpMax = d->search_range.cpMax;
    ttf.lpstrText = gtk_entry_get_text (GTK_ENTRY (d->entry_find));

    pos = scintilla_send_message (d->sci, SCI_FINDTEXT, flags, (long) &ttf);
    if (pos >= 0) {
        /* mark search result and move next range */
        scintilla_send_message (d->sci, SCI_SETSEL, 
                                ttf.chrgText.cpMin, 
                                ttf.chrgText.cpMax);
        d->search_range.cpMin = ttf.chrgText.cpMax;
    };
    return pos;
}

static void
skip_cb (GtkWidget *widget, ScintillaReplaceDialog *d)
{
    if (search_next (d) < 0)
        gnome_dialog_close (d->ask_dialog);
}

static void
replace_cb (GtkWidget *widget, ScintillaReplaceDialog *d)
{
    scintilla_send_message (d->sci, SCI_REPLACESEL, 0, 
                            (long) gtk_entry_get_text
                            (GTK_ENTRY (d->entry_replace)));
    skip_cb (widget, d);
}

static void
replace_all_cb (GtkWidget *widget, ScintillaReplaceDialog *d)
{
    do {
        scintilla_send_message (d->sci, SCI_REPLACESEL, 0, 
                                (long) gtk_entry_get_text
                                (GTK_ENTRY (d->entry_replace)));
    } while (search_next (d) >= 0);
    gnome_dialog_close (d->ask_dialog);
}

static void
cancel_cb (GtkWidget *widget, ScintillaReplaceDialog *d)
{
    gnome_dialog_close (d->ask_dialog);
}

static GnomeDialog *
ask_dialog_new (ScintillaReplaceDialog *d)
{
    GnomeDialog *dialog;
    GtkWidget *label;

    dialog = GNOME_DIALOG (gnome_dialog_new (_("Replace confirmation"),
                                             _("Replace"), 
                                             _("Replace All"),
                                             _("Skip"),
                                             GNOME_STOCK_BUTTON_CANCEL, 
                                             NULL));
    label = gtk_label_new (_("Found occurrence in document"));
    gtk_box_pack_start (GTK_BOX (dialog->vbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    gnome_dialog_button_connect (dialog, 0, replace_cb, d);
    gnome_dialog_button_connect (dialog, 1, replace_all_cb, d);
    gnome_dialog_button_connect (dialog, 2, skip_cb, d);
    gnome_dialog_button_connect (dialog, 3, cancel_cb, d);
    gnome_dialog_close_hides (dialog, TRUE);

    gnome_dialog_set_default (dialog, 0);

    return dialog;
}


static void
setup_replace (GtkWidget *widget, ScintillaReplaceDialog *d)
{
    if (strlen (gtk_entry_get_text (GTK_ENTRY (d->entry_find))) == 0)
        return;

    if (GTK_TOGGLE_BUTTON (d->selection_only)->active) {
        d->search_range.cpMin = scintilla_send_message 
            (d->sci, SCI_GETSELECTIONSTART, 0, 0);
        d->search_range.cpMax = scintilla_send_message
            (d->sci, SCI_GETSELECTIONEND, 0, 0);
    } else {
        d->search_range.cpMin = scintilla_send_message (d->sci, SCI_GETCURRENTPOS, 
                                                        0, 0);
        d->search_range.cpMax = scintilla_send_message (d->sci, SCI_GETLENGTH, 
                                                        0, 0);
    };

    if (!d->ask_dialog)
        d->ask_dialog = ask_dialog_new (d);

    if (search_next (d) >= 0) {
        gtk_widget_show (GTK_WIDGET (d->ask_dialog));
        gdk_window_raise (GTK_WIDGET (d->ask_dialog)->window);
    };
}


static ScintillaReplaceDialog *
scintilla_replace_dialog_new (ScintillaObject *sci)
{
    GtkWidget *hbox, *label, *table;
    ScintillaReplaceDialog *dialog;
    
    dialog = g_new0 (ScintillaReplaceDialog, 1);

    dialog->sci = sci;

    /* create the dialog */
    dialog->dialog = GNOME_DIALOG 
        (gnome_dialog_new (_("Find and Replace"), 
                           _("Replace"), 
                           GNOME_STOCK_BUTTON_CANCEL, 
                           NULL));

    /* find and replace entries table */
    table = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), table, FALSE, FALSE, 0);

    label = gtk_label_new (_("Find:"));
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      (GtkAttachOptions) GTK_FILL, 
                      (GtkAttachOptions) 0, 2, 2);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

    dialog->entry_find = gtk_entry_new_with_max_length (20);
    gtk_table_attach (GTK_TABLE (table), dialog->entry_find, 1, 2, 0, 1,
                      (GtkAttachOptions) GTK_FILL | GTK_EXPAND, 
                      (GtkAttachOptions) 0, 2, 2);

    label = gtk_label_new (_("Replace with:"));
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      (GtkAttachOptions) GTK_FILL, 
                      (GtkAttachOptions) 0, 2, 2);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

    dialog->entry_replace = gtk_entry_new_with_max_length (20);
    gtk_table_attach (GTK_TABLE (table), dialog->entry_replace, 1, 2, 1, 2,
                      (GtkAttachOptions) GTK_FILL | GTK_EXPAND, 
                      (GtkAttachOptions) 0, 2, 2);

    /* search options hbox */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), hbox, TRUE, TRUE, 0);

    dialog->case_sensitive = gtk_check_button_new_with_label (_("Case sensitive"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->case_sensitive, FALSE, FALSE, 0);

    dialog->whole_word = gtk_check_button_new_with_label (_("Whole word"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->whole_word, FALSE, FALSE, 0);

    dialog->word_start = gtk_check_button_new_with_label (_("Match word start"));
    gtk_box_pack_start (GTK_BOX (hbox), dialog->word_start, FALSE, FALSE, 0);

    /* selection only checkbox */
    dialog->selection_only = gtk_check_button_new_with_label (_("Selection only"));
    gtk_box_pack_start (GTK_BOX (dialog->dialog->vbox), 
                        dialog->selection_only, TRUE, TRUE, 0);

    gtk_widget_show_all (dialog->dialog->vbox);

    /* set dialog options */
    gnome_dialog_button_connect (dialog->dialog, 0, setup_replace, dialog);
    gnome_dialog_close_hides (dialog->dialog, TRUE);
    gnome_dialog_set_close (dialog->dialog, TRUE);
    gnome_dialog_set_default (dialog->dialog, 0);

    return dialog;
}


void
scintilla_replace_dialog_destroy (ScintillaReplaceDialog *dialog)
{
    if (dialog->ask_dialog)
        gtk_widget_destroy (GTK_WIDGET (dialog->ask_dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog->dialog));
    g_free (dialog);
}


void
run_replace_dialog (ScintillaObject *sci)
{
    ScintillaReplaceDialog *dialog;

    dialog = gtk_object_get_data (GTK_OBJECT (sci), "replace_dialog");
    if (!dialog) {
        dialog = scintilla_replace_dialog_new (sci);
        gtk_object_set_data (GTK_OBJECT (sci), "replace_dialog", dialog);
    };
    
    if (dialog) {
        gtk_widget_show (GTK_WIDGET (dialog->dialog));
        gdk_window_raise (GTK_WIDGET (dialog->dialog)->window);
        gtk_widget_grab_focus (dialog->entry_find);
    };
}

