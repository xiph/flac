/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002  Daisuke Shimamura
 *
 * Almost from id3_tag.c - 2001/02/16
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2001-2002  Jerome Couderc <j.couderc@ifrance.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <id3.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "configure.h"
#include "genres.h"
#include "charset.h"
#include "mylocale.h"
#include "id3_tag.h"

/****************
 * Declarations *
 ****************/
#define ID3V2_MAX_STRING_LEN 4096
#define NUMBER_TRACK_FORMATED 1

/**************
 * Prototypes *
 **************/
static size_t ID3Tag_Link_1       (ID3Tag *id3tag, const char *filename);
static size_t ID3Field_GetASCII_1 (const ID3Field *field, char *buffer, size_t maxChars, index_t itemNum);

static gchar  *Id3tag_Genre_To_String (unsigned char genre_code);
static void Strip_String (gchar *string);

/*************
 * Functions *
 *************/
/*
 * Read id3v1.x / id3v2 tag and load data into the File_Tag structure using id3lib functions.
 * Returns TRUE on success, else FALSE.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 */
gboolean Id3tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE *file;
    ID3Tag *id3_tag = NULL;    /* Tag defined by the id3lib */
    gchar *string, *string1, *string2;
    gboolean USE_CHARACTER_SET_TRANSLATION;

    USE_CHARACTER_SET_TRANSLATION = flac_cfg.convert_char_set;

    if (!filename || !FileTag)
        return FALSE;

    if ( (file=fopen(filename,"r"))==NULL )
    {
        g_print(_("ERROR while opening file: '%s' (%s).\n\a"),filename,g_strerror(errno));
        return FALSE;
    }
    fclose(file); // We close it cause id3lib opens/closes file itself


    /* Get data from tag */
    if ( (id3_tag = ID3Tag_New()) )
    {
        ID3Frame *id3_frame;
        ID3Field *id3_field;
        luint frm_size;
        luint num_chars;
        guint field_num = 0; // First field

        /* Link the file to the tag */
        frm_size = ID3Tag_Link_1(id3_tag,filename);

        string = g_malloc(ID3V2_MAX_STRING_LEN+1);

        /*********
         * Title *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TITLE)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                // Note: if 'num_chars' is equal to 0, then the field is empty or corrupted!
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        Strip_String(string1);
                        FileTag->title = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
                        Strip_String(string);
                        FileTag->title = g_strdup(string);
                    }
                }
            }
        }


        /**********
         * Artist *
         **********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_LEADARTIST)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        Strip_String(string1);
                        FileTag->artist = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
                        Strip_String(string);
                        FileTag->artist = g_strdup(string);
                    }
                }
            }
        }


        /*********
         * Album *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_ALBUM)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        Strip_String(string1);
                        FileTag->album = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
                        Strip_String(string);
                        FileTag->album = g_strdup(string);
                    }
                }
            }
        }


        /********
         * Year *
         ********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_YEAR)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    gchar *tmp_str;

                    Strip_String(string);

                    /* Fix for id3lib 3.7.x: if the id3v1.x tag was filled with spaces
                     * instead of zeroes, then the year field contains garbages! */
                    tmp_str = string;
                    while (isdigit(*tmp_str)) tmp_str++;
                    *tmp_str = 0;
                    /* End of fix for id3lib 3.7.x */

                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        Strip_String(string1);
                        FileTag->year = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
                        Strip_String(string);
                        FileTag->year = g_strdup(string);
                    }
                }
            }
        }


        /*************************
         * Track and Total Track *
         *************************/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_TRACKNUM)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    
                    Strip_String(string);

                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        string2 = strchr(string1,'/');
                        if (NUMBER_TRACK_FORMATED)
                        {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup_printf("%.2d",atoi(string2+1)); // Just to have numbers like this : '01', '05', '12', ...
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup_printf("%.2d",atoi(string1)); // Just to have numbers like this : '01', '05', '12', ...
                        }else
                        {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup(string2+1);
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup(string1);
                        }
                        g_free(string1);
                    }else
                    {
                        string2 = strchr(string,'/');
                        if (NUMBER_TRACK_FORMATED)
                        {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup_printf("%.2d",atoi(string2+1)); // Just to have numbers like this : '01', '05', '12', ...
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup_printf("%.2d",atoi(string)); // Just to have numbers like this : '01', '05', '12', ...
                        }else
                            {
                            if (string2)
                            {
                                FileTag->track_total = g_strdup(string2+1);
                                *string2 = '\0';
                            }
                            FileTag->track = g_strdup(string);
                        }
                    }
                }
            }
        }


        /*********
         * Genre *
         *********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_CONTENTTYPE)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                /*
                 * We manipulate only the name of the genre
                 */
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    gchar *tmp;

                    Strip_String(string);

                    if ( (string[0]=='(') && (tmp=strchr(string,')')) && (strlen((tmp+1))>0) )
                    {
    
                        /* Convert a genre written as '(3)Dance' into 'Dance' */
                        if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(tmp+1);
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(tmp+1);
                        }

                    }else if ( (string[0]=='(') && (tmp=strchr(string,')')) )
                    {
    
                        /* Convert a genre written as '(3)' into 'Dance' */
                        *tmp = 0;
                        if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(Id3tag_Genre_To_String(atoi(string+1)));
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(Id3tag_Genre_To_String(atoi(string+1)));
                        }

                    }else
                    {

                        /* Genre is already written as 'Dance' */
                            if (USE_CHARACTER_SET_TRANSLATION)
                        {
                            string1 = convert_from_file_to_user(string);
                            FileTag->genre = g_strdup(string1);
                            g_free(string1);
                        }else
                        {
                            FileTag->genre = g_strdup(string);
                        }

                    }
                }
            }
        }


        /***********
         * Comment *
         ***********/
        if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_COMMENT)) )
        {
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_TEXT)) )
            {
                if ( (num_chars=ID3Field_GetASCII_1(id3_field,string,ID3V2_MAX_STRING_LEN,field_num)) > 0
                     && string != NULL )
                {
                    if (USE_CHARACTER_SET_TRANSLATION)
                    {
                        string1 = convert_from_file_to_user(string);
                        Strip_String(string1);
                        FileTag->comment = g_strdup(string1);
                        g_free(string1);
                    }else
                    {
                        Strip_String(string);
                        FileTag->comment = g_strdup(string);
                    }
                }
            }
            /*if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_DESCRIPTION)) )
            {
                gchar *comment1 = g_malloc0(MAX_STRING_LEN+1);
                num_chars = ID3Field_GetASCII(id3_field,comment1,MAX_STRING_LEN,Item_Num);
                g_free(comment1);
            }
            if ( (id3_field = ID3Frame_GetField(id3_frame,ID3FN_LANGUAGE)) )
            {
                gchar *comment2 = g_malloc0(MAX_STRING_LEN+1);
                num_chars = ID3Field_GetASCII(id3_field,comment2,MAX_STRING_LEN,Item_Num);
                g_free(comment2);
            }*/
        }
        g_free(string);

        /* Free allocated data */
        ID3Tag_Delete(id3_tag);
    }

    return TRUE;
}

