/*
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Jeroen Zwartepoorte
 *                    James Willcox <jwillcox@cs.indiana.edu>
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
#include <gconf/gconf-client.h>
#include <libgnome/gnome-macros.h>
#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs.h>
#include <string.h>
#include "gdl-recent.h"

typedef struct _GdlRecentMenuData GdlRecentMenuData;

struct _GdlRecentMenuData {
	GdlRecent		*recent;
	char 			*uri;
};

struct _GdlRecentPriv {
	char			*key;
	int			 limit;
	BonoboUIComponent	*uic;
	char			*menu_path;
	GConfClient		*gconf_client;
	int			 notify_id;
	GHashTable		*monitors;
	int			 list_type;
};

enum {
	PROP_BOGUS,
	PROP_KEY,
	PROP_MENU_PATH,
	PROP_LIMIT,
	PROP_LIST_TYPE
};

enum {
	ACTIVATE,
	LAST_SIGNAL
};

static guint gdl_recent_signals[LAST_SIGNAL] = { 0 };

/* Method definitions. */
static void gdl_recent_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec);
static void gdl_recent_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void gdl_recent_finalize (GObject *object);

static void gdl_recent_clear_menu (GdlRecent *recent);
static void gdl_recent_menu_cb (BonoboUIComponent *uic,
				gpointer           data,
				const char        *cname);
static char *gdl_recent_escape_underlines (const char *text);
static void gdl_menu_data_destroy_cb (gpointer  data,
				      GClosure *closure);
static void gdl_recent_monitor_cb (GnomeVFSMonitorHandle   *handle,
				   const gchar             *monitor_uri,
				   const gchar             *info_uri,
				   GnomeVFSMonitorEventType event_type,
				   gpointer                 data);
static void gdl_recent_monitor_uri (GdlRecent  *recent,
				    const char *uri);
static void gdl_recent_monitor_foreach_free (gpointer key,
					     gpointer value,
					     gpointer user_data);
static void gdl_recent_update_menus (GdlRecent *recent,
				     GSList    *uri_list);
static GSList *gdl_recent_delete_from_list (GdlRecent  *recent,
					    GSList     *list,
					    const char *uri);
static void gdl_recent_g_slist_deep_free (GSList *list);
static GSList *gdl_recent_gconf_to_list (GConfValue* value);
static void gdl_recent_notify_cb (GConfClient *client,
				  guint        cnxn_id,
				  GConfEntry  *entry,
				  gpointer     user_data);

/* Boilerplate code. */
GNOME_CLASS_BOILERPLATE (GdlRecent, gdl_recent,
			 GObject, G_TYPE_OBJECT);

