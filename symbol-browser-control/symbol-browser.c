/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* symbol-browser widget
 *
 * Copyright (C) 2001 Naba Kumar <kh_naba@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gal/e-table/e-table.h>
#include <gal/e-table/e-tree-scrolled.h>
#include <gal/e-table/e-tree-memory-callbacks.h>
#include <gal/e-table/e-tree-table-adapter.h>
#include <sys/stat.h>
#include <dirent.h>

#include <tm_project.h>
#include "symbol-browser.h"
#include "pixmaps/folder.xpm"
#include "pixmaps/class.xpm"
#include "pixmaps/struct.xpm"
#include "pixmaps/member.xpm"
#include "pixmaps/function.xpm"
#include "pixmaps/enum.xpm"
#include "pixmaps/variable.xpm"
#include "pixmaps/extern_variable.xpm"
#include "pixmaps/unknown.xpm"


#define DIRECTORY_SEP_CHAR '/'
#define DIRECTORY_SEP_STR "/"

#define SYMBOL_BROWSER_SPEC  \
"<ETableSpecification cursor-mode=\"line\" selection-mode=\"browse\" \
		horizontal-scrolling=\"true\"> \
   <ETableColumn model_col=\"0\" _title=\"Symbols\" expansion=\"1.0\" \
		   minimum_width=\"20\" resizable=\"true\" cell=\"tree-string\" \
		   compare=\"string\"/> \
	 <ETableState> \
		 <column source=\"0\"/> \
		 <grouping></grouping> \
	 </ETableState>
</ETableSpecification > "

/* This variable MUST be synchronized with the TMTagType definition */
static gchar*
symbol_types[] = {
	"Unknown", /*! tm_tag_undef_t = 0, < Unknown type */
	"Classes", /*! tm_tag_class_t = 1, < Class declaration */
	"Enums", /*! tm_tag_enum_t = 2, < Enum declaration */
	"Enumerators", /*! tm_tag_enumerator_t = 4, < Enumerator value */
	"Fields", /*! tm_tag_field_t = 8, < Field (Java only) */
	"Functions", /*! tm_tag_function_t = 16, < Function definition */
	"Interfaces", /*! tm_tag_interface_t = 32, < Interface (Java only */
	"Members", /*! tm_tag_member_t = 64, < Member variable of class/struct */
	"Methods", /*! tm_tag_method_t = 128, < Class method (Java only */
	"Namespaces", /*! tm_tag_namespace_t = 256, < Namespace declaration */
	"Packages", /*! tm_tag_package_t = 512, < Package (Java only) */
	"Prototypes", /*! tm_tag_prototype_t = 1024, < Function prototype */
	"Structs", /*! tm_tag_struct_t = 2048, < Struct declaration */
	"Typedefs", /*! tm_tag_typedef_t = 4096, < Typedef */
	"Unions", /*! tm_tag_union_t = 8192, !< Union */
	"Variables", /*! tm_tag_variable_t = 16384, < Variable */
	"ExternVars", /*! tm_tag_externvar_t = 32768, < Extern or forward declaration */
	"Macros", /*! tm_tag_macro_t = 65536, < Macro (withour arguments) */
	"MacrosWithArgs", /*! tm_tag_macro_with_arg_t = 131072, < Parameterized macro */
	NULL
};

enum {
	GO_TO_SIGNAL,
	LAST_SIGNAL
};

static guint gsb_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;

struct _GnomeSymbolBrowserPriv
{
	GtkWidget *etree_sc;
	GtkWidget *etree;
	gchar* directory;

	TMWorkObject* project;
	
	ETreeModel *etree_model;
	ETreePath etree_root;
	GHashTable* etree_sub_nodes;
};

/* Memory Callbacks */
static GdkPixbuf *gsb_icon_at (ETreeModel *model, ETreePath path, void *data);
static gint gsb_col_count (ETreeModel *model, void *data);
static void *gsb_val_at (ETreeModel *model, ETreePath path, int col, void *data);
static void gsb_set_val_at (ETreeModel *model, ETreePath path, int col, const void *val, void *data);
static gboolean gsb_is_editable (ETreeModel *model, ETreePath path, int col, void *data);
static void *gsb_dup_val (ETreeModel *model, int col, const void *val, void *data);
static void gsb_free_val (ETreeModel *model, int col, void *val, void *data);
static void *gsb_init_val (ETreeModel *model, int col, void *data);
static gboolean gsb_val_is_empty (ETreeModel *model, int col, const void *val, void *data);
static char *gsb_val_to_string (ETreeModel *model, int col, const void *val, void *data);

