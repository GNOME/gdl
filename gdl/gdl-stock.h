/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- 
 * gdl-stock.h
 * 
 * Copyright (C) 2003 Jeroen Zwartepoorte
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GDL_STOCK_H__
#define __GDL_STOCK_H__

G_BEGIN_DECLS

#define GDL_STOCK_CLOSE			"gdl-close"
#define GDL_STOCK_MENU_LEFT		"gdl-menu-left"
#define GDL_STOCK_MENU_RIGHT		"gdl-menu-right"

void gdl_stock_init (void);

G_END_DECLS

#endif /* __GDL_STOCK_H__ */
