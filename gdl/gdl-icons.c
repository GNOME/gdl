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
 */

#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <librsvg/rsvg.h>

#include <string.h>

#include "program.xpm"
#include "shared.xpm"
#include "static.xpm"

#include <math.h>

#include "default-icon.h"

#include <gconf/gconf-client.h>

#include "gdl-icons.h"

#define ICON_SIZE 15
#define LOAD_SIZE 24

static const char *icon_file_name_suffixes[] =
{
	".svg",
        ".png",
        ".jpg",
};

static char *theme_dir = NULL;
GHashTable *pixbufs_by_name = NULL;
GConfClient *gconf_client = NULL;

static GdkPixbuf *
scale_icon (GdkPixbuf *unscaled)
{
        double ratio;
        GdkPixbuf *pixbuf;
        if (gdk_pixbuf_get_height (unscaled) == 0) {
                g_warning ("invalid pixmap: height of 0.\n");
                return NULL;
        }
        ratio = (double)gdk_pixbuf_get_width (unscaled) / (double) gdk_pixbuf_get_height (unscaled);
        pixbuf = gdk_pixbuf_scale_simple (unscaled, 
                                          floor ((15 * ratio) + 0.5), 15,
                                          GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (unscaled));
        return pixbuf;
        
}

static char *
make_icon_name_from_mime_type (const char *mime_type)
{
	char *icon_name = g_strdup_printf ("gnome-%s", mime_type);
	char *p;
	
	for (p = icon_name; *p != '\0'; p++) {
		if (*p == '/') *p = '-';
	}
	
	return icon_name;
}

static char *
remove_icon_file_name_suffix (const char *icon_name)
{
        int i;
        const char *suffix;
	int len;

	if (!icon_name) {
		return NULL;
	}
	
	len = strlen (icon_name);

        for (i = 0; i < G_N_ELEMENTS (icon_file_name_suffixes); i++) {
                suffix = icon_file_name_suffixes[i];
                if (!strcmp (icon_name + (len - strlen (suffix)), 
			     suffix)) {
                        return g_strndup (icon_name, len - strlen (suffix));
                }
        }
        return g_strdup (icon_name);
}

static char *
get_mime_type_icon_without_suffix (const char *mime_type)
{
	return remove_icon_file_name_suffix (gnome_vfs_mime_get_icon (mime_type));
}

static char *
get_icon_name_for_file (GnomeVFSFileInfo *info)
{
	char *name;
	switch (info->type) {
        case GNOME_VFS_FILE_TYPE_DIRECTORY:
		return g_strdup ("i-directory");
        case GNOME_VFS_FILE_TYPE_FIFO:
                return g_strdup ("i-fifo");
        case GNOME_VFS_FILE_TYPE_SOCKET:
		return g_strdup ("i-sock");
        case GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE:
		return g_strdup ("i-chardev");
        case GNOME_VFS_FILE_TYPE_BLOCK_DEVICE:
		return g_strdup ("i-blockdev");
        case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
        	/* Non-broken symbolic links return the target's type. */
		return g_strdup ("i-symlink");
        case GNOME_VFS_FILE_TYPE_REGULAR:
	case GNOME_VFS_FILE_TYPE_UNKNOWN:
	default:
		name = get_mime_type_icon_without_suffix (info->mime_type);
		if (!name) {
			name = make_icon_name_from_mime_type (info->mime_type);
		}
		if (!name) {
			name = g_strdup ("i-regular");
		}
		return name;
        }
}

static char *
get_theme_dir (const char *theme_name)
{
	char *path;

	path = g_strdup_printf (NAUTILUS_THEMEDIR "/%s", 
				theme_name);

	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		g_free (path);
		path = g_strdup_printf ("%s/.nautilus/themes/%s", 
					g_get_home_dir (), theme_name);
	}
	
	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		g_free (path);
		path = NULL;
	}
	
	return path;
}

static void
initialize_special ()
{
	GdkPixbuf *pixbuf;
	
	pixbuf = scale_icon (gdk_pixbuf_new_from_xpm_data ((const char**)program_xpm));
	g_hash_table_insert (pixbufs_by_name, g_strdup ("program"), pixbuf);
	pixbuf = scale_icon (gdk_pixbuf_new_from_xpm_data ((const char**)shared_xpm));
	g_hash_table_insert (pixbufs_by_name, g_strdup ("shared_lib"), pixbuf);
	pixbuf = scale_icon (gdk_pixbuf_new_from_xpm_data ((const char**)static_xpm));
	g_hash_table_insert (pixbufs_by_name, g_strdup ("static_lib"), pixbuf);
}

static void
reload_theme (const char *theme_name) 
{
	if (theme_dir) {
		g_free (theme_dir);
		theme_dir = NULL;
	}
	
	if (pixbufs_by_name) {
		g_hash_table_destroy (pixbufs_by_name);
	}
	
	pixbufs_by_name = g_hash_table_new_full (g_str_hash,
						 g_str_equal,
						 (GDestroyNotify)g_free, 
						 (GDestroyNotify)g_object_unref);
	
	if (gconf_client == NULL) {
		gconf_client = gconf_client_get_default ();
	}

	if (theme_name == NULL) {
		char *name = gconf_client_get_string (gconf_client,
						      "/desktop/gnome/file_views/icon_theme",
						      NULL);
		if (name) {
			theme_dir = get_theme_dir (name);
			g_free (name);
		} else {
			theme_dir = get_theme_dir ("default");
		}
	} else {
		theme_dir = get_theme_dir (theme_name);
	}

	initialize_special ();

}	