/* Private routines */
static void
gdl_recent_class_init (GdlRecentClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gdl_recent_finalize;
	object_class->get_property = gdl_recent_get_property;
	object_class->set_property = gdl_recent_set_property;

	gdl_recent_signals[ACTIVATE] = g_signal_new ("activate",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GdlRecentClass, activate),
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1,
			G_TYPE_STRING);

	g_object_class_install_property (object_class,
					 PROP_KEY,
					 g_param_spec_string ("key",
						 	      _("GConf key"),
							      _("The gconf key for storing the recent entries."),
							      "",
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MENU_PATH,
					 g_param_spec_string ("ui-path",
						 	      _("Path"),
							      _("The path to put the menu items."),
							      "",
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LIMIT,
					 g_param_spec_int    ("limit",
						 	      _("Limit"),
							      _("The maximum number of items to be allowed in the list."),
							      1,
							      1000,
							      10,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LIST_TYPE,
					 g_param_spec_int    ("list-type",
						 	      _("List type"),
							      _("The type with which the items will be numbered."),
							      0,
							      2,
							      1,
							      G_PARAM_READWRITE));
}

static void
gdl_recent_instance_init (GdlRecent *recent)
{
	recent->priv = g_new0 (GdlRecentPriv, 1);
	recent->priv->gconf_client = gconf_client_get_default ();
	recent->priv->monitors = g_hash_table_new (g_str_hash, g_str_equal);
	recent->priv->key = NULL;
	recent->priv->menu_path = NULL;
}

static void
gdl_recent_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GdlRecent *recent = GDL_RECENT (object);

	switch (prop_id) {
		case PROP_KEY:
			g_value_set_string (value, recent->priv->key);
			break;
		case PROP_MENU_PATH:
			g_value_set_string (value, recent->priv->menu_path);
			break;
		case PROP_LIMIT:
			g_value_set_int (value, recent->priv->limit);
			break;
		case PROP_LIST_TYPE:
			g_value_set_int (value, recent->priv->list_type);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gdl_recent_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GdlRecent *recent = GDL_RECENT (object);

	switch (prop_id) {
		case PROP_KEY:
			if (recent->priv->key) {
				gconf_client_notify_remove (recent->priv->gconf_client,
							    recent->priv->notify_id);
				g_free (recent->priv->key);
			}

			recent->priv->key = g_strdup (g_value_get_string (value));

			/* Register notify callback for gconf changes. */
			gconf_client_add_dir (recent->priv->gconf_client,
					      recent->priv->key,
					      GCONF_CLIENT_PRELOAD_NONE,
					      NULL);

			recent->priv->notify_id = gconf_client_notify_add (recent->priv->gconf_client,
									   recent->priv->key,
									   gdl_recent_notify_cb,
									   recent, NULL, NULL);
			break;
		case PROP_MENU_PATH:
			if (recent->priv->menu_path)
				g_free (recent->priv->menu_path);
			recent->priv->menu_path = g_strdup (g_value_get_string (value));
			break;
		case PROP_LIMIT:
			gdl_recent_set_limit (recent, g_value_get_int (value));
			break;
		case PROP_LIST_TYPE:
			gdl_recent_set_list_type (recent, g_value_get_int (value));
			break;
		default:
			break;
	}
}

static void
gdl_recent_finalize (GObject *object)
{
	GdlRecent *recent = GDL_RECENT (object);

	gconf_client_notify_remove (recent->priv->gconf_client,
				    recent->priv->notify_id);

	g_free (recent->priv->key);
	g_free (recent->priv->menu_path);
	g_object_unref (recent->priv->gconf_client);
	g_hash_table_foreach (recent->priv->monitors,
			      gdl_recent_monitor_foreach_free,
			      NULL);
	g_hash_table_destroy (recent->priv->monitors);
}

static void
gdl_recent_clear_menu (GdlRecent *recent)
{
	int i = 1;
	gboolean done = FALSE;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));

	while (!done) {
		char *verb_name = g_strdup_printf ("%s%d", "uri-", i);
		char *item_path = g_strconcat (recent->priv->menu_path, "/", verb_name, NULL);
		if (bonobo_ui_component_path_exists (recent->priv->uic,
						     item_path, NULL))
			bonobo_ui_component_rm (recent->priv->uic,
						item_path, NULL);
		else
			done = TRUE;

		g_free (item_path);
		g_free (verb_name);

		i++;
	}
}

static void
gdl_recent_menu_cb (BonoboUIComponent *uic,
		    gpointer           data,
		    const char        *cname)
{
	GdlRecentMenuData *md = data;

	g_return_if_fail (md != NULL);
	g_return_if_fail (md->uri != NULL);
	g_return_if_fail (md->recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (md->recent));

	g_signal_emit (G_OBJECT (md->recent),
		       gdl_recent_signals[ACTIVATE],
		       0,
		       md->uri);
}

static char *
gdl_recent_escape_underlines (const char *text)
{
	GString *str;
	gint length;
	const gchar *p;
	const gchar *end;

	g_return_val_if_fail (text != NULL, NULL);

	length = strlen (text);

	str = g_string_new ("");

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);

		switch (*p) {
			case '_':
				g_string_append (str, "__");
				break;
			default:
				g_string_append_len (str, p, next - p);
				break;
		}

		p = next;
	}

	return g_string_free (str, FALSE);
}

static void
gdl_menu_data_destroy_cb (gpointer data, GClosure *closure)
{
	GdlRecentMenuData *md = data;

	g_free (md->uri);
	g_free (md);
}

