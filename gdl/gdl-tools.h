/*  -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * This file is part of the GNOME Devtool Libraries
 * 
 * Copyright (C) 1999-2000 Dave Camp <dave@helixcode.com>
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

/* Miscellaneous GDL tools/macros */

#ifndef __GDL_TOOLS_H__
#define __GDL_TOOLS_H__

#include <glib.h>
#include <gtk/gtkwidget.h>


/* FIXME: Toggle this */

G_BEGIN_DECLS

#define DO_GDL_TRACE

#ifdef DO_GDL_TRACE

#ifdef __GNUC__

#define GDL_TRACE()  G_STMT_START {             \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
	   "file %s: line %d (%s)",           \
	   __FILE__,                          \
	   __LINE__,                          \
	   __PRETTY_FUNCTION__); } G_STMT_END 

#define GDL_TRACE_EXTRA(format, args...) G_STMT_START {     \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
	   "file %s: line %d (%s): "format,   \
	   __FILE__,                          \
	   __LINE__,                          \
	   __PRETTY_FUNCTION__,               \
	   ##args); } G_STMT_END                   
    
#else /* __GNUC__ */

#define GDL_TRACE()  G_STMT_START {             \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
	   "file %s: line %d",                \
	   __FILE__,                          \
	   __LINE__); } G_STMT_END 

#define GDL_TRACE_EXTRA(format, args...) G_STMT_START {     \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
	   "file %s: line %d: "format,        \
	   __FILE__,                          \
	   __LINE__,                          \
	   ##args); } G_STMT_END                       
#endif /* __GNUC__ */

#else /* DO_GDL_TRACE */

#define GDL_TRACE()
#define GDL_TRACE_EXTRA()

#endif /* DO_GDL_TRACE */

#define GDL_CALL_VIRTUAL(object, get_class_cast, method, args) \
    (get_class_cast (object)->method ? (* get_class_cast (object)->method) args : (void)0)
#define GDL_CALL_VIRTUAL_WITH_DEFAULT(object, get_class_cast, method, args, default) \
    (get_class_cast (object)->method ? (* get_class_cast (object)->method) args : default)

GtkWidget	*gdl_button_new_with_stock_image (const char  *text,
						  const char  *stock_id);

G_END_DECLS

#endif