static char *
get_themed_icon_file_path (const char *theme_dir,
			   const char *icon_name, 
			   unsigned size)
{
	char *themed_icon_name, *path = NULL;
	int i;

	if (icon_name[0] == '/') {
		themed_icon_name = g_strdup (icon_name);
	} else {
		themed_icon_name = g_strconcat (theme_dir, "/", 
						icon_name, NULL);
	}

	for (i = 0; i < G_N_ELEMENTS (icon_file_name_suffixes); i++) {
		if (strcmp (icon_file_name_suffixes[i], ".svg") != 0
		    && size != -1) {
			path = g_strdup_printf ("%s-%u%s", 
						themed_icon_name,
						size, 
						icon_file_name_suffixes[i]);
		} else {
			path = g_strdup_printf ("%s%s",
						themed_icon_name, 
						icon_file_name_suffixes[i]);
		}

		if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			break;
		}

		g_free (path);
		path = NULL;
	}
	g_free (themed_icon_name);
	
	return path;
}


static char *
find_themed_icon_filename (const char *theme_dir, const char *name)
{
	char *path;
	char *name_with_modifier = g_strdup_printf ("%s-aa", name);

	path = get_themed_icon_file_path (theme_dir, name_with_modifier, 
					  LOAD_SIZE);
	if (path != NULL) {
		g_free (name_with_modifier);
		return path;
	}

	path = get_themed_icon_file_path (theme_dir, name, LOAD_SIZE);
	if (path != NULL) {
		g_free (name_with_modifier);
		return path;
	}

	path = get_themed_icon_file_path (theme_dir, name_with_modifier, -1);
	if (path != NULL) {
		g_free (name_with_modifier);
		return path;
	}

	path = get_themed_icon_file_path (theme_dir, name, -1);
	if (path != NULL) {
		g_free (name_with_modifier);
		return path;
	}

	g_free (name_with_modifier);
	return NULL;
}

static char *
get_icon_filename (const char *icon_name)
{
	char *path = find_themed_icon_filename (theme_dir, icon_name);
	if (!path) {
		path = find_themed_icon_filename (GNOME_VFS_ICONDIR, 
						  icon_name);
	}
	if (!path) {
		path = find_themed_icon_filename (theme_dir, "i-regular");
	}
	if (!path) {
		path = find_themed_icon_filename (GNOME_VFS_ICONDIR, 
						  "i-regular");
	}

	return path;
}

static gboolean
path_is_svg (const char *path) 
{
        char *uri;
        GnomeVFSFileInfo *file_info;
        gboolean is_svg;

        /* Sync. file I/O is OK here because this is used only for installed
         * icons, not for the general case which could include icons on devices
         * other than the local hard disk.
         */

        uri = gnome_vfs_get_uri_from_local_path (path);
        file_info = gnome_vfs_file_info_new ();
        gnome_vfs_get_file_info (uri, file_info,
                                 GNOME_VFS_FILE_INFO_GET_MIME_TYPE
                                 | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
        g_free (uri);
        is_svg = strcmp (file_info->mime_type, "image/svg") == 0;
        gnome_vfs_file_info_unref (file_info);

        return is_svg;
}

#define NAUTILUS_ICON_SIZE_STANDARD     48

static GdkPixbuf *
pixbuf_for_name (const char *name)
{
	GdkPixbuf *pixbuf = NULL;
	char *path;

	if (!theme_dir) {
		reload_theme (NULL);
	}

	pixbuf = g_hash_table_lookup (pixbufs_by_name, name);
	if (pixbuf) {
		return g_object_ref (pixbuf);
	}
	
	path = get_icon_filename (name);
	
	if (path) {
		if (path_is_svg (path)) {
			double zoom = (double)ICON_SIZE / NAUTILUS_ICON_SIZE_STANDARD;
			pixbuf = scale_icon (rsvg_pixbuf_from_file_at_zoom (path, zoom, zoom, NULL));
		} else {
			pixbuf = scale_icon (gdk_pixbuf_new_from_file (path, NULL));
		}
		g_free (path);
	}
	
	if (!pixbuf) {
		pixbuf = gdk_pixbuf_new_from_data (default_icon,
						   GDK_COLORSPACE_RGB,
						   TRUE,
						   8,
						   default_icon_width,
						   default_icon_height,
						   default_icon_width * 4, /* stride */
						   NULL, /* don't destroy data */
						   NULL);
		pixbuf = scale_icon (pixbuf);
	}
	g_hash_table_insert (pixbufs_by_name, g_strdup (name), pixbuf);
	
	g_object_ref (pixbuf);

	return pixbuf;
}

GdkPixbuf *
gdl_icon_for_mime (const char *mime_type)
{
	char *name;
	GdkPixbuf *ret;
	name = get_mime_type_icon_without_suffix (mime_type);
	if (!name) {
		name = make_icon_name_from_mime_type (mime_type);
	}
	if (!name) {
		name = g_strdup ("i-regular");
	}
	ret = pixbuf_for_name (name);
	g_free (name);

	return ret;
}

GdkPixbuf *
gdl_icon_for_uri (const char *uri)
{
	GnomeVFSFileInfo *info;
	char *name;
	GdkPixbuf *pixbuf = NULL;
	
	info = gnome_vfs_file_info_new ();

	gnome_vfs_get_file_info (uri, info, 
				 GNOME_VFS_FILE_INFO_FOLLOW_LINKS | GNOME_VFS_FILE_INFO_GET_MIME_TYPE | GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE);

	name = get_icon_name_for_file (info);
	
	pixbuf = pixbuf_for_name (name);
	g_free (name);

	gnome_vfs_file_info_unref (info);
	
	return pixbuf;
}

GdkPixbuf *
gdl_icon_for_folder (void)
{
	return pixbuf_for_name ("i-directory");
}

GdkPixbuf *
gdl_icon_for_special (const char *name)
{
	return pixbuf_for_name (name);
}


