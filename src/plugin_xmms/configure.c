/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002  Daisuke Shimamura
 *
 * Based on mpg123 plugin
 *          and prefs.c - 2000/05/06
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2000-2002  Jerome Couderc <j.couderc@ifrance.com>
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <pthread.h>

#include <xmms/configfile.h>
#include <xmms/dirbrowser.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>
#include <xmms/plugin.h>

#include "mylocale.h"
#include "charset.h"
#include "configure.h"

/*
 * Initialize Global Valueable
 */
flac_config_t flac_cfg = {
	FALSE,
	NULL,
	FALSE,
	NULL,
	NULL
};


static GtkWidget *flac_configurewin = NULL;
static GtkWidget *vbox, *notebook;

static GtkWidget *title_tag_override, *title_tag_box, *title_tag_entry, *title_desc;
static GtkWidget *convert_char_set, *fileCharacterSetEntry, *userCharacterSetEntry;

static gchar *gtk_entry_get_text_1 (GtkWidget *widget);
static void flac_configurewin_ok(GtkWidget * widget, gpointer data);
static void configure_destroy(GtkWidget * w, gpointer data);
static void title_tag_override_cb(GtkWidget * w, gpointer data);
static void convert_char_set_cb(GtkWidget * w, gpointer data);

static void flac_configurewin_ok(GtkWidget * widget, gpointer data)
{
	ConfigFile *cfg;
	gchar *filename;

	(void)widget, (void)data; /* unused arguments */
	g_free(flac_cfg.tag_format);
        flac_cfg.tag_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(title_tag_entry)));	
	flac_cfg.file_char_set = Charset_Get_Name_From_Title(gtk_entry_get_text_1(fileCharacterSetEntry));
	flac_cfg.user_char_set = Charset_Get_Name_From_Title(gtk_entry_get_text_1(userCharacterSetEntry));

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfg = xmms_cfg_open_file(filename);
	if (!cfg)
		cfg = xmms_cfg_new();
	xmms_cfg_write_boolean(cfg, "flac", "tag_override", flac_cfg.tag_override);
	xmms_cfg_write_string(cfg, "flac", "tag_format", flac_cfg.tag_format);
	xmms_cfg_write_boolean(cfg, "flac", "convert_char_set", flac_cfg.convert_char_set);
	xmms_cfg_write_string(cfg, "flac", "file_char_set", flac_cfg.file_char_set);
	xmms_cfg_write_string(cfg, "flac", "user_char_set", flac_cfg.user_char_set);
	xmms_cfg_write_file(cfg, filename);
	xmms_cfg_free(cfg);
	g_free(filename);
	gtk_widget_destroy(flac_configurewin);
}

static void configure_destroy(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
}

static void title_tag_override_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.tag_override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_tag_override));

	gtk_widget_set_sensitive(title_tag_box, flac_cfg.tag_override);
	gtk_widget_set_sensitive(title_desc, flac_cfg.tag_override);

}

static void convert_char_set_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.convert_char_set = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(convert_char_set));

	gtk_widget_set_sensitive(fileCharacterSetEntry, flac_cfg.convert_char_set);
	gtk_widget_set_sensitive(userCharacterSetEntry, flac_cfg.convert_char_set);
}

