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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo.h>
#include <gtk/gtktreeview.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

#include <tm_project.h>
#include <tm_tagmanager.h>
#include "symbol-browser.h"
#include "../gdl/libgdlmarshal.h"
#include "../gdl/gdl-icons.h"

#include "pixmaps/sv_unknown.xpm"
#include "pixmaps/sv_class.xpm"
#include "pixmaps/sv_struct.xpm"
#include "pixmaps/sv_function.xpm"
#include "pixmaps/sv_macro.xpm"
#include "pixmaps/sv_variable.xpm"
#include "pixmaps/sv_typedef.xpm"

#include "pixmaps/sv_enum.xpm"
#include "pixmaps/sv_enumerator.xpm"

#include "pixmaps/sv_private_fun.xpm"
#include "pixmaps/sv_protected_fun.xpm"
#include "pixmaps/sv_public_fun.xpm"

#include "pixmaps/sv_private_var.xpm"
#include "pixmaps/sv_protected_var.xpm"
#include "pixmaps/sv_public_var.xpm"

#include "pixmaps/sv_static_fun.xpm"
#include "pixmaps/sv_static_var.xpm"

/* This variable MUST be synchronized with the TMTagType definition */
static gchar*
symbol_types[] = {
	N_("Unknown"), /*! tm_tag_undef_t = 0, < Unknown type */
	N_("Classes"), /*! tm_tag_class_t = 1, < Class declaration */
	N_("Enums"), /*! tm_tag_enum_t = 2, < Enum declaration */
	N_("Enumerators"), /*! tm_tag_enumerator_t = 4, < Enumerator value */
	N_("Fields"), /*! tm_tag_field_t = 8, < Field (Java only) */
	N_("Functions"), /*! tm_tag_function_t = 16, < Function definition */
	N_("Interfaces"), /*! tm_tag_interface_t = 32, < Interface (Java only */
	N_("Members"), /*! tm_tag_member_t = 64, < Member variable of class/struct */
	N_("Methods"), /*! tm_tag_method_t = 128, < Class method (Java only */
	N_("Namespaces"), /*! tm_tag_namespace_t = 256, < Namespace declaration */
	N_("Packages"), /*! tm_tag_package_t = 512, < Package (Java only) */
	N_("Prototypes"), /*! tm_tag_prototype_t = 1024, < Function prototype */
	N_("Structs"), /*! tm_tag_struct_t = 2048, < Struct declaration */
	N_("Typedefs"), /*! tm_tag_typedef_t = 4096, < Typedef */
	N_("Unions"), /*! tm_tag_union_t = 8192, !< Union */
	N_("Variables"), /*! tm_tag_variable_t = 16384, < Variable */
	N_("ExternVars"), /*! tm_tag_externvar_t = 32768, < Extern or forward declaration */
	N_("Macros"), /*! tm_tag_macro_t = 65536, < Macro (withour arguments) */
	N_("MacrosWithArgs"), /*! tm_tag_macro_with_arg_t = 131072, < Parameterized macro */
	NULL
};

enum {
	SYMBOL_NAME = 0,
	COMBO_N_COLUMNS
};

static GtkVBoxClass *parent_class = NULL;

struct _GnomeSymbolBrowserPriv {
	GtkWidget *tree_sw;
	GtkWidget *tree;
	GtkWidget *symbol_combo;
	
	gchar *directory;

	TMWorkObject *project;
	TMWorkObject *tm_file;

	GtkTreeModel *tree_model;
	GtkListStore *combo_store;
	GHashTable *symbol_hash;

	BonoboListener *listener;

	BonoboEventSource *event_source;

	GdlIcons *icons;
};

typedef enum {
	GSB_TREE_FOLDER,
	GSB_TREE_ROOT,
	GSB_TREE_SYMBOL,
} GsbTreeNodeType;

typedef struct _GsbTreeNodeData GsbTreeNodeData;
struct _GsbTreeNodeData {
	GsbTreeNodeType type;
	TMSymbol *symbol;
};

#define GSB_TREE_NODE_DATA_TYPE gsb_tree_node_data_get_type ()


static void gnome_symbol_browser_class_init (GnomeSymbolBrowserClass *klass);
static void gnome_symbol_browser_init       (GnomeSymbolBrowser      *sb);
static void gnome_symbol_browser_finalize   (GObject *obj);


static GdkPixbuf *get_image_for_type_key    (GnomeSymbolBrowser *gsb,
					     gchar              *type_name);