void Initialize_File_Tag_Item (File_Tag *FileTag)
{
    if (FileTag)
    {
        FileTag->key         = 0;
        FileTag->saved       = FALSE;    
        FileTag->title       = NULL;
        FileTag->artist      = NULL;
        FileTag->album       = NULL;
        FileTag->track       = NULL;
        FileTag->track_total = NULL;
        FileTag->year        = NULL;
        FileTag->genre       = NULL;
        FileTag->comment     = NULL;
    }
}

/*
 * Frees a File_Tag item.
 */
gboolean Free_File_Tag_Item (File_Tag *FileTag)
{
    if (!FileTag) return FALSE;

    if (FileTag->title)       g_free(FileTag->title);
    if (FileTag->artist)      g_free(FileTag->artist);
    if (FileTag->album)       g_free(FileTag->album);
    if (FileTag->year)        g_free(FileTag->year);
    if (FileTag->track)       g_free(FileTag->track);
    if (FileTag->track_total) g_free(FileTag->track_total);
    if (FileTag->genre)       g_free(FileTag->genre);
    if (FileTag->comment)     g_free(FileTag->comment);

    return TRUE;
}

/*
 * Returns the name of a genre code if found
 * Three states for genre code :
 *    - defined (0 to GENRE_MAX)
 *    - undefined/unknown (GENRE_MAX+1 to ID3_INVALID_GENRE-1)
 *    - invalid (>ID3_INVALID_GENRE)
 */
