/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gdl-icons.c
 *
 * Copyright (C) 2000-2001 Dave Camp
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
 *
 * Authors: Dave Camp, Jeroen Zwartepoorte
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gdl-i18n.h"
#include <string.h>
#include <libgnome/gnome-macros.h>
#include <libgnomeui/gnome-icon-lookup.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include "gdl-icons.h"

enum {
	PROP_BOGUS,
	PROP_ICON_SIZE,
	PROP_ICON_ZOOM
};

struct _GdlIconsPrivate {
	int icon_size;
	double icon_zoom;

	GnomeIconTheme *icon_theme;
	GHashTable *icons;
};

GNOME_CLASS_BOILERPLATE (GdlIcons, gdl_icons, GObject, G_TYPE_OBJECT);

static void
gdl_icons_get_property (GObject *object,
			guint prop_id,
			GValue *value,
			GParamSpec *pspec)
{
	GdlIcons *icons = GDL_ICONS (object);

	switch (prop_id) {
		case PROP_ICON_SIZE:
			g_value_set_int (value, icons->priv->icon_size);
			break;
		case PROP_ICON_ZOOM:
			g_value_set_double (value, icons->priv->icon_zoom);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gdl_icons_set_property (GObject *object,
			guint prop_id,
			const GValue *value,
			GParamSpec *pspec)
{
	GdlIcons *icons = GDL_ICONS (object);

	switch (prop_id) {
		case PROP_ICON_SIZE:
			icons->priv->icon_size = g_value_get_int (value);
			g_hash_table_destroy (icons->priv->icons);
			icons->priv->icons = g_hash_table_new_full (g_str_hash, g_str_equal,
								    (GDestroyNotify)g_free,
								    (GDestroyNotify)gdk_pixbuf_unref);
			break;
		case PROP_ICON_ZOOM:
			icons->priv->icon_zoom = g_value_get_double (value);
			g_hash_table_destroy (icons->priv->icons);
			icons->priv->icons = g_hash_table_new_full (g_str_hash, g_str_equal,
								    (GDestroyNotify)g_free,
								    (GDestroyNotify)gdk_pixbuf_unref);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
theme_changed_cb (GnomeIconTheme *theme,
		  gpointer user_data)
{
	GdlIcons *icons = GDL_ICONS (user_data);

	g_hash_table_destroy (icons->priv->icons);
	icons->priv->icons = g_hash_table_new_full (g_str_hash, g_str_equal,
						    (GDestroyNotify)g_free,
						    (GDestroyNotify)gdk_pixbuf_unref);
}

static void
gdl_icons_dispose (GObject *object)
{
	GdlIcons *icons = GDL_ICONS (object);
	GdlIconsPrivate *priv = icons->priv;

	if (priv) {
		g_object_unref (priv->icon_theme);
		g_hash_table_destroy (priv->icons);

		g_free (priv);
		icons->priv = NULL;
	}
}

static void
gdl_icons_class_init (GdlIconsClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = gdl_icons_dispose;
	object_class->get_property = gdl_icons_get_property;
	object_class->set_property = gdl_icons_set_property;

	g_object_class_install_property (object_class, PROP_ICON_SIZE,
					 g_param_spec_int ("icon-size", 
							   _("Icon size"),
							   _("Icon size"),
							   12, 192, 24,
							   G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_ICON_ZOOM,
					 g_param_spec_double ("icon-zoom", 
							      _("Icon zoom"),
							      _("Icon zoom"),
							      1.0, 256.0, 24.0,
							      G_PARAM_READWRITE));
}

static void
gdl_icons_instance_init (GdlIcons *icons)
{
	GdlIconsPrivate *priv;

	priv = g_new0 (GdlIconsPrivate, 1);
	icons->priv = priv;

	priv->icon_theme = gnome_icon_theme_new ();
	g_signal_connect (G_OBJECT (priv->icon_theme), "changed",
			  G_CALLBACK (theme_changed_cb), icons);
	priv->icons = g_hash_table_new_full (g_str_hash, g_str_equal,
					     (GDestroyNotify)g_free,
					     (GDestroyNotify)gdk_pixbuf_unref);
}

GdlIcons *
gdl_icons_new (int icon_size,
	       double icon_zoom)
{
	return GDL_ICONS (g_object_new (GDL_TYPE_ICONS,
					"icon-size", icon_size,
					"icon-zoom", icon_zoom,
					NULL));
}

GdkPixbuf *
gdl_icons_get_folder_icon (GdlIcons *icons)
{
	g_return_val_if_fail (icons != NULL, NULL);
	g_return_val_if_fail (GDL_IS_ICONS (icons), NULL);

	return gdl_icons_get_mime_icon (icons, "application/directory-normal");
}

GdkPixbuf *
gdl_icons_get_uri_icon (GdlIcons *icons,
			const char *uri)
{
	GnomeVFSFileInfo *info;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (icons != NULL, NULL);
	g_return_val_if_fail (GDL_IS_ICONS (icons), NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	info = gnome_vfs_file_info_new ();
	gnome_vfs_get_file_info (uri, info,
				 GNOME_VFS_FILE_INFO_FOLLOW_LINKS |
				 GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
				 GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE);
	if (info->mime_type)
		pixbuf = gdl_icons_get_mime_icon (icons, info->mime_type);
	else
		pixbuf = gdl_icons_get_mime_icon (icons, "gnome-fs-regular");
	gnome_vfs_file_info_unref (info);

	return pixbuf;
}

GdkPixbuf *
gdl_icons_get_mime_icon (GdlIcons *icons,
			 const char *mime_type)
{
	GdkPixbuf *pixbuf;
	char *icon_name;
	char *icon_path;

	g_return_val_if_fail (icons != NULL, NULL);
	g_return_val_if_fail (GDL_IS_ICONS (icons), NULL);
	g_return_val_if_fail (mime_type != NULL, NULL);

	pixbuf = g_hash_table_lookup (icons->priv->icons, mime_type);
	if (pixbuf != NULL) {
		g_object_ref (G_OBJECT (pixbuf));
		return pixbuf;
	}

	if (!strcmp (mime_type, "application/directory-normal")) {
		icon_name = g_strdup ("gnome-fs-directory");
	} else {
		icon_name = gnome_icon_lookup (icons->priv->icon_theme,
					       NULL,
					       NULL,
					       NULL,
					       NULL,
					       mime_type,
					       GNOME_ICON_LOOKUP_FLAGS_NONE,
					       NULL);
	}

	if (!icon_name) {
		/* Return regular icon if one doesn't exist for mime type. */
		if (!strcmp (mime_type, "gnome-fs-regular"))
			return NULL;
		else
			return gdl_icons_get_mime_icon (icons, "gnome-fs-regular");
	} else {
		icon_path = gnome_icon_theme_lookup_icon (icons->priv->icon_theme,
							  icon_name,
							  icons->priv->icon_size,
							  NULL,
							  NULL);

		if (!icon_path) {
			g_free (icon_name);
			if (!strcmp (mime_type, "gnome-fs-regular"))
				return NULL;
			else
				return gdl_icons_get_mime_icon (icons, "gnome-fs-regular");
		} else {
			pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
			if (pixbuf != NULL) {
				GdkPixbuf *scaled;
				int new_w, new_h;
				int w, h;
				double factor;

				w = gdk_pixbuf_get_width (pixbuf);
				h = gdk_pixbuf_get_height (pixbuf);

				factor = MIN (icons->priv->icon_zoom / w,
					      icons->priv->icon_zoom / h);
				new_w  = MAX ((int) (factor * w), 1);
				new_h  = MAX ((int) (factor * h), 1);

				scaled = gdk_pixbuf_scale_simple (pixbuf,
								  new_w,
								  new_h,
								  GDK_INTERP_BILINEAR);
				g_object_unref (pixbuf);
				pixbuf = scaled;
			} else {
				g_free (icon_name);
				g_free (icon_path);
				if (!strcmp (mime_type, "gnome-fs-regular"))
					return NULL;
				else
					return gdl_icons_get_mime_icon (icons,
									"gnome-fs-regular");
			}
		}
	}

	g_hash_table_insert (icons->priv->icons, g_strdup (mime_type), pixbuf);
	g_object_ref (pixbuf);

	g_free (icon_path);
	g_free (icon_name);

	return pixbuf;
}