static void
gdl_recent_update_menus (GdlRecent *recent,
			 GSList    *uri_list)
{
	char *verb_name, *cmd;
	char *full_uri, *uri, *escaped_name;
	char *item_path, *tip;
	GdlRecentMenuData *md;
	BonoboUIComponent *uic;
	int i;
	GClosure *closure;
	GnomeVFSURI *vfs_uri;
	GSList *l;
	char *label = NULL;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));

	uic = recent->priv->uic;

	gdl_recent_clear_menu (recent);

	bonobo_ui_component_freeze (uic, NULL);

	for (l = uri_list; l != NULL; l = l->next) {
		full_uri = l->data;

		/* Verify that the uri exists. */
		vfs_uri = gnome_vfs_uri_new (full_uri);
		if (!gnome_vfs_uri_exists (vfs_uri))
			gdl_recent_delete (recent, full_uri);
		gnome_vfs_uri_unref (vfs_uri);
	}

	for (l = uri_list, i = 1; l != NULL; l = l->next, i++) {
		full_uri = l->data;
		uri = g_path_get_basename (full_uri);

		/* This is what gets passed to our private "activate" callback. */
		md = (GdlRecentMenuData *)g_malloc (sizeof (GdlRecentMenuData));
		md->recent = recent;
		md->uri = g_strdup (g_slist_nth_data (uri_list, i - 1));

		/* Create verb & command for menuitem. */
		verb_name = g_strdup_printf ("%s%d", "uri-", i);
		cmd = g_strdup_printf ("<cmd name = \"%s\" /> ", verb_name);
		bonobo_ui_component_set_translate (uic, "/commands/", cmd, NULL);

		closure = g_cclosure_new (G_CALLBACK (gdl_recent_menu_cb),
					  md, gdl_menu_data_destroy_cb);

		bonobo_ui_component_add_verb_full (uic, verb_name, closure); 

		escaped_name = gdl_recent_escape_underlines (uri);

		tip =  g_strdup_printf (_("Open %s"), full_uri);

		switch (recent->priv->list_type) {
			case GDL_RECENT_LIST_NONE:
				label = g_strdup (escaped_name);
				break;
			case GDL_RECENT_LIST_NUMERIC:
				if (i < 10)
					label = g_strdup_printf ("_%d. %s", i, escaped_name);
				else
					label = g_strdup_printf ("%d. %s", i, escaped_name);
				break;
			case GDL_RECENT_LIST_ALPHABETIC:
				label = g_strdup_printf ("_%c. %s", i + (int)'A' - 1, escaped_name);
				break;
		}

		item_path = g_strconcat (recent->priv->menu_path, "/", verb_name, NULL);

		if (bonobo_ui_component_path_exists (uic, item_path, NULL)) {
			bonobo_ui_component_set_prop (uic, item_path, "label", label, NULL);
			bonobo_ui_component_set_prop (uic, item_path, "tip", tip, NULL);
		} else {
			char *xml;

			xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"%s\""
						" _label=\"%s\"  _tip=\"%s \" hidden=\"0\" />", 
						verb_name, verb_name, label, tip);

			bonobo_ui_component_set_translate (uic,
							   recent->priv->menu_path,
							   xml,
							   NULL);

			g_free (xml); 
		}

		gdl_recent_monitor_uri (recent, md->uri);

		g_free (verb_name);
		g_free (cmd);
		g_free (uri);
		g_free (escaped_name);
		g_free (item_path);
		g_free (label);
		g_free (tip);
	}

	bonobo_ui_component_thaw (uic, NULL);
}

static GSList *
gdl_recent_delete_from_list (GdlRecent  *recent,
			     GSList     *list,
			     const char *uri)
{
	GSList *l, *ret_list;

	/* Return list is maintained separately so that list
	 with deleted node could be returned */
	ret_list = list;
	for (l = list; l != NULL; l = l->next) {
		char *text = l->data;

		if (!strcmp (text, uri)) {
			ret_list = g_slist_delete_link (ret_list, l);
			g_free (text);
			
			/* Start the loop all over again to remove any more
			  duplicates */
			l = ret_list;
			if (!l)
			    break;
		}
	}

	return ret_list;
}

