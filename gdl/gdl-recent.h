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

#ifndef GDL_RECENT_H
#define GDL_RECENT_H

#include <glib-object.h>
#include <bonobo/bonobo-ui-component.h>

G_BEGIN_DECLS

typedef struct _GdlRecent      GdlRecent;
typedef struct _GdlRecentClass GdlRecentClass;
typedef struct _GdlRecentPriv  GdlRecentPriv;

#define GDL_RECENT_TYPE        (gdl_recent_get_type ())
#define GDL_RECENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GDL_RECENT_TYPE, GdlRecent))
#define GDL_RECENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GDL_RECENT_TYPE, GdlRecentClass))
#define GDL_IS_RECENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDL_RECENT_TYPE))
#define GDL_IS_RECENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GDL_RECENT_TYPE))

struct _GdlRecent {
	GObject parent;

	GdlRecentPriv *priv;
};

struct _GdlRecentClass {
	GObjectClass parent_class;

	void (*activate) (GdlRecent *recent, const char *uri);
};

enum {
	GDL_RECENT_LIST_NONE,
	GDL_RECENT_LIST_NUMERIC,
	GDL_RECENT_LIST_ALPHABETIC
};

/* Creation */
GType              gdl_recent_get_type      (void);
GdlRecent         *gdl_recent_new           (const char        *key,
					     const char        *menu_path,
					     int                limit,
					     int                list_type);

void               gdl_recent_set_ui_component (GdlRecent         *recent,
						BonoboUIComponent *uic);

/* Add/remove/get entries. */
void               gdl_recent_add           (GdlRecent  *recent,
					     const char *uri);
void               gdl_recent_delete        (GdlRecent  *recent,
					     const char *uri);
void               gdl_recent_clear         (GdlRecent  *recent);
GSList            *gdl_recent_get_list      (GdlRecent  *recent);

/* Properties. */
int                gdl_recent_get_limit     (GdlRecent  *recent);
void               gdl_recent_set_limit     (GdlRecent  *recent,
					     int         limit);
int                gdl_recent_get_list_type (GdlRecent *recent);
void               gdl_recent_set_list_type (GdlRecent *recent,
					     int        list_type);

G_END_DECLS

#endif /* GDL_RECENT_H */