static gchar *get_tag_type_name             (TMTagType  type);
static gchar *get_tag_type_name_key         (TMSymbol  *symbol);
static gboolean gsb_goto_tag                (GnomeSymbolBrowser* gsb, const gchar* qual_name);
static void gsb_goto_file_line              (GnomeSymbolBrowser* gsb, const gchar* file, gint line);

static gboolean destroy_symbol_data_cb (gpointer key,
					gpointer value,
					gpointer data);
static void gsb_update_tree (GnomeSymbolBrowser *gsb);
static void gsb_insert_nodes (GnomeSymbolBrowser *gsb,
			      GtkTreeIter        *parent,
			      TMSymbol           *symbol,
			      gint                level);

static GsbTreeNodeData *gsb_tree_node_data_new (GsbTreeNodeType type,
						TMSymbol       *symbol);
static GsbTreeNodeData *gsb_tree_node_data_copy (GsbTreeNodeData *src);
static void gsb_tree_node_data_free (GsbTreeNodeData *node);
static GType gsb_tree_node_data_get_type (void);
static void gsb_tree_node_set_pixbuf (GtkTreeViewColumn *tree_column,
				      GtkCellRenderer   *cell,
				      GtkTreeModel      *model,
				      GtkTreeIter       *iter,
				      gpointer           data);
static void gsb_tree_node_set_text (GtkTreeViewColumn *tree_column,
				    GtkCellRenderer   *cell,
				    GtkTreeModel      *model,
				    GtkTreeIter       *iter,
				    gpointer           data);

static void row_activated_cb (GtkTreeView       *tree_view,
			      GtkTreePath       *path,
			      GtkTreeViewColumn *column,
			      gpointer           user_data);

/* ----------------------------------------------------------------------
 * Symbol Tree Functions
 * ---------------------------------------------------------------------- */
static gboolean
destroy_symbol_data_cb (gpointer key,
			gpointer value,
			gpointer data)
{
	g_free (key);
	gtk_tree_path_free (value);
	return TRUE;
}

static void
gsb_update_tree (GnomeSymbolBrowser *gsb)
{
	GnomeSymbolBrowserPriv *priv;
	TMSymbol *symbol_tree;
	
	priv = gsb->priv;

	if (priv->project) {
		/* TMSymbol *symbol = TM_PROJECT (priv->project)->symbol_tree; */
		symbol_tree = tm_symbol_tree_new(priv->project->tags_array);

		if (symbol_tree) {
			GtkTreePath *root = gtk_tree_path_new_first ();

			gsb_insert_nodes (gsb, NULL, symbol_tree, 0);

			/* expand root node. */
			gtk_tree_view_expand_row (GTK_TREE_VIEW (gsb->priv->tree),
						  root, FALSE);
			gtk_tree_path_free (root);
		} else
			g_warning ("Project contains no detectable symbols.");
	}
}

static void
gsb_insert_nodes (GnomeSymbolBrowser *gsb,
		  GtkTreeIter        *parent,
		  TMSymbol           *symbol,
		  gint                level)
{
	GnomeSymbolBrowserPriv *priv;
	GsbTreeNodeData        *node;
	GtkTreeIter             iter, iter2;
	GtkTreePath            *path;
	gint                    i;
	gboolean                has_children;
	
	priv = gsb->priv;

	/*symbol_print (symbol, level);*/

	switch (level) {
		case 0: /* root node (folder). */
			node = gsb_tree_node_data_new (GSB_TREE_FOLDER, NULL);
			gtk_tree_store_append (GTK_TREE_STORE (priv->tree_model),
					       &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (priv->tree_model),
					    &iter, 0, node, -1);
			break;
		case 1: /* structure, method or variables root nodes. */
			
			path = g_hash_table_lookup(priv->symbol_hash, get_tag_type_name_key (symbol));
		
			if (!path) {
				node = gsb_tree_node_data_new (GSB_TREE_ROOT, symbol);
				gtk_tree_store_append (GTK_TREE_STORE (priv->tree_model), &iter2, parent);
				gtk_tree_store_set (GTK_TREE_STORE (priv->tree_model), &iter2, 0, node, -1);
				
				/* Insert tag type root node into hashtable */
				path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->tree_model),
								&iter2);
				g_hash_table_insert (priv->symbol_hash,
							 g_strdup (get_tag_type_name_key (symbol)),
							 path);
			} else {
				gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->tree_model), &iter2, path);
			}
			parent = &iter2;
			
		case 2:
			node = gsb_tree_node_data_new (GSB_TREE_SYMBOL, symbol);
			gtk_tree_store_append (GTK_TREE_STORE (priv->tree_model), &iter, parent);
			gtk_tree_store_set (GTK_TREE_STORE (priv->tree_model), &iter, 0, node, -1);

			break;
		default:
			g_warning ("Recursion level in gsb_insert_nodes exceeds 3 !!!");
			return;
	}

	/* recurse. */
	if (level == 0 ||
		((symbol->tag) &&
		(symbol->tag->type != tm_tag_undef_t) &&
		(symbol->tag->type != tm_tag_function_t) &&
		(symbol->info.children) &&
		(symbol->info.children->len > 0)))
			has_children = TRUE;
		else
			has_children = FALSE;

	if (has_children) {
		for (i=0; i < symbol->info.children->len; ++i) {
			TMSymbol *sym = TM_SYMBOL(symbol->info.children->pdata[i]);
			if (!sym || ! sym->tag || !sym->tag->atts.entry.file)
				continue;
			gsb_insert_nodes (gsb, &iter, TM_SYMBOL (sym), level+1);
		}
	}
}

