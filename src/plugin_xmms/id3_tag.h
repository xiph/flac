/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002  Daisuke Shimamura
 *
 *  Based on id3_tag.h - 2001/02/16
 *           et_core.h - 2001/10/21
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2001-2002  Jerome Couderc <j.couderc@ifrance.com>
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

#ifndef __ID3TAG_H__
#define __ID3TAG_H__

#include <glib.h>

/***************
 * Declaration *
 ***************/

#ifndef MAX
#    define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#    define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


/*
 * Description of each item of the TagList list
 */
typedef struct _File_Tag File_Tag;
struct _File_Tag
{
    gulong key;            /* Incremented value */
    gboolean saved;        /* Set to TRUE if this tag had been saved */
    gchar *title;          /* Title of track */
    gchar *artist;         /* Artist name */
    gchar *album;          /* Album name */
    gchar *year;           /* Year of track */
    gchar *track;          /* Position of track in the album */
    gchar *track_total;    /* The number of tracks for the album (ex: 12/20) */
    gchar *genre;          /* Genre of song */
    gchar *comment;        /* Comment */
};

/**************
 * Prototypes *
 **************/
void Initialize_File_Tag_Item (File_Tag *FileTag);
gboolean Free_File_Tag_Item (File_Tag *FileTag);
gboolean Id3tag_Read_File_Tag  (gchar *filename, File_Tag *FileTag);

#endif /* __ID3TAG_H__ */
