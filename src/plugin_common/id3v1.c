/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002  Josh Coalson
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

#include <stdio.h>

#include "FLAC/assert.h"
#include "id3v1.h"
#include "locale_hack.h"


/* 
 * Do not sort genres!!
 * Last Update: 2000/04/30
 */
static const char * const FLAC_plugin__id3v1_tag_genre_table[] =
{
	"Blues",		/* 0 */
	"Classic Rock",
	"Country",
	"Dance",
	"Disco",
	"Funk",			/* 5 */
	"Grunge",
	"Hip-Hop", 
	"Jazz",
	"Metal",
	"New Age",		/* 10 */		
	"Oldies",
	"Other", 
	"Pop",
	"R&B",
	"Rap",			/* 15 */
	"Reggae", 
	"Rock",
	"Techno",
	"Industrial",
	"Alternative", 		/* 20 */
	"Ska",
	"Death Metal", 
	"Pranks",
	"Soundtrack",
	"Euro-Techno", 		/* 25 */
	"Ambient",
	"Trip-Hop", 
	"Vocal",
	"Jazz+Funk", 
	"Fusion",		/* 30 */
	"Trance",
	"Classical",
	"Instrumental", 
	"Acid",
	"House",		/* 35 */
	"Game",
	"Sound Clip", 
	"Gospel",
	"Noise",
	"Altern Rock", 		/* 40 */
	"Bass",
	"Soul",
	"Punk",
	"Space",
	"Meditative",		/* 45 */
	"Instrumental Pop",
	"Instrumental Rock", 
	"Ethnic",
	"Gothic",
	"Darkwave",		/* 50 */
	"Techno-Industrial", 
	"Electronic", 
	"Pop-Folk",
	"Eurodance", 
	"Dream",		/* 55 */
	"Southern Rock", 
	"Comedy", 
	"Cult",
	"Gangsta",
	"Top 40",		/* 60 */
	"Christian Rap", 
	"Pop/Funk", 
	"Jungle",
	"Native American", 
	"Cabaret",		/* 65 */
	"New Wave",
	"Psychadelic", 
	"Rave",
	"Showtunes", 
	"Trailer",		/* 70 */
	"Lo-Fi",
	"Tribal",
	"Acid Punk",
	"Acid Jazz", 
	"Polka",		/* 75 */
	"Retro",
	"Musical",
	"Rock & Roll", 
	"Hard Rock", 
	"Folk",			/* 80 */
	"Folk/Rock",
	"National Folk", 
	"Fast Fusion",
	"Swing",
	"Bebob",		/* 85 */
	"Latin",
	"Revival",
	"Celtic",
	"Bluegrass",
	"Avantgarde",		/* 90 */
	"Gothic Rock",
	"Progressive Rock",
	"Psychedelic Rock", 
	"Symphonic Rock", 
	"Slow Rock",		/* 95 */
	"Big Band", 
	"Chorus",
	"Easy Listening", 
	"Acoustic", 
	"Humour",		/* 100 */
	"Speech",
	"Chanson", 
	"Opera",
	"Chamber Music", 
	"Sonata",		/* 105 */
	"Symphony",
	"Booty Bass", 
	"Primus",
	"Porn Groove", 
	"Satire",		/* 110 */
	"Slow Jam", 
	"Club",
	"Tango",
	"Samba",
	"Folklore",		/* 115 */
	"Ballad",
	"Power Ballad",
	"Rhythmic Soul",
	"Freestyle",
	"Duet",			/* 120 */
	"Punk Rock",
	"Drum Solo",
	"A Capella",
	"Euro-House",
	"Dance Hall",		/* 125 */
	"Goa",
	"Drum & Bass",
	"Club-House",
	"Hardcore",
	"Terror",		/* 130 */
	"Indie",
	"BritPop",
	"Negerpunk",
	"Polsk Punk",
	"Beat",			/* 135 */
	"Christian Gangsta Rap",
	"Heavy Metal",
	"Black Metal",
	"Crossover",
	"Contemporary Christian",/* 140 */
	"Christian Rock",
	"Merengue",
	"Salsa",
	"Thrash Metal",
	"Anime",		/* 145 */
	"JPop",
	"Synthpop"
};


FLAC__bool FLAC_plugin__id3v1_tag_get(const char *filename, FLAC_Plugin__Id3v1_Tag *tag)
{
	char raw[128];
	FILE *f;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != tag);

	memset(tag, 0, sizeof(FLAC_Plugin__Id3v1_Tag));

	if(0 == (f = fopen(filename, "rb")))
		return false;
	if(-1 == fseek(f, -128, SEEK_END)) {
		fclose(f);
		return false;
	}
	if(fread(raw, 1, 128, f) < 128) {
		fclose(f);
		return false;
	}
	fclose(f);
	if(strncmp(raw, "TAG", 3))
		return false;
	else {
		memcpy(tag->tag, raw, 3);
		memcpy(tag->title, raw+3, 30);
		memcpy(tag->artist, raw+33, 30);
		memcpy(tag->album, raw+63, 30);
		memcpy(tag->year, raw+93, 4);
		memcpy(tag->comment.v1_0.comment, raw+97, 30);
		tag->genre = raw[127];
		return true;
	}
}

const char *FLAC_plugin__id3v1_tag_get_genre_as_string(unsigned char genre_code)
{
	if (genre_code < FLAC_plugin__id3v1_tag_genre_table_max())
		return gettext(FLAC_plugin__id3v1_tag_genre_table[genre_code]);

	return "";
}

unsigned FLAC_plugin__id3v1_tag_genre_table_max()
{
	return sizeof(FLAC_plugin__id3v1_tag_genre_table) / sizeof(FLAC_plugin__id3v1_tag_genre_table[0]) - 1;
}