static GsbTreeNodeData *
gsb_tree_node_data_new (GsbTreeNodeType type,
			TMSymbol       *symbol)
{
	GsbTreeNodeData *node = g_new0 (GsbTreeNodeData, 1);

	node->type = type;
	node->symbol = symbol;

	return node;
}

static void
gsb_tree_node_data_free (GsbTreeNodeData *node)
{
	g_free (node);
}

static GsbTreeNodeData *
gsb_tree_node_data_copy (GsbTreeNodeData *src)
{
	GsbTreeNodeData *node;

	node = g_new (GsbTreeNodeData, 1);
	node->type = src->type;
	node->symbol = src->symbol;

	return node;
}

static GType
gsb_tree_node_data_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0)
		our_type = g_boxed_type_register_static ("GsbTreeNodeData",
							 (GBoxedCopyFunc) gsb_tree_node_data_copy,
							 (GBoxedFreeFunc) gsb_tree_node_data_free);

	return our_type;
}

static void
gsb_tree_node_set_pixbuf (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	GsbTreeNodeData *node;
	GdkPixbuf *pixbuf = NULL;
	gchar *type_key;

	gtk_tree_model_get (model, iter, 0, &node, -1);

	switch (node->type) {
	case GSB_TREE_FOLDER:
		pixbuf = get_image_for_type_key (GNOME_SYMBOL_BROWSER (data), "Folders");
		break;
	case GSB_TREE_ROOT:
		type_key = get_tag_type_name_key(node->symbol);
		pixbuf = get_image_for_type_key (GNOME_SYMBOL_BROWSER (data), type_key);
		break;
	case GSB_TREE_SYMBOL:
		type_key = get_tag_type_name_key(node->symbol);
		pixbuf = get_image_for_type_key (GNOME_SYMBOL_BROWSER (data), type_key);
		break;
	}

	g_object_set (GTK_CELL_RENDERER (cell), "pixbuf", 
		      pixbuf, NULL);

	gsb_tree_node_data_free (node);
}

static void
gsb_tree_node_set_text (GtkTreeViewColumn *tree_column,
			GtkCellRenderer   *cell,
			GtkTreeModel      *model,
			GtkTreeIter       *iter,
			gpointer           data)
{
	GsbTreeNodeData *node;
	gchar *node_text = NULL;

	gtk_tree_model_get (model, iter, 0, &node, -1);

	switch (node->type) {
	case GSB_TREE_FOLDER:
		node_text = g_strdup (_("Tags"));
		break;
	case GSB_TREE_ROOT:
		if (node->symbol) {
			if (node->symbol->tag) {
				if (node->symbol->tag->type != tm_tag_undef_t)
					node_text = g_strdup (get_tag_type_name(node->symbol->tag->type));
				else
					node_text = g_strdup (_("Undefined"));
			} else
				node_text = g_strdup (_("<No Symbol>"));
		}
		break;
	case GSB_TREE_SYMBOL:
		if (node->symbol) {
			if (node->symbol->tag) {
				if (node->symbol->tag->type != tm_tag_undef_t)
					node_text = g_strdup (node->symbol->tag->name);
				else
					node_text = g_strdup (_("Undefined"));
			} else
				node_text = g_strdup (_("<No Symbol>"));
		}
		break;
	}

	g_object_set (GTK_CELL_RENDERER (cell), "text", 
		      node_text, NULL);

	gsb_tree_node_data_free (node);
}