/* Private funtions */
static void gnome_symbol_browser_class_init (GnomeSymbolBrowserClass* klass);
static void gnome_symbol_browser_init (GnomeSymbolBrowser * tree);

static GdkPixbuf *get_image_for_type (gchar* type_name);
static gchar* get_tag_type_name (TMTagType type);

static void gsb_insert_nodes (GnomeSymbolBrowser* tree, ETreePath node, TMSymbol* sym, gint level);
static ETreePath get_sub_node (GnomeSymbolBrowser* tree, TMSymbol* sym);
static void gsb_update_tree (GnomeSymbolBrowser *tree);
static TMSymbol* gsb_sub_node_data_new(TMTagType type);
static void gsb_sub_node_data_destroy(TMSymbol* sym);

static void on_double_click (ETree * et, int row, ETreePath path,
			     int col, GdkEvent * event, gpointer data);
static gboolean destroy_symbol_data_cb (gpointer key, gpointer value, gpointer data);

GtkType
gnome_symbol_browser_get_type (void)
{
	static GtkType type = 0;

	if (type == 0)
	{
		static const GtkTypeInfo info = {
			"GnomeSymbolBrowser",
			sizeof (GnomeSymbolBrowser),
			sizeof (GnomeSymbolBrowserClass),
			(GtkClassInitFunc) gnome_symbol_browser_class_init,
			(GtkObjectInitFunc) gnome_symbol_browser_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		type = gtk_type_unique (gtk_vbox_get_type (), &info);
	}

	return type;
}

static void
gnome_symbol_browser_class_init (GnomeSymbolBrowserClass * klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (gtk_vbox_get_type ());
	object_class->destroy = gnome_symbol_browser_destroy;
	
	gsb_signals[GO_TO_SIGNAL] = gtk_signal_new(
								"go_to",
								GTK_RUN_LAST,
								object_class->type,
								GTK_SIGNAL_OFFSET(GnomeSymbolBrowserClass, go_to),
								gtk_marshal_NONE__INT_POINTER,
								GTK_TYPE_NONE, 2,
								GTK_TYPE_STRING,
								GTK_TYPE_LONG);
	gtk_object_class_add_signals (object_class, gsb_signals, LAST_SIGNAL);
	
	klass->go_to = NULL;
}

static void
gnome_symbol_browser_init (GnomeSymbolBrowser * sb)
{
	GnomeSymbolBrowserPriv *priv;

	priv = g_malloc(sizeof(GnomeSymbolBrowserPriv));
	sb->priv = priv;

	priv->etree_model = e_tree_memory_callbacks_new (
							gsb_icon_at,
							gsb_col_count,
							NULL,
							NULL,
							NULL,
							NULL,
							gsb_val_at,
							gsb_set_val_at,
							gsb_is_editable,
							gsb_dup_val,
							gsb_free_val,
							gsb_init_val,
							gsb_val_is_empty,
							gsb_val_to_string,
							sb);

	e_tree_memory_set_expanded_default (E_TREE_MEMORY (priv->etree_model),
					    FALSE);

	priv->etree_sc =
		e_tree_scrolled_new (priv->etree_model, NULL,
				     SYMBOL_BROWSER_SPEC, NULL);
	priv->etree =
		GTK_WIDGET (e_tree_scrolled_get_tree
			    (E_TREE_SCROLLED (priv->etree_sc)));

	gtk_signal_connect (GTK_OBJECT (priv->etree), "double_click",
			    GTK_SIGNAL_FUNC (on_double_click), sb);

	gtk_box_pack_start (GTK_BOX (sb), priv->etree_sc, TRUE, TRUE, 5);
	gtk_widget_show (priv->etree_sc);

	priv->etree_sub_nodes = g_hash_table_new (g_str_hash, g_str_equal);
	priv->etree_root = NULL;
	priv->directory = NULL;
	priv->project = NULL;
}

GtkWidget*
gnome_symbol_browser_new (void)
{
	return GTK_WIDGET (gtk_type_new (GNOME_TYPE_SYMBOL_BROWSER));
}

void
gnome_symbol_browser_destroy (GtkObject * obj)
{
	GnomeSymbolBrowser* gsb;
	gsb = GNOME_SYMBOL_BROWSER(obj);
	
	g_return_if_fail (gsb != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER(gsb));
	
	g_print ("GSB: In function gnome_symbol_browser_destroy()\n");
	
	gnome_symbol_browser_reset (gsb);
	g_hash_table_destroy (gsb->priv->etree_sub_nodes);
	g_free (gsb->priv);
	GTK_OBJECT_CLASS (parent_class)->destroy (obj);
}

void
on_double_click (ETree * et, int row, ETreePath path, int col,
		 GdkEvent * event, gpointer data)
{
	GnomeSymbolBrowser* gsb;
	TMSymbol *sym;
	gchar *file;
	glong line;
	
	gsb = data;
	sym = e_tree_memory_node_get_data (E_TREE_MEMORY (gsb->priv->etree_model), path);
	file = NULL;
	line = -1;
	
	if (sym) {
		if (sym->tag) {
			if ((sym->tag->type != tm_tag_undef_t) && sym->tag->atts.entry.file) {
				file = sym->tag->atts.entry.file->work_object.file_name;
				line = (glong)sym->tag->atts.entry.line;
			}
		}
	}
	
	if (file) {
		gtk_signal_emit (GTK_OBJECT(gsb), gsb_signals[GO_TO_SIGNAL], file, line);
	}
}

gboolean
gnome_symbol_browser_open_dir(GnomeSymbolBrowser* sb, const gchar* dir)
{
	g_return_val_if_fail(dir != NULL, FALSE);
	g_return_val_if_fail(sb != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_SYMBOL_BROWSER(sb), FALSE);
	
	printf ("In function gnome_symbol_browser_open_dir()\n");

	/* Clear symbol browser */
	gnome_symbol_browser_reset(sb);
	
	sb->priv->directory = g_strdup (dir);
	
	/* Open/Create a new project */
	sb->priv->project = tm_project_new(dir, "Project");
	tm_project_update (sb->priv->project, TRUE, TRUE, TRUE);
	tm_project_save (TM_PROJECT(sb->priv->project));
	gsb_update_tree(sb);
	
	return TRUE;
}

void
gnome_symbol_browser_reset(GnomeSymbolBrowser* gsb)
{
	GnomeSymbolBrowserPriv *priv;
	
	g_return_if_fail (gsb != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER(gsb));
	
	priv = gsb->priv;
	
	gnome_symbol_browser_clear(gsb);
	
	if (priv->directory) {
		g_free(priv->directory);
		priv->directory = NULL;
	}
	if (priv->project) {
		tm_project_save (TM_PROJECT(priv->project));
		tm_project_destroy(TM_PROJECT(priv->project));
		priv->project = NULL;
	}
}

void
gnome_symbol_browser_clear(GnomeSymbolBrowser* gsb)
{
	GnomeSymbolBrowserPriv *priv;
	
	g_return_if_fail (gsb != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER(gsb));
	
	priv = gsb->priv;
	
	e_tree_memory_freeze (E_TREE_MEMORY (priv->etree_model));
	g_hash_table_foreach_remove(priv->etree_sub_nodes, destroy_symbol_data_cb, gsb);
	
	if (priv->etree_root) {
		e_tree_memory_node_remove (E_TREE_MEMORY (priv->etree_model), priv->etree_root);
		priv->etree_root = NULL;
	}
	g_hash_table_foreach_remove(priv->etree_sub_nodes, destroy_symbol_data_cb, gsb);
	
	e_tree_memory_thaw (E_TREE_MEMORY (priv->etree_model));
}

static gboolean
destroy_symbol_data_cb (gpointer key, gpointer value, gpointer data)
{
	GnomeSymbolBrowser* gsb = data;
	ETreePath path = value;
	TMSymbol *sym = 
		e_tree_memory_node_get_data (E_TREE_MEMORY (gsb->priv->etree_model), path);
	gsb_sub_node_data_destroy(sym);
	g_free (key);
	return TRUE;
}

static void
gsb_update_tree (GnomeSymbolBrowser *tree)
{
	GnomeSymbolBrowserPriv *priv;
	
	priv = tree->priv;
	
	e_tree_memory_freeze (E_TREE_MEMORY (priv->etree_model));
	
	if (priv->project) {
		TMSymbol* sym = TM_PROJECT(priv->project)->symbol_tree;
		if (sym) {
			gsb_insert_nodes (tree, priv->etree_root, sym, 0);
		} else {
			g_warning ("Project contains no detectable symbols.");
		}
	}
	
	e_tree_memory_thaw (E_TREE_MEMORY (priv->etree_model));
}

static void
symbol_print(TMSymbol *sym, guint level)
{
	int i;

	g_assert(sym);
	
	for (i=0; i < level; ++i)
		fputc('\t', stderr);
	fprintf(stderr, "%s ", (sym->tag)?sym->tag->name:"Root");
	fprintf(stderr, "(Tag:%d)\n", (sym->tag)?sym->tag->type:-1);
}

static void
gsb_insert_nodes (GnomeSymbolBrowser* tree, ETreePath node, TMSymbol* sym, gint level)
{
	GnomeSymbolBrowserPriv *priv;
	ETreePath etp;
	GSList *tmp;
	
	priv = tree->priv;
	
	/*
	g_print ("In function gsb_insert_nodes: ");
	symbol_print(sym, level);
	*/
	
	switch(level) {
		case 0:
			priv->etree_root =
				e_tree_memory_node_insert (E_TREE_MEMORY (priv->etree_model), NULL, 0, NULL);
			etp = priv->etree_root;
			break;
		case 1:
			etp = get_sub_node (tree, sym);
			etp = e_tree_memory_node_insert (E_TREE_MEMORY (priv->etree_model), etp, 0, sym);
			break;
		case 2:
			etp = e_tree_memory_node_insert (E_TREE_MEMORY (priv->etree_model), node, 0, sym);
			break;
		case 3:
		case 4:
		default:
			g_warning ("Recursion level in gsb_insert_nodes exceeds 3 !!!");
			return;
	}
	for (tmp = sym->children; tmp; tmp = g_slist_next(tmp))
		gsb_insert_nodes (tree, etp, TM_SYMBOL(tmp->data), level+1);
}

static TMSymbol*
gsb_sub_node_data_new(TMTagType type)
{
	TMSymbol* symbol_data;
	symbol_data = g_new0(TMSymbol, 1);
	symbol_data->tag = g_new0(TMTag, 1);
	symbol_data->tag->name = g_strdup(get_tag_type_name(type));
	symbol_data->tag->type = type;
	symbol_data->tag->atts.entry.file = NULL;
	return symbol_data;
}

static void
gsb_sub_node_data_destroy(TMSymbol* sym)
{
	g_return_if_fail(sym != NULL);
	g_free(sym->tag->name);
	g_free(sym->tag);
	g_free(sym);
}

static ETreePath
get_sub_node (GnomeSymbolBrowser* tree, TMSymbol* sym)
{
	GnomeSymbolBrowserPriv *priv;
	ETreePath node;
	TMSymbol* symbol_data;
	
	priv = tree->priv;
	
	g_return_val_if_fail (sym != NULL, priv->etree_root);
	
	if (sym->tag == NULL)
		return priv->etree_root;
	
	node = g_hash_table_lookup (priv->etree_sub_nodes, get_tag_type_name(sym->tag->type));
	if (node)
		return node;
	
	symbol_data = gsb_sub_node_data_new(sym->tag->type);
	
	node = e_tree_memory_node_insert (E_TREE_MEMORY (priv->etree_model),
				priv->etree_root, 0, symbol_data);
	g_hash_table_insert (priv->etree_sub_nodes,
				g_strdup (get_tag_type_name(sym->tag->type)), node);
	return node;
}

static GdkPixbuf*
gsb_icon_at (ETreeModel *model, ETreePath path, void *data)
{
	TMSymbol *sym = e_tree_memory_node_get_data (E_TREE_MEMORY (model), path);

	if (sym) {
		return get_image_for_type(get_tag_type_name(sym->tag->type));
	} else {
		return get_image_for_type("Folders");
	}
}

static gint
gsb_col_count (ETreeModel *model, void *data)
{
	return 1;
}

static void*
gsb_dup_val (ETreeModel *model, int col, const void *val, void *data)
{
	void *new_val = NULL;
	
	g_print ("GBF: dup_val\n");

	switch (col) {
	case 0:
		new_val = g_strdup ((char *)val);
		break;
	default:
		g_warning ("Unknown column");
	}
	return new_val;
}

static void
gsb_free_val (ETreeModel *model, int col, void *val, void *data)
{
	g_print ("GSB: free_val\n");

	switch (col) {
	case 0:
		g_free (val);
		break;
	default:
		g_warning ("Unknown column");
	}	
}

static void*
gsb_init_val (ETreeModel *model, int col, void *data)
{
	void *init_val = NULL;

	g_print ("GSB: init_val %d\n", col);
	
	switch (col) {
	case 0:
		init_val = NULL;
	default:
		g_warning ("Unknown column");
	}
	
	return init_val;
}

static gboolean
gsb_val_is_empty (ETreeModel *model, int col, const void *val, void *data)
{
	g_print ("GSB: val_is_empty %d\n", col);

	switch (col) {
	case 0:
		return val == NULL;
		break;
	default:
		g_warning ("Unknown column");
	}
	return FALSE;
}

static char*
gsb_val_to_string (ETreeModel *model, int col, const void *val, void *data)
{
	switch (col) {
	case 0:
		return g_strdup (val);
		break;
	default:
		g_warning ("Unknown column");
	}
	return NULL;
}

static void*
gsb_val_at (ETreeModel *model, ETreePath path, int col, void *data)
{
	TMSymbol *sym = e_tree_memory_node_get_data (E_TREE_MEMORY (model), path);
	
	/* g_print ("GSB: Val at col %d\n", col); */
	
	if (sym) {
		if (sym->tag) {
			if (sym->tag->type != tm_tag_undef_t) {
				return g_strdup(sym->tag->name);
			} else {
				return g_strdup("Undefined");
			}
		} else {
			return g_strdup("<No Symbol>");
		}
	} else {
		return g_strdup("Tags");
	}
}

static void
gsb_set_val_at (ETreeModel *model, ETreePath path, int col, const void *val, void *data)
{
}

static gboolean
gsb_is_editable (ETreeModel *model, ETreePath path, int col, void *data)
{
	return FALSE;
}

GdkPixbuf *
get_image_for_type (gchar* type_name) 
{
	static GHashTable *icons = NULL;
	GdkPixbuf *pixbuf;

	if (!icons) {
		icons = g_hash_table_new (g_str_hash, g_str_equal);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)folder_xpm);
		g_hash_table_insert (icons, g_strdup ("Tags"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)folder_xpm);
		g_hash_table_insert (icons, g_strdup ("Folders"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)unknown_xpm);
		g_hash_table_insert (icons, g_strdup ("Undefined"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)unknown_xpm);
		g_hash_table_insert (icons, g_strdup ("Unknown"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)class_xpm);
		g_hash_table_insert (icons, g_strdup ("Classes"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)struct_xpm);
		g_hash_table_insert (icons, g_strdup ("Structs"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)member_xpm);
		g_hash_table_insert (icons, g_strdup ("Members"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)function_xpm);
		g_hash_table_insert (icons, g_strdup ("Functions"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)enum_xpm);
		g_hash_table_insert (icons, g_strdup ("Enums"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)variable_xpm);
		g_hash_table_insert (icons, g_strdup ("Variables"), pixbuf);
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)extern_variable_xpm);
		g_hash_table_insert (icons, g_strdup ("ExternVars"), pixbuf);
	}

	pixbuf = g_hash_table_lookup (icons, type_name);
	if (pixbuf)
		return pixbuf;
	
	return g_hash_table_lookup (icons, "Unknown");
}

static gchar*
get_tag_type_name (TMTagType type) 
{
	gint count = 1;
	gint32 type_32 = (gint32)type;
	
	g_return_val_if_fail (type < tm_tag_max_t, symbol_types[0]);
	
	if (type_32) {
		while (!(type_32 & 1)) {
			type_32 >>= 1;
			count ++;
		}
		
		/* g_print ("Symbol type name: %s\n", symbol_types[count]);*/
		
		return symbol_types[count];
	} else {
		return symbol_types[0];
	}
}
