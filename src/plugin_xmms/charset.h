/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002  Daisuke Shimamura
 *
 * Almost from charset.h - 2001/12/04
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 1999-2001  H蛆ard Kv虱en <havardk@xmms.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __CHARSET_H__
#define __CHARSET_H__


/***************
 * Declaration *
 ***************/

typedef struct {
	gchar *charset_title;
	gchar *charset_name;
} CharsetInfo;

/* translated charset titles */
extern const CharsetInfo charset_trans_array[];

/**************
 * Prototypes *
 **************/

gchar* get_current_charset (void);

gchar* convert_from_file_to_user (const gchar *string);
gchar* convert_from_user_to_file (const gchar *string);

GList *Charset_Create_List (void);
gchar *Charset_Get_Name_From_Title (gchar *charset_title);
gchar *Charset_Get_Title_From_Name (gchar *charset_name);

gboolean test_conversion_charset (char *from, char *to);


#endif /* __CHARSET_H__ */