/* ----------------------------------------------------------------------
 * Callbacks
 * ---------------------------------------------------------------------- */

static void
symbol_combo_changed_cb (GtkComboBox* combo, gpointer user_data)
{
	GnomeSymbolBrowser *gsb;
	gchar *string = NULL;
	GtkTreeIter iter;

	gsb = GNOME_SYMBOL_BROWSER(user_data);

	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (gsb->priv->combo_store),
				    &iter,
				    SYMBOL_NAME, &string,
				    -1);
		if (string && strlen (string) > 0)
			gsb_goto_tag(gsb, string);
		g_free (string);
	}
}

static void
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           user_data)
{
	GnomeSymbolBrowser *gsb = user_data;
	GsbTreeNodeData *node;
	TMSymbol *symbol;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *file = NULL;
	glong line = -1;

	model = gtk_tree_view_get_model (tree_view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &node, -1);

	symbol = node->symbol;

	if (symbol && symbol->tag) {
		if ((symbol->tag->type != tm_tag_undef_t) && symbol->tag->atts.entry.file) {
			file = symbol->tag->atts.entry.file->work_object.file_name;
			line = (glong) symbol->tag->atts.entry.line;
		}
	}

	if (file) {
		gsb_goto_file_line(gsb, file, line);
	} else
		g_warning ("No Symbol found!\n");
}

/* ----------------------------------------------------------------------
 * Utility Functions
 * ---------------------------------------------------------------------- */
static GdkPixbuf *
get_image_for_type_key (GnomeSymbolBrowser *gsb,
			gchar *type_name_key) 
{
	static GHashTable *icons = NULL;
	GdkPixbuf *pixbuf;

	if (!icons) {
		
		icons = g_hash_table_new (g_str_hash, g_str_equal);
		
		pixbuf = gdl_icons_get_folder_icon (gsb->priv->icons);
		g_object_ref (pixbuf);
		g_hash_table_insert (icons, g_strdup ("Tags"), pixbuf);
		g_hash_table_insert (icons, g_strdup ("Folders"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_unknown_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_none_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_class_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_class_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_struct_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_struct_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_enum_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_enum_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_enumerator_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_enumerator_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_macro_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_macro_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_function_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_function_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_variable_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_variable_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_typedef_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_typedef_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_private_fun_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_private_fun_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_protected_fun_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_protected_fun_t"), pixbuf);
		
		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_public_fun_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_public_fun_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_private_var_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_private_var_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_protected_var_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_protected_var_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_public_var_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_public_var_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_static_fun_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_static_fun_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_static_var_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_static_var_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_public_fun_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_public_fun_t"), pixbuf);

		pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)sv_public_var_xpm);
		g_hash_table_insert (icons, g_strdup ("sv_public_var_t"), pixbuf);

	}

	pixbuf = g_hash_table_lookup (icons, type_name_key);
	if (pixbuf)
		return pixbuf;

	return g_hash_table_lookup (icons, "sv_none_t");
}

static gchar*
get_tag_type_name_key (TMSymbol *sym)
{
	gchar access;
	gchar *type;

	if (!sym || !sym->tag || (tm_tag_file_t == sym->tag->type))
		return "sv_none_t";
	
	access = sym->tag->atts.entry.access;
	switch (sym->tag->type)
	{
		case tm_tag_class_t:
			type = "sv_class_t";
			break;
		case tm_tag_struct_t:
			type = "sv_struct_t";
			break;
		case tm_tag_enum_t:
			type = "sv_enum_t";
			break;
		case tm_tag_enumerator_t:
			type = "sv_enumerator_t";
			break;
		case tm_tag_typedef_t:
			type = "sv_typedef_t";
			break;
		case tm_tag_function_t:
		case tm_tag_prototype_t:
			if ((sym->info.equiv) && (TAG_ACCESS_UNKNOWN == access))
				access = sym->info.equiv->atts.entry.access;
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					type = "sv_private_fun_t";
					break;
				case TAG_ACCESS_PROTECTED:
					type = "sv_protected_fun_t";
					break;
				case TAG_ACCESS_PUBLIC:
					type = "sv_public_fun_t";
					break;
				default:
					type = "sv_function_t";
					break;
			}
			break;
		case tm_tag_member_t:
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					type = "sv_private_var_t";
					break;
				case TAG_ACCESS_PROTECTED:
					type = "sv_protected_var_t";
					break;
				case TAG_ACCESS_PUBLIC:
					type = "sv_public_var_t";
					break;
				default:
					type = "sv_variable_t";
					break;
			}
			break;
		case tm_tag_externvar_t:
		case tm_tag_variable_t:
			type = "sv_variable_t";
			break;
		case tm_tag_macro_t:
		case tm_tag_macro_with_arg_t:
			type = "sv_macro_t";
			break;
		default:
			type = "sv_none_t";
			break;
	}
	return type;
}