static void
gdl_recent_g_slist_deep_free (GSList *list)
{
	GSList *lst;

	if (list == NULL)
		return;

	lst = list;
	while (lst) {
		g_free (lst->data);
		lst->data = NULL;
		lst = lst->next;
	}

	g_slist_free (list);
}

static GSList *
gdl_recent_gconf_to_list (GConfValue* value)
{
	GSList* iter;
	GSList *list = NULL;

	g_return_val_if_fail (value, NULL);

	iter = gconf_value_get_list (value);

	while (iter != NULL) {
		GConfValue* element = iter->data;
		char *text = g_strdup (gconf_value_get_string (element));

		list = g_slist_prepend (list, text);

		iter = g_slist_next (iter);
	}

	list = g_slist_reverse (list);

	return list;
}

static void
gdl_recent_notify_cb (GConfClient *client,
		      guint        cnxn_id,
		      GConfEntry  *entry,
		      gpointer     user_data)
{
	GSList *list = NULL;
	GdlRecent *recent = GDL_RECENT (user_data);

	/* Bail out if we don't have a menu. */
	if (!recent->priv->uic)
		return;

	/* This means the key was unset (cleared). */
	if (entry->value == NULL) {
		gdl_recent_clear_menu (recent);
		return;
	}

	list = gdl_recent_gconf_to_list (entry->value);

	gdl_recent_update_menus (recent, list);

	gdl_recent_g_slist_deep_free (list);
}

static void
gdl_recent_monitor_cb (GnomeVFSMonitorHandle   *handle,
		       const gchar             *monitor_uri,
		       const gchar             *info_uri,
		       GnomeVFSMonitorEventType event_type,
		       gpointer                 data)
{
	GdlRecent *recent;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GDL_IS_RECENT (data));

	recent = GDL_RECENT (data);

	/* If a file was deleted, we just remove it from our list. */
	switch (event_type) {
		case GNOME_VFS_MONITOR_EVENT_DELETED:
			gdl_recent_delete (recent, monitor_uri);
			break;
		default:
		break;
	}
}

static void
gdl_recent_monitor_uri (GdlRecent  *recent,
			const char *uri)
{
	GnomeVFSMonitorHandle *handle = NULL;
	GnomeVFSResult result;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));
	g_return_if_fail (uri != NULL);

	handle = g_hash_table_lookup (recent->priv->monitors, uri);
	if (handle == NULL) {
		/* This is a new uri, so we need to monitor it. */
		result = gnome_vfs_monitor_add (&handle,
						uri,
						GNOME_VFS_MONITOR_FILE,
						gdl_recent_monitor_cb,
						recent);

		if (result == GNOME_VFS_OK) {
			g_hash_table_insert (recent->priv->monitors,
					     g_strdup (uri),
					     handle);
		}
	}
}

static void
gdl_recent_monitor_foreach_free (gpointer key,
				 gpointer value,
				 gpointer user_data)
{
	char *uri = key;
	GnomeVFSMonitorHandle *handle = value;

	g_free (uri);
	gnome_vfs_monitor_cancel (handle);
}

/* ----------------------------------------------------------------------
 * Public interface 
 * ---------------------------------------------------------------------- */

GdlRecent *
gdl_recent_new (const char        *key,
		const char        *menu_path,
		int                limit,
		int                list_type)
{
	GdlRecent *recent;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (menu_path != NULL, NULL);

	recent = GDL_RECENT (g_object_new (GDL_RECENT_TYPE,
					   "key", key,
					   "ui-path", menu_path,
					   "limit", limit,
					   "list-type", list_type,
					   NULL));

	return recent;
}

