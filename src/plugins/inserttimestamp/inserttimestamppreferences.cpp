/*
 * gnote
 *
 * Copyright (C) 2011-2013,2017,2019-2020,2023 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <glibmm/i18n.h>

#include "sharp/datetime.hpp"
#include "sharp/propertyeditor.hpp"

#include "preferences.hpp"
#include "inserttimestamppreferences.hpp"
 
namespace inserttimestamp {

  const char * SCHEMA_INSERT_TIMESTAMP = "org.gnome.gnote.insert-timestamp";
  const char * INSERT_TIMESTAMP_FORMAT = "format";

  bool InsertTimestampPreferences::s_static_inited = false;
  std::vector<Glib::ustring> InsertTimestampPreferences::s_formats;
  Glib::RefPtr<Gio::Settings> InsertTimestampPreferences::s_settings;

  Glib::RefPtr<Gio::Settings> & InsertTimestampPreferences::settings()
  {
    if(!s_settings) {
      s_settings = Gio::Settings::create(SCHEMA_INSERT_TIMESTAMP);
    }

    return s_settings;
  }

  void InsertTimestampPreferences::_init_static()
  {
    if(!s_static_inited) {
      s_formats.push_back("%c");
      s_formats.push_back("%m/%d/%y %H:%M:%S");
      s_formats.push_back("%m/%d/%y");
      s_formats.push_back("%Y-%m-%d %H:%M:%S");
      s_formats.push_back("%Y-%m-%d");

      s_static_inited = true;
    }
  }


  InsertTimestampPreferences::InsertTimestampPreferences(gnote::IGnote &, gnote::Preferences &, gnote::NoteManager &)
  {
    _init_static();

    set_row_spacing(12);
    int row = 0;

    // Get current values
    auto ts_settings = settings();
    Glib::ustring dateFormat = ts_settings->get_string(INSERT_TIMESTAMP_FORMAT);

    auto now = Glib::DateTime::create_now_local();

    // Label
    auto label = Gtk::make_managed<Gtk::Label>(_("Choose one of the predefined formats or use your own."));
    label->property_wrap() = true;
    label->property_xalign() = 0;
    attach(*label, 0, row++, 1, 1);

    // Use Selected Format
    selected_radio = Gtk::make_managed<Gtk::CheckButton>(_("Use _Selected Format"), true);
    attach(*selected_radio, 0, row++, 1, 1);

    // 1st column (visible): formatted date
    // 2nd column (not visible): date format
    store = Gtk::ListStore::create(m_columns);

    for(auto format : s_formats) {
      Gtk::TreeIter treeiter = store->append();
      treeiter->set_value(0, sharp::date_time_to_string(now, format));
      treeiter->set_value(1, format);
    }

    scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_hexpand(true);
    scroll->set_vexpand(true);
    attach(*scroll, 0, row++, 1, 1);

    tv = Gtk::make_managed<Gtk::TreeView>(store);
    tv->set_headers_visible(false);
    tv->append_column ("Format", m_columns.formatted);

    scroll->set_child(*tv);

    // Use Custom Format
    auto customBox = Gtk::make_managed<Gtk::Grid>();
    customBox->set_column_spacing(12);
    attach(*customBox, 0, row++, 1, 1);

    custom_radio = Gtk::make_managed<Gtk::CheckButton>(_("_Use Custom Format"), true);
    custom_radio->set_group(*selected_radio);
    customBox->attach(*custom_radio, 0, 0, 1, 1);

    custom_entry = Gtk::make_managed<Gtk::Entry>();
    customBox->attach(*custom_entry, 1, 0, 1, 1);

    sharp::PropertyEditor *entryEditor = new sharp::PropertyEditor(
      [ts_settings]()->Glib::ustring { return ts_settings->get_string(INSERT_TIMESTAMP_FORMAT); },
      [ts_settings](const Glib::ustring & value) { ts_settings->set_string(INSERT_TIMESTAMP_FORMAT, value); },
      *custom_entry);
    entryEditor->setup ();

    // Activate/deactivate widgets
    bool useCustom = true;
    Gtk::TreeIter<Gtk::TreeConstRow> iter;
    for(iter = store->children().begin();
        iter != store->children().end(); ++iter) {

      const Gtk::TreeConstRow & tree_row(*iter);
      Glib::ustring value = tree_row[m_columns.format];
      if (dateFormat == value) {
        // Found format in list
        useCustom = false;
        break;
      }
    }

    if (useCustom) {
      custom_radio->set_active(true);
      scroll->set_sensitive(false);
    } 
    else {
      selected_radio->set_active(true);
      custom_entry->set_sensitive(false);
      tv->get_selection()->select(iter);
      Gtk::TreePath treepath = store->get_path (iter);				
      tv->scroll_to_row(treepath);
    }

    // Register Toggled event for one radio button only
    selected_radio->signal_toggled().connect(
      sigc::mem_fun(*this, 
                    &InsertTimestampPreferences::on_selected_radio_toggled));
    tv->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, 
                    &InsertTimestampPreferences::on_selection_changed));
  }


  /// Called when toggling between radio buttons.
  /// Activate/deactivate widgets depending on selection.
  void InsertTimestampPreferences::on_selected_radio_toggled ()
  {
    if (selected_radio->get_active()) {
      scroll->set_sensitive(true);
      custom_entry->set_sensitive(false);
      // select 1st row
      auto iter = store->children().begin();
      tv->get_selection()->select(iter);
      Gtk::TreePath treepath = store->get_path(iter);				
      tv->scroll_to_row(treepath);
    } 
    else {
      scroll->set_sensitive(false);
      custom_entry->set_sensitive(true);
      tv->get_selection()->unselect_all ();
    }
  }

  /// Called when a different format is selected in the TreeView.
  /// Set the GConf key to selected format.
  void InsertTimestampPreferences::on_selection_changed ()
  {
    auto iter = tv->get_selection()->get_selected();
    if (iter) {
      Glib::ustring format;
      iter->get_value(1, format);
      settings()->set_string(INSERT_TIMESTAMP_FORMAT, format);
    }
  }

}