static gchar *
get_tag_type_name (TMTagType type) 
{
	gint count = 1;
	gint32 type_32 = (gint32)type;

	g_return_val_if_fail (type < tm_tag_max_t, _(symbol_types[0]));

	if (type_32) {
		while (!(type_32 & 1)) {
			type_32 >>= 1;
			count ++;
		}

		/* g_print ("Symbol type name: %s\n", symbol_types[count]);*/

		return _(symbol_types[count]);
	} else
		return _(symbol_types[0]);
}

#if 0 /* used for debugging */
static void
symbol_print (TMSymbol *symbol,
	      guint     level)
{
	int i;

	g_assert (symbol != NULL);
	
	for (i=0; i < level; i++)
		fputc ('\t', stderr);

	fprintf (stderr, "%s ", (symbol->tag) ? symbol->tag->name : "Root");
	fprintf (stderr, "(Tag:%d)\n", (symbol->tag) ? symbol->tag->type : -1);
}
#endif

static gboolean
gsb_goto_tag(GnomeSymbolBrowser* gsb, const gchar* qual_name)
{
	char *name;
	char *scope;
	char *typestring;
	int type;
	guint i;
	int cmp;
	TMTag *tag;

	if (!qual_name || !gsb || !gsb->priv->tm_file || !gsb->priv->tm_file->tags_array
	  || (0 == gsb->priv->tm_file->tags_array->len))
		return FALSE;
	
	scope = g_strdup(qual_name);
	name = strrchr(scope, '.');
	if (name)
	{
		*name = '\0';
		++ name;
		
	}
	else
	{
		name = scope;
		scope = NULL;
	}

	type = -1;
	typestring =  strstr(name, " - [");
	if (typestring)
	{
		gchar* type_name;
		gchar* ptr;
		
		*typestring = '\0';
		typestring = typestring + 4;
		type_name = g_strdup(typestring);
		ptr = type_name;
		while (*ptr && *ptr != ']') {*ptr = tolower(*ptr); ptr++;}
		*ptr = '\0';
		type = tm_tag_name_type(type_name);
		g_free(type_name);
	}
	
	for (i = 0; i < gsb->priv->tm_file->tags_array->len; ++i)
	{
		tag = TM_TAG(gsb->priv->tm_file->tags_array->pdata[i]);
		cmp = strcmp(name, tag->name);
		if (0 == cmp && (type == tag->type || type == -1))
		{
			if ((!scope || !tag->atts.entry.scope) || (0 == strcmp(scope, tag->atts.entry.scope)))
			{
				gsb_goto_file_line(gsb,
				                   tag->atts.entry.file->work_object.file_name,
				                   tag->atts.entry.line);
				g_free(scope?scope:name);
				return TRUE;
			}
		}
		else if (cmp < 0)
		{
			g_free(scope?scope:name);
			return FALSE;
		}
	}
	g_free(scope?scope:name);
	return FALSE;
}

static void
gsb_goto_file_line(GnomeSymbolBrowser* gsb, const gchar* file, gint line)
{
	BonoboArg *arg = bonobo_arg_new (BONOBO_ARG_STRING);
	gchar *location = g_strdup_printf ("%s:%i", file, line);
	BONOBO_ARG_SET_STRING (arg, location);

	bonobo_event_source_notify_listeners (gsb->priv->event_source,
						  "go-to",
						  arg, NULL);
	bonobo_arg_release (arg);
	g_free (location);
}