static gchar *Id3tag_Genre_To_String (unsigned char genre_code)
{
    if (genre_code>=ID3_INVALID_GENRE)    /* empty */
        return "";
    else if (genre_code>GENRE_MAX)        /* unknown tag */
        return "Unknown";
    else                                  /* known tag */
        return id3_genres[genre_code];
}



/*
 * As the ID3Tag_Link function of id3lib-3.8.0pre2 returns the ID3v1 tags
 * when a file has both ID3v1 and ID3v2 tags, we first try to explicitely
 * get the ID3v2 tags with ID3Tag_LinkWithFlags and, if we cannot get them,
 * fall back to the ID3v1 tags.
 * (Written by Holger Schemel).
 */
static size_t ID3Tag_Link_1 (ID3Tag *id3tag, const char *filename)
{
    size_t offset;

#   if ( (ID3LIB_MAJOR >= 3) && (ID3LIB_MINOR >= 8)  )
        /* First, try to get the ID3v2 tags */
        offset = ID3Tag_LinkWithFlags(id3tag,filename,ID3TT_ID3V2);
        if (offset == 0)
        {
            /* No ID3v2 tags available => try to get the ID3v1 tags */
            offset = ID3Tag_LinkWithFlags(id3tag,filename,ID3TT_ID3V1);
        }
#   else
        /* Function 'ID3Tag_LinkWithFlags' is not defined up to id3lib-.3.7.13 */
        offset = ID3Tag_Link(id3tag,filename);
#   endif
    //g_print("ID3 TAG SIZE: %d\t%s\n",offset,g_basename(filename));
    return offset;
}


/*
 * As the ID3Field_GetASCII function differs with the version of id3lib, we must redefine it.
 */
static size_t ID3Field_GetASCII_1(const ID3Field *field, char *buffer, size_t maxChars, index_t itemNum)
{

    /* Defined by id3lib:   ID3LIB_MAJOR_VERSION, ID3LIB_MINOR_VERSION, ID3LIB_PATCH_VERSION
     * Defined by autoconf: ID3LIB_MAJOR,         ID3LIB_MINOR,         ID3LIB_PATCH
     *
     * <= 3.7.12 : first item num is 1 for ID3Field_GetASCII
     *  = 3.7.13 : first item num is 0 for ID3Field_GetASCII
     * >= 3.8.0  : doesn't need item num for ID3Field_GetASCII
     */
     //g_print("id3lib version: %d.%d.%d\n",ID3LIB_MAJOR,ID3LIB_MINOR,ID3LIB_PATCH);
#    if (ID3LIB_MAJOR >= 3)
         // (>= 3.x.x)
#        if (ID3LIB_MINOR <= 7)
             // (3.0.0 to 3.7.x)
#            if (ID3LIB_PATCH >= 13)
                 // (>= 3.7.13)
                 return ID3Field_GetASCII(field,buffer,maxChars,itemNum);
#            else
                 return ID3Field_GetASCII(field,buffer,maxChars,itemNum+1);
#            endif
#        else
             // (>= to 3.8.0)
             //return ID3Field_GetASCII(field,buffer,maxChars);
             return ID3Field_GetASCIIItem(field,buffer,maxChars,itemNum);
#        endif
#    else
         // Not tested (< 3.x.x)
         return ID3Field_GetASCII(field,buffer,maxChars,itemNum+1);
#    endif
}

/*
 * Delete spaces at the end and the beginning of the string 
 */
static void Strip_String (gchar *string)
{
    if (!string) return;
    string = g_strstrip(string);
}

