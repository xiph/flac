/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999,2000  Håvard Kvålen
 *  Copyright (C) 2002  Daisuke Shimamura
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include "FLAC/all.h"
#include "id3_tag.h"
#include "configure.h"

gboolean get_file_info(char *filename, flac_file_info_struct *tmp_file_info)
{
	FLAC__StreamMetadata streaminfo;

	if(0 == filename)
		filename = "";

	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		return FALSE;
	}

	tmp_file_info->sample_rate = streaminfo.data.stream_info.sample_rate;
	tmp_file_info->channels = streaminfo.data.stream_info.channels;
	tmp_file_info->bits_per_sample = streaminfo.data.stream_info.bits_per_sample;
	tmp_file_info->total_samples = streaminfo.data.stream_info.total_samples;

	tmp_file_info->length_in_msec = streaminfo.data.stream_info.total_samples * 10 / (streaminfo.data.stream_info.sample_rate / 100);

	return TRUE;
}

static GtkWidget *window = NULL;
static GtkWidget *filename_entry, *id3_frame;
static GtkWidget *title_entry, *artist_entry, *album_entry, *year_entry, *tracknum_entry, *comment_entry;
static GtkWidget *genre_combo;
static GtkWidget *flac_level, *flac_bitrate, *flac_samplerate, *flac_flags;
static GtkWidget *flac_fileinfo, *flac_genre;

static gchar *current_filename = NULL;

extern gchar *flac_filename;
extern gint flac_bitrate, flac_frequency, flac_layer, flac_lsf, flac_mode;
extern gboolean flac_stereo, flac_flac25;

#define MAX_STR_LEN 100

static void set_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
	gint stripped_len;
	gchar *text;

	stripped_len = flac_strip_spaces(tag, length);
	text = g_strdup_printf("%-*.*s", stripped_len, stripped_len, tag);
	gtk_entry_set_text(entry, text);
	g_free(text);
}

static void get_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
	gchar *text;

	text = gtk_entry_get_text(entry);
	memset(tag, ' ', length);
	memcpy(tag, text, strlen(text) > length ? length : strlen(text));
}

static gint genre_comp_func(gconstpointer a, gconstpointer b)
{
	return strcasecmp(a, b);
}

static gchar* channel_mode_name(int mode)
{
	static const gchar *channel_mode[] = {N_("Mono"), N_("Stereo")};
	if (mode < 1 || mode > 2)
		return "";
	return gettext(channel_mode[mode]);
}