static const GString*
get_qualified_tag_name(const TMTag *tag)
{
	static GString *s = NULL;
	gchar *tag_name;

	g_return_val_if_fail((tag && tag->name), NULL);
	if (!s)
		s = g_string_sized_new(255);
	g_string_assign(s, "");
	if (tag->atts.entry.scope)
		g_string_append_printf(s, "%s.", tag->atts.entry.scope);
	g_string_append_printf(s, "%s", tag->name);
	
	tag_name = g_strdup(tm_tag_type_name(tag));
	if (tag_name) {
		char* ptr = tag_name;
		while(*ptr) {*ptr = toupper(*ptr); ptr++;}
		/* Hack to allow anjuta to find both prototypes and functions */
		g_string_append_printf(s, " - [%s]", tag_name);
		g_free(tag_name);
	}
	
	return s;
}

static void
gsb_fill_symbol_combo (GnomeSymbolBrowser* gsb, gchar* filepath, guint tag_types)
{
	TMTag *tag;
	guint i;
	GtkTreeIter iter;
	GList *tag_names = NULL;

	if ((!gsb->priv->tm_file) || (!gsb->priv->tm_file->tags_array) ||
	    (gsb->priv->tm_file->tags_array->len <= 0))
		return;

	for (i=0; i < gsb->priv->tm_file->tags_array->len; ++i) {
		tag = TM_TAG(gsb->priv->tm_file->tags_array->pdata[i]);
		if (tag->type & tag_types) {
			tag_names = g_list_prepend (tag_names, 
						    g_strdup (get_qualified_tag_name(tag)->str));
		}
	}
	tag_names = g_list_sort(tag_names, (GCompareFunc) strcmp);

	while (tag_names)
	{
		gtk_list_store_append (gsb->priv->combo_store, &iter);
		gtk_list_store_set (gsb->priv->combo_store, &iter,
				    SYMBOL_NAME, tag_names->data,
				    -1);
		g_free (tag_names->data);
		tag_names = g_list_delete_link (tag_names, tag_names);
	}
}

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

static void
gnome_symbol_browser_class_init (GnomeSymbolBrowserClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	g_object_class->finalize = gnome_symbol_browser_finalize;
}

static void
gnome_symbol_browser_init (GnomeSymbolBrowser *sb)
{
	GnomeSymbolBrowserPriv *priv;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	priv = g_new0 (GnomeSymbolBrowserPriv, 1);
	sb->priv = priv;

	/* create tree model. */
	priv->tree_model = 
		GTK_TREE_MODEL (gtk_tree_store_new (1, GSB_TREE_NODE_DATA_TYPE));

	/* create scrolled window. */
	priv->tree_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->tree_sw),
					     GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->tree_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	/* create tree view. */
	priv->tree = gtk_tree_view_new_with_model (priv->tree_model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->tree), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->tree), FALSE);

	gtk_container_add (GTK_CONTAINER (priv->tree_sw), priv->tree);

	g_signal_connect (G_OBJECT (priv->tree), "row_activated",
			  G_CALLBACK (row_activated_cb), sb);

	/* set renderer for files column. */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, "Symbols");

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, 
						 renderer, 
						 gsb_tree_node_set_pixbuf, 
						 sb, 
						 NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, 
						 renderer, 
						 gsb_tree_node_set_text, 
						 sb, 
						 NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree), column);

	gtk_box_pack_start (GTK_BOX (sb), priv->tree_sw, TRUE, TRUE, 0);
	gtk_widget_show_all (priv->tree_sw);
	
	/* Create File Symbol Combo List */
	priv->combo_store = gtk_list_store_new (COMBO_N_COLUMNS, G_TYPE_STRING);
	priv->symbol_combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->combo_store));

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->symbol_combo),
				    renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->symbol_combo),
					renderer,
					"text", SYMBOL_NAME,
					NULL);

	g_signal_connect (priv->symbol_combo, "changed", 
			  G_CALLBACK (symbol_combo_changed_cb), sb);

	gtk_widget_ref (priv->symbol_combo);
	gtk_widget_set_size_request (priv->symbol_combo, 300, -1);
	gtk_widget_show (priv->symbol_combo);

	priv->symbol_hash = g_hash_table_new (g_str_hash, g_str_equal);
	priv->directory = NULL;
	priv->project = NULL;
	priv->tm_file = NULL;
	priv->event_source = bonobo_event_source_new ();
	priv->icons = gdl_icons_new (16);
}

