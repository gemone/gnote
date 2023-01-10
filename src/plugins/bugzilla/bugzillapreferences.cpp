/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2022-2023 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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



#include <glib.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>

#include "sharp/directory.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "debug.hpp"
#include "utils.hpp"

#include "bugzillanoteaddin.hpp"
#include "bugzillapreferences.hpp"


namespace bugzilla {

  bool BugzillaPreferences::s_static_inited = false;;
  Glib::ustring BugzillaPreferences::s_image_dir;

  void BugzillaPreferences::_init_static()
  {
    if(!s_static_inited) {
      s_image_dir = BugzillaNoteAddin::images_dir();
      s_static_inited = true;
    }
  }

  BugzillaPreferences::BugzillaPreferences(gnote::IGnote &, gnote::Preferences &, gnote::NoteManager &)
  {
    _init_static();
    last_opened_dir = Glib::get_home_dir();

    set_row_spacing(12);
    int row = 0;

    auto l = Gtk::make_managed<Gtk::Label>(_("You can use any bugzilla just by dragging links "
      "into notes.  If you want a special icon for "
      "certain hosts, add them here."));
    l->property_wrap() = true;
    l->property_xalign() = 0;

    attach(*l, 0, row++, 1, 1);

    icon_store = Gtk::ListStore::create(m_columns);
    icon_store->set_sort_column(m_columns.host, Gtk::SortType::ASCENDING);

    icon_tree = Gtk::make_managed<Gtk::TreeView>(icon_store);
    icon_tree->set_headers_visible(true);
    icon_tree->get_selection()->set_mode(Gtk::SelectionMode::SINGLE);
    icon_tree->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &BugzillaPreferences::selection_changed));

    auto host_col = Gtk::make_managed<Gtk::TreeViewColumn>(_("Host Name"), m_columns.host);
    host_col->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
    host_col->set_resizable(true);
    host_col->set_expand(true);
    host_col->set_min_width(200);

    host_col->set_sort_column(m_columns.host);
    host_col->set_sort_indicator(false);
    host_col->set_reorderable(false);
    host_col->set_sort_order(Gtk::SortType::ASCENDING);

    icon_tree->append_column (*host_col);

    auto icon_col = Gtk::make_managed<Gtk::TreeViewColumn>(_("Icon"), m_columns.icon);
    icon_col->set_sizing(Gtk::TreeViewColumn::Sizing::FIXED);
    icon_col->set_max_width(50);
    icon_col->set_min_width(50);
    icon_col->set_resizable(false);

    icon_tree->append_column(*icon_col);

    Gtk::ScrolledWindow *sw = Gtk::make_managed<Gtk::ScrolledWindow>();
    sw->property_height_request() = 200;
    sw->property_width_request() = 300;
    sw->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    sw->set_child(*icon_tree);
    sw->set_hexpand(true);
    sw->set_vexpand(true);

    attach(*sw, 0, row++, 1, 1);

    add_button = Gtk::make_managed<Gtk::Button>(_("_Add"), true);
    add_button->signal_clicked().connect(
      sigc::mem_fun(*this, &BugzillaPreferences::add_clicked));

    remove_button = Gtk::make_managed<Gtk::Button>(_("_Remove"), true);
    remove_button->set_sensitive(false);
    remove_button->signal_clicked().connect(
      sigc::mem_fun(*this,  &BugzillaPreferences::remove_clicked));

    auto hbutton_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    hbutton_box->set_spacing(6);

    hbutton_box->append(*add_button);
    hbutton_box->append(*remove_button);
    attach(*hbutton_box, 0, row++, 1, 1);
  }


  void BugzillaPreferences::update_icon_store()
  {
    if (!sharp::directory_exists (s_image_dir)) {
      return;
    }

    icon_store->clear(); // clear out the old entries

    std::vector<Glib::ustring> icon_files = sharp::directory_get_files (s_image_dir);
    for(auto icon_file : icon_files) {
      sharp::FileInfo file_info(icon_file);

      Glib::RefPtr<Gdk::Pixbuf> pixbuf;
      try {
        pixbuf = Gdk::Pixbuf::create_from_file(icon_file);
      } 
      catch (const std::exception & e) {
        DBG_OUT("Error loading Bugzilla Icon %s: %s", icon_file.c_str(), e.what());
      }

      if (!pixbuf) {
        continue;
      }

      Glib::ustring host = parse_host(file_info);
      if (!host.empty()) {
        Gtk::TreeIter treeiter = icon_store->append ();
        
        (*treeiter)[m_columns.icon] = pixbuf;
        (*treeiter)[m_columns.host] = host;
        (*treeiter)[m_columns.file_path] = icon_file;
      }
    }
  }


  Glib::ustring BugzillaPreferences::parse_host(const sharp::FileInfo & file_info)
  {
    Glib::ustring name = file_info.get_name();
    Glib::ustring ext = file_info.get_extension();

    if (ext.empty()) {
      return "";
    }

    int ext_pos = name.find(ext);
    if (ext_pos <= 0) {
      return "";
    }

    Glib::ustring host = sharp::string_substring(name, 0, ext_pos);
    if (host.empty()) {
      return "";
    }

    return host;
  }

  void BugzillaPreferences::on_realize()
  {
    Gtk::Grid::on_realize();

    update_icon_store();
  }


  void BugzillaPreferences::selection_changed()
  {
    remove_button->set_sensitive(icon_tree->get_selection()->count_selected_rows() > 0);
  }
  
  namespace {

    /** sanitize the hostname. Return false if nothing could be done */
    bool sanitize_hostname(Glib::ustring & hostname)
    {
      if(hostname.find("/") != Glib::ustring::npos || hostname.find(":") != Glib::ustring::npos) {
        sharp::Uri uri(std::move(hostname));
        Glib::ustring new_host = uri.get_host();
        if(new_host.empty()) {
          return false;
        }
        hostname = std::move(new_host);
      }
      return true;
    }

  }

  void BugzillaPreferences::add_clicked()
  {
    auto dialog = Gtk::make_managed<Gtk::FileChooserDialog>(_("Select an icon..."), Gtk::FileChooser::Action::OPEN);
    dialog->add_button(_("_Cancel"), Gtk::ResponseType::CANCEL);
    dialog->add_button(_("_Open"), Gtk::ResponseType::OK);

    dialog->set_default_response(Gtk::ResponseType::OK);
    dialog->set_current_folder(Gio::File::create_for_path(last_opened_dir));

    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    filter->add_pixbuf_formats();

    dialog->add_filter(filter);

    // Extra Widget
    Gtk::Label *l = Gtk::make_managed<Gtk::Label>(_("_Host name:"), true);
    l->set_margin_start(6);
    Gtk::Entry *host_entry = Gtk::make_managed<Gtk::Entry>();
    host_entry->set_hexpand(true);
    host_entry->set_margin_end(6);
    l->set_mnemonic_widget(*host_entry);
    Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
    hbox->set_column_spacing(6);
    hbox->attach(*l, 0, 0, 1, 1);
    hbox->attach(*host_entry, 1, 0, 1, 1);

    dialog->get_content_area()->append(*hbox);
    dialog->show();
    dialog->signal_response().connect([this, dialog, host_entry](int response) {
      if(response == (int) Gtk::ResponseType::OK) {
        Glib::ustring icon_file = dialog->get_file()->get_path();
        Glib::ustring host = sharp::string_trim(host_entry->get_text());

        bool valid = sanitize_hostname(host);
      
        if(valid && !host.empty()) {
          // Keep track of the last directory the user had open
          last_opened_dir = dialog->get_current_folder()->get_path();

          // Copy the file to the BugzillaIcons directory
          Glib::ustring err_msg;
          if(!copy_to_bugzilla_icons_dir(icon_file, host, err_msg)) {
            auto err = Gtk::make_managed<gnote::utils::HIGMessageDialog>((Gtk::Window*)dialog->get_parent(),
              GTK_DIALOG_DESTROY_WITH_PARENT,
              Gtk::MessageType::ERROR, Gtk::ButtonsType::OK,
              _("Error saving icon"),
              Glib::ustring(_("Could not save the icon file.")) + "  " + err_msg);
            err->show();
            err->signal_response().connect([err](int) { err->hide(); });
          }
          else {
            dialog->hide();
            update_icon_store();
          }
        }
        else {
          // Let the user know that they
          // have to specify a host name.
          auto warn = Gtk::make_managed<gnote::utils::HIGMessageDialog>(
            dialog, GTK_DIALOG_DESTROY_WITH_PARENT,
            Gtk::MessageType::WARNING, Gtk::ButtonsType::OK,
            _("Host name invalid"),
            _("You must specify a valid Bugzilla "
              "host name to use with this icon."));
          warn->show();
          warn->signal_response().connect([warn, host_entry](int) {
            warn->hide();
            host_entry->grab_focus();
          });
        }
      } 
      else {
        dialog->hide();
        return;
      }
    });
  }

  bool BugzillaPreferences::copy_to_bugzilla_icons_dir(const Glib::ustring & file_path,
                                                       const Glib::ustring & host,
                                                       Glib::ustring & err_msg)
  {
    err_msg = "";

    sharp::FileInfo file_info(file_path);
    Glib::ustring ext = file_info.get_extension();
    Glib::ustring saved_path = s_image_dir + "/" + host + ext;
    try {
      if (!sharp::directory_exists (s_image_dir)) {
        g_mkdir_with_parents(s_image_dir.c_str(), S_IRWXU);
      }

      sharp::file_copy (file_path, saved_path);
    } 
    catch (const std::exception & e) {
      err_msg = e.what();
      return false;
    }

    resize_if_needed (saved_path);
    return true;
  }

  void BugzillaPreferences::resize_if_needed(const Glib::ustring & p)
  {
    Glib::RefPtr<Gdk::Pixbuf> pix, newpix;

    try {
      const double dim = 16;
      pix = Gdk::Pixbuf::create_from_file(p);
      int height, width;
      int orig_h, orig_w;
      orig_h = pix->get_height();
      orig_w = pix->get_width();
      int orig_dim = std::max(orig_h, orig_w);
      double ratio = dim / (double)orig_dim;
      width = (int)(ratio * orig_w);
      height = (int)(ratio * orig_h);
      newpix = pix->scale_simple(width, height, Gdk::InterpType::BILINEAR);
      newpix->save(p, "png");
    }
    catch(...) {

    }
  }


  void BugzillaPreferences::remove_clicked()
  {
    // Remove the icon file and call UpdateIconStore ().
    Gtk::TreeIter<Gtk::TreeRow> iter;
    iter = icon_tree->get_selection()->get_selected();
    if (!iter) {
      return;
    }

    Glib::ustring icon_path = (*iter)[m_columns.file_path];

    auto dialog = Gtk::make_managed<gnote::utils::HIGMessageDialog>(nullptr, 
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          Gtk::MessageType::QUESTION, Gtk::ButtonsType::NONE,
                                          _("Really remove this icon?"),
                                          _("If you remove an icon it is permanently lost."));

    auto button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
    dialog->add_action_widget(*button, Gtk::ResponseType::CANCEL);
    dialog->set_default_response(Gtk::ResponseType::CANCEL);

    button = Gtk::make_managed<Gtk::Button>(_("_Delete"), true);
    dialog->add_action_widget(*button, 666);

    dialog->show();
    dialog->signal_response().connect([this, dialog, icon_path](int result) {
      dialog->hide();
      if(result == 666) {
        try {
          sharp::file_delete(icon_path);
          update_icon_store();
        }
        catch(const sharp::Exception & e) {
          ERR_OUT(_("Error removing icon %s: %s"), icon_path.c_str(), e.what());
        }
      }
    });
  }


}