void FLAC_XMMS__file_info_box(char *filename)
{
	gint i;
	FILE *fh;
	gchar *tmp, *title;

	gboolean rc;

#ifdef FLAC__HAS_ID3LIB
	File_Tag tag;
	Initialize_File_Tag_Item (&tag);
	rc = Id3tag_Read_File_Tag (filename, &tag);
#else
	id3v2_struct tag;
	memset(&tag, 0, sizeof(tag));
	rc = get_id3v1_tag_as_v2_(filename, &tag);
#endif

	if (!window)
	{
		GtkWidget *vbox, *hbox, *left_vbox, *table;
		GtkWidget *flac_frame, *flac_box;
		GtkWidget *label, *filename_hbox;
		GtkWidget *bbox, *save, *remove_id3, *cancel;

		window = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
		gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &window);
		gtk_container_set_border_width(GTK_CONTAINER(window), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(window), vbox);

		filename_hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), filename_hbox, FALSE, TRUE, 0);

		label = gtk_label_new(_("Filename:"));
		gtk_box_pack_start(GTK_BOX(filename_hbox), label, FALSE, TRUE, 0);
		filename_entry = gtk_entry_new();
		gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
		gtk_box_pack_start(GTK_BOX(filename_hbox), filename_entry, TRUE, TRUE, 0);

		hbox = gtk_hbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

		left_vbox = gtk_vbox_new(FALSE, 10);
		gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);

		id3_frame = gtk_frame_new(_("ID3 Tag:"));
		gtk_box_pack_start(GTK_BOX(left_vbox), id3_frame, FALSE, FALSE, 0);

		table = gtk_table_new(5, 5, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 5);
		gtk_container_add(GTK_CONTAINER(id3_frame), table);

		label = gtk_label_new(_("Title:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 5, 5);

		title_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), title_entry, 1, 4, 0, 1, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Artist:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 5, 5);

		artist_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), artist_entry, 1, 4, 1, 2, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Album:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 5, 5);

		album_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), album_entry, 1, 4, 2, 3, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Comment:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 5, 5);

		comment_entry = gtk_entry_new_with_max_length(30);
		gtk_table_attach(GTK_TABLE(table), comment_entry, 1, 4, 3, 4, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Year:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 5, 5);

		year_entry = gtk_entry_new_with_max_length(4);
		gtk_widget_set_usize(year_entry, 40, -1);
		gtk_table_attach(GTK_TABLE(table), year_entry, 1, 2, 4, 5, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Track number:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5, GTK_FILL, GTK_FILL, 5, 5);

		tracknum_entry = gtk_entry_new_with_max_length(3);
		gtk_widget_set_usize(tracknum_entry, 40, -1);
		gtk_table_attach(GTK_TABLE(table), tracknum_entry, 3, 4, 4, 5, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		label = gtk_label_new(_("Genre:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 5, 5);

		genre_combo = gtk_combo_new();
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(genre_combo)->entry), FALSE);
		if (!genre_list)
		{
			for (i = 0; i < GENRE_MAX; i++)
				genre_list = g_list_prepend(genre_list, (char *) flac_id3_genres[i]);
			genre_list = g_list_prepend(genre_list, "");
			genre_list = g_list_sort(genre_list, genre_comp_func);
		}
		gtk_combo_set_popdown_strings(GTK_COMBO(genre_combo), genre_list);

		gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 4, 5, 6, GTK_FILL | GTK_EXPAND | GTK_SHRINK, GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

		bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
		gtk_box_pack_start(GTK_BOX(left_vbox), bbox, FALSE, FALSE, 0);

		save = gtk_button_new_with_label(_("Save"));
		gtk_signal_connect(GTK_OBJECT(save), "clicked", GTK_SIGNAL_FUNC(save_cb), NULL);
		GTK_WIDGET_SET_FLAGS(save, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), save, TRUE, TRUE, 0);
		gtk_widget_grab_default(save);

		remove_id3 = gtk_button_new_with_label(_("Remove ID3"));
		gtk_signal_connect(GTK_OBJECT(remove_id3), "clicked", GTK_SIGNAL_FUNC(remove_id3_cb), NULL);
		GTK_WIDGET_SET_FLAGS(remove_id3, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), remove_id3, TRUE, TRUE, 0);

		cancel = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(window));
		GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

		flac_frame = gtk_frame_new(_("FLAC Info:"));
		gtk_box_pack_start(GTK_BOX(hbox), flac_frame, FALSE, FALSE, 0);

		flac_box = gtk_vbox_new(FALSE, 5);
		gtk_container_add(GTK_CONTAINER(flac_frame), flac_box);
		gtk_container_set_border_width(GTK_CONTAINER(flac_box), 10);
		gtk_box_set_spacing(GTK_BOX(flac_box), 0);

		flac_level = gtk_label_new("");
		gtk_widget_set_usize(flac_level, 120, -2);
		gtk_misc_set_alignment(GTK_MISC(flac_level), 0, 0);
		gtk_box_pack_start(GTK_BOX(flac_box), flac_level, FALSE, FALSE, 0);

		flac_bitrate = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(flac_bitrate), 0, 0);
		gtk_label_set_justify(GTK_LABEL(flac_bitrate), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(flac_box), flac_bitrate, FALSE, FALSE, 0);

		flac_samplerate = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(flac_samplerate), 0, 0);
		gtk_box_pack_start(GTK_BOX(flac_box), flac_samplerate, FALSE, FALSE, 0);

		flac_flags = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(flac_flags), 0, 0);
		gtk_label_set_justify(GTK_LABEL(flac_flags), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(flac_box), flac_flags, FALSE, FALSE, 0);

		flac_fileinfo = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(flac_fileinfo), 0, 0);
		gtk_label_set_justify(GTK_LABEL(flac_fileinfo), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(flac_box), flac_fileinfo, FALSE, FALSE, 0);

		gtk_widget_show_all(window);
	}

	if (current_filename)
		g_free(current_filename);
	current_filename = g_strdup(filename);

	title = g_strdup_printf(_("File Info - %s"), g_basename(filename));
	gtk_window_set_title(GTK_WINDOW(window), title);
	g_free(title);

	gtk_entry_set_text(GTK_ENTRY(filename_entry), filename);
	gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

	if (tag.title != NULL && strlen(tag.title) > 0)
	{
		gtk_entry_set_text(GTK_ENTRY(title_entry), tag.title);
	}
	else
	{
		title = g_strdup(g_basename(filename));
		if ((tmp = strrchr(title, '.')) != NULL)
			*tmp = '\0';
		gtk_entry_set_text(GTK_ENTRY(title_entry), title);
		g_free(title);
	}

	gtk_entry_set_text(GTK_ENTRY(artist_entry), tag.artist);
	gtk_entry_set_text(GTK_ENTRY(album_entry), tag.album);
	gtk_entry_set_text(GTK_ENTRY(year_entry), tag.year);
	gtk_entry_set_text(GTK_ENTRY(tracknum_entry), tag.track);
	gtk_entry_set_text(GTK_ENTRY(comment_entry), tag.comment);
	gtk_entry_set_text(GTK_ENTRY(genre_entry), tag.genre);

	gtk_label_set_text(GTK_LABEL(flac_level), "");
	gtk_label_set_text(GTK_LABEL(flac_bitrate), "");
	gtk_label_set_text(GTK_LABEL(flac_samplerate), "");
	gtk_label_set_text(GTK_LABEL(flac_flags), "");
	gtk_label_set_text(GTK_LABEL(flac_fileinfo), "");

	if (!strncasecmp(filename, "http://", 7))
	{
		file_info_http(filename);
		return;
	}

	gtk_widget_set_sensitive(id3_frame, TRUE);

	label_set_text(flac_bitrate, _("Bits/Samples: %d"), tmp_file_info->bits_per_sample);
	/* tmp_file_info->length_in_msec */

	label_set_text(flac_samplerate, _("Samplerate: %ld Hz"), tmp_file_info->sample_rate);
	label_set_text(flac_channel, _("Channel: %s"), channel_mode_name(tmp_file_info->channel);
#if 0
	label_set_text(flac_fileinfo, _("%d frames\nFilesize: %lu B"), num_frames, ftell(fh));
#endif
}