static void
gnome_symbol_browser_finalize (GObject *object)
{
	GnomeSymbolBrowser *gsb;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER (object));

	gsb = GNOME_SYMBOL_BROWSER (object);

	gnome_symbol_browser_reset (gsb);
	
	g_hash_table_destroy (gsb->priv->symbol_hash);
	gtk_widget_unref(gsb->priv->symbol_combo);
	g_object_unref (gsb->priv->icons);

	g_free (gsb->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* ----------------------------------------------------------------------
 * Public interface 
 * ---------------------------------------------------------------------- */

GtkWidget *
gnome_symbol_browser_new (void)
{
	return GTK_WIDGET (g_object_new (GNOME_TYPE_SYMBOL_BROWSER, NULL));
}

GType
gnome_symbol_browser_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (GnomeSymbolBrowserClass),
			NULL, /* base_init. */
			NULL, /* base_finalize. */
			(GClassInitFunc) gnome_symbol_browser_class_init,
			NULL, /* class_finalize. */
			NULL, /* class_data. */
			sizeof (GnomeSymbolBrowser),
			0, /* n_preallocs. */
			(GInstanceInitFunc) gnome_symbol_browser_init,
			NULL, /* value_table. */            
		};

		type = g_type_register_static (GTK_TYPE_VBOX, "GnomeSymbolBrowser", &info, 0);
	}

	return type;
}

/* Returns the symbol combo for control creation */
GtkWidget*
gnome_symbol_browser_get_symbol_combo(GnomeSymbolBrowser *gsb)
{
	
	g_return_val_if_fail (gsb != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SYMBOL_BROWSER (gsb), NULL);

	return gsb->priv->symbol_combo;
}

void
gnome_symbol_browser_reset(GnomeSymbolBrowser* gsb)
{
	GnomeSymbolBrowserPriv *priv;

	g_return_if_fail (gsb != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER (gsb));

	priv = gsb->priv;

	gnome_symbol_browser_clear (gsb);

	if (priv->directory) {
		g_free (priv->directory);
		priv->directory = NULL;
	}

	if (priv->project) {
		tm_project_save (TM_PROJECT (priv->project));
		tm_project_free (TM_PROJECT (priv->project));
		priv->project = NULL;
	}
}

gboolean
gnome_symbol_browser_open_dir (GnomeSymbolBrowser *gsb,
			       const gchar        *dir)
{
	g_return_val_if_fail (gsb != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SYMBOL_BROWSER (gsb), FALSE);
	g_return_val_if_fail (dir != NULL, FALSE);

	/* Clear symbol browser */
	gnome_symbol_browser_reset (gsb);

	gsb->priv->directory = g_strdup (dir);

	/* Open/Create a new project */
	gsb->priv->project = tm_project_new (dir, NULL, NULL, FALSE);
	tm_project_update (gsb->priv->project, TRUE, TRUE, TRUE);
	tm_project_save (TM_PROJECT (gsb->priv->project));
	gsb_update_tree (gsb);

	return TRUE;
}

void
gnome_symbol_browser_clear (GnomeSymbolBrowser *gsb)
{
	GnomeSymbolBrowserPriv *priv;

	g_return_if_fail (gsb != NULL);
	g_return_if_fail (GNOME_IS_SYMBOL_BROWSER (gsb));

	priv = gsb->priv;

	gtk_tree_store_clear (GTK_TREE_STORE (priv->tree_model));
	gnome_symbol_browser_set_file(gsb, NULL);
	g_hash_table_foreach_remove (priv->symbol_hash,
				     destroy_symbol_data_cb,
				     NULL);
}

void
gnome_symbol_browser_set_file (GnomeSymbolBrowser *gsb, gchar* filepath)
{
	GtkTreeIter iter;
	
	tm_source_file_free (gsb->priv->tm_file);
	if (filepath)
		gsb->priv->tm_file = tm_source_file_new(filepath, TRUE);
	else 
		gsb->priv->tm_file = NULL;

	gtk_list_store_clear (gsb->priv->combo_store);
	gtk_list_store_append (gsb->priv->combo_store, &iter);
	gtk_list_store_set (gsb->priv->combo_store, &iter,
			    SYMBOL_NAME, "", -1);
	
	/* Update File Symbol list */
	if (filepath) {
		gsb_fill_symbol_combo (gsb, filepath, tm_tag_max_t);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (gsb->priv->symbol_combo), 0);
}

BonoboEventSource *
gnome_symbol_browser_get_event_source (GnomeSymbolBrowser *gsb)
{
	return gsb->priv->event_source;
}