void
gdl_recent_set_ui_component (GdlRecent         *recent,
			     BonoboUIComponent *uic)
{
	GSList *uri_list;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));
	g_return_if_fail (uic != NULL);

	recent->priv->uic = uic;

	uri_list = gconf_client_get_list (recent->priv->gconf_client,
					  recent->priv->key,
					  GCONF_VALUE_STRING, NULL);

	gdl_recent_update_menus (recent, uri_list);

	gdl_recent_g_slist_deep_free (uri_list);
}

void
gdl_recent_add (GdlRecent  *recent,
		const char *uri)
{
	GSList *uri_list;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));
	g_return_if_fail (uri != NULL);

	uri_list = gconf_client_get_list (recent->priv->gconf_client,
					  recent->priv->key,
					  GCONF_VALUE_STRING, NULL);

	/* If this is already in our list, remove it. */
	uri_list = gdl_recent_delete_from_list (recent, uri_list, uri);

	/* Prepend the new one. */
	uri_list = g_slist_prepend (uri_list, g_strdup (uri));

	/* If we're over the limit, delete from the end. */
	while (g_slist_length (uri_list) > recent->priv->limit) {
		char *tmp_uri;
		tmp_uri = g_slist_nth_data (uri_list, g_slist_length (uri_list) - 1);
		uri_list = g_slist_remove (uri_list, tmp_uri);
		g_free (tmp_uri);
	}

	gconf_client_set_list (recent->priv->gconf_client,
			       recent->priv->key,
			       GCONF_VALUE_STRING,
			       uri_list, NULL);

	gconf_client_suggest_sync (recent->priv->gconf_client, NULL);

	gdl_recent_g_slist_deep_free (uri_list);
}

void
gdl_recent_delete (GdlRecent  *recent,
		   const char *uri)
{
	GSList *uri_list, *new_uri_list;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));
	g_return_if_fail (uri != NULL);

	uri_list = gconf_client_get_list (recent->priv->gconf_client,
					  recent->priv->key,
					  GCONF_VALUE_STRING, NULL);

	new_uri_list = gdl_recent_delete_from_list (recent, uri_list, uri);

	/* Delete it from gconf. */
	gconf_client_set_list (recent->priv->gconf_client,
			       recent->priv->key,
			       GCONF_VALUE_STRING,
			       new_uri_list,
			       NULL);
	gconf_client_suggest_sync (recent->priv->gconf_client, NULL);

	gdl_recent_g_slist_deep_free (new_uri_list);
}

void
gdl_recent_clear (GdlRecent *recent)
{
	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));

	gconf_client_unset (recent->priv->gconf_client,
			    recent->priv->key, NULL);
}

GSList *
gdl_recent_get_list (GdlRecent *recent)
{
	GSList *uri_list;

	g_return_val_if_fail (recent != NULL, FALSE);
	g_return_val_if_fail (GDL_IS_RECENT (recent), FALSE);

	uri_list = gconf_client_get_list (recent->priv->gconf_client,
					  recent->priv->key,
					  GCONF_VALUE_STRING, NULL);

	return uri_list;
}

int
gdl_recent_get_limit (GdlRecent *recent)
{
	g_return_val_if_fail (recent != NULL, -1);
	g_return_val_if_fail (GDL_IS_RECENT (recent), -1);

	return recent->priv->limit;
}

void
gdl_recent_set_limit (GdlRecent *recent,
		      int        limit)
{
	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));

	recent->priv->limit = limit;
}

int
gdl_recent_get_list_type (GdlRecent *recent)
{
	g_return_val_if_fail (recent != NULL, 0);
	g_return_val_if_fail (GDL_IS_RECENT (recent), 0);

	return recent->priv->list_type;
}

void
gdl_recent_set_list_type (GdlRecent *recent,
			  int        list_type)
{
	GSList *uri_list;

	g_return_if_fail (recent != NULL);
	g_return_if_fail (GDL_IS_RECENT (recent));

	recent->priv->list_type = list_type;

	if (recent->priv->uic) {
		uri_list = gconf_client_get_list (recent->priv->gconf_client,
						  recent->priv->key,
						  GCONF_VALUE_STRING, NULL);

		gdl_recent_update_menus (recent, uri_list);

		gdl_recent_g_slist_deep_free (uri_list);
	}
}