void FLAC_XMMS__configure(void)
{
	GtkWidget *title_frame, *title_tag_vbox, *title_tag_label;
	GtkWidget *label, *hbox;
	GtkWidget *bbox, *ok, *cancel;
	GList *list;

	if (flac_configurewin != NULL) {
		gdk_window_raise(flac_configurewin->window);
		return;
	}
	flac_configurewin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(flac_configurewin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &flac_configurewin);
	gtk_signal_connect(GTK_OBJECT(flac_configurewin), "destroy", GTK_SIGNAL_FUNC(configure_destroy), &flac_configurewin);
	gtk_window_set_title(GTK_WINDOW(flac_configurewin), _("Flac Configuration"));
	gtk_window_set_policy(GTK_WINDOW(flac_configurewin), FALSE, FALSE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(flac_configurewin), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(flac_configurewin), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/* Title config.. */

	title_frame = gtk_frame_new(_("ID3 Tags:"));
	gtk_container_border_width(GTK_CONTAINER(title_frame), 5);

	title_tag_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(title_tag_vbox), 5);
	gtk_container_add(GTK_CONTAINER(title_frame), title_tag_vbox);

	/* Convert Char Set */

	convert_char_set = gtk_check_button_new_with_label(_("Convert Character Set"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(convert_char_set), flac_cfg.convert_char_set);
	gtk_signal_connect(GTK_OBJECT(convert_char_set), "clicked", convert_char_set_cb, NULL);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), convert_char_set, FALSE, FALSE, 0);
	// Combo boxes...
	hbox = gtk_hbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(title_tag_vbox),hbox);
	label = gtk_label_new(_("Convert character set from :"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	fileCharacterSetEntry = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(hbox),fileCharacterSetEntry,TRUE,TRUE,0);

	label = gtk_label_new (_("to :"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	userCharacterSetEntry = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(hbox),userCharacterSetEntry,TRUE,TRUE,0);

	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fileCharacterSetEntry)->entry),FALSE);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(userCharacterSetEntry)->entry),FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(fileCharacterSetEntry),TRUE,FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(userCharacterSetEntry),TRUE,FALSE);

	list = Charset_Create_List();
	gtk_combo_set_popdown_strings(GTK_COMBO(fileCharacterSetEntry),list);
	gtk_combo_set_popdown_strings(GTK_COMBO(userCharacterSetEntry),list);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(fileCharacterSetEntry)->entry),Charset_Get_Title_From_Name(flac_cfg.file_char_set));
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(userCharacterSetEntry)->entry),Charset_Get_Title_From_Name(flac_cfg.user_char_set));
	gtk_widget_set_sensitive(fileCharacterSetEntry, flac_cfg.convert_char_set);
	gtk_widget_set_sensitive(userCharacterSetEntry, flac_cfg.convert_char_set);

	/* Override Tagging Format */

	title_tag_override = gtk_check_button_new_with_label(_("Override generic titles"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_tag_override), flac_cfg.tag_override);
	gtk_signal_connect(GTK_OBJECT(title_tag_override), "clicked", title_tag_override_cb, NULL);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_override, FALSE, FALSE, 0);

        title_tag_box = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(title_tag_box, flac_cfg.tag_override);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_box, FALSE, FALSE, 0);
	
	title_tag_label = gtk_label_new(_("Title format:"));
	gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_label, FALSE, FALSE, 0);
	
	title_tag_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(title_tag_entry), flac_cfg.tag_format);
	gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_entry, TRUE, TRUE, 0);

	title_desc = xmms_titlestring_descriptions("pafFetnygc", 2);
	gtk_widget_set_sensitive(title_desc, flac_cfg.tag_override);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_desc, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), title_frame, gtk_label_new(_("Title")));

	/* Buttons */

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(flac_configurewin_ok), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(flac_configurewin));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(flac_configurewin);
}

void FLAC_XMMS__aboutbox()
{
	static GtkWidget *about_window;

	if (about_window)
		gdk_window_raise(about_window->window);

	about_window = xmms_show_message(
		_("About Flac Plugin"),
		_("Flac Plugin by Josh Coalson\n"
		  "contributions by\n"
		  "......\n"
		  "......\n"
		  "and\n"
		  "Daisuke Shimamura\n"
		  "Visit http://flac.sourceforge.net/"),
		_("Ok"), FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(about_window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &about_window);
}

/*
 * Get text of an Entry or a ComboBox
 */
static gchar *gtk_entry_get_text_1 (GtkWidget *widget)
{
    if (GTK_IS_COMBO(widget))
    {
        return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));
    }else if (GTK_IS_ENTRY(widget))
    {
        return gtk_entry_get_text(GTK_ENTRY(widget));
    }else
    {
        return NULL;
    }
}

