/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2021 Aurimas Cernius
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



#ifndef __NOTE_TAG_HPP_
#define __NOTE_TAG_HPP_

#include <map>

#include <sigc++/signal.h>
#include <glibmm/refptr.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttag.h>
#include <gtkmm/texttagtable.h>

#include "contrast.hpp"
#include "tag.hpp"
#include "sharp/exception.hpp"

namespace sharp {
  class XmlWriter;
  class XmlReader;
}

namespace gnote {
  class NoteEditor;


enum TagSaveType {
  NO_SAVE,
  META,
  CONTENT
};



class NoteTag
  : public Gtk::TextTag
{
public:
  typedef Glib::RefPtr<NoteTag> Ptr;
  typedef Glib::RefPtr<const NoteTag> ConstPtr;
  typedef sigc::signal<bool, const NoteEditor &,
                       const Gtk::TextIter &, const Gtk::TextIter &> TagActivatedHandler;

  enum TagFlags {
    NO_FLAG       = 0,
    CAN_SERIALIZE = 1,
    CAN_UNDO      = 2,
    CAN_GROW      = 4,
    CAN_SPELL_CHECK = 8,
    CAN_ACTIVATE  = 16,
    CAN_SPLIT     = 32
  };

  static Ptr create(const Glib::ustring & tag_name, int flags)
    {
      return Ptr(new NoteTag(tag_name, flags));
    }
  const Glib::ustring & get_element_name() const
    { 
      return m_element_name;
    }
  TagSaveType save_type() const
    {
      return m_save_type;
    }
  void set_save_type(TagSaveType type)
    {
      m_save_type = type;
    }
  bool can_serialize() const
    {
      return (m_flags & CAN_SERIALIZE) != 0;
    }
  void set_can_serialize(bool value);
  bool can_undo() const
    {
      return (m_flags & CAN_UNDO) != 0;
    }
  void set_can_undo(bool value);
  bool can_grow() const
    {
      return (m_flags & CAN_GROW) != 0;
    }
  void set_can_grow(bool value);
  bool can_spell_check() const
    {
      return (m_flags & CAN_SPELL_CHECK) != 0;
    }
  void set_can_spell_check(bool value);
  bool can_activate() const
    {
      return (m_flags & CAN_ACTIVATE) != 0;
    }
  void set_can_activate(bool value);
  bool can_split() const
    {
      return (m_flags & CAN_SPLIT) != 0;
    }
  void set_can_split(bool value);

  void get_extents(const Gtk::TextIter & iter, Gtk::TextIter & start,
                   Gtk::TextIter & end);
  virtual void write(sharp::XmlWriter &, bool) const;
  virtual void read(sharp::XmlReader &, bool);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_image() const;
  virtual void set_image(const Glib::RefPtr<Gdk::Pixbuf> &);
  virtual Gtk::Widget * get_widget() const
    {
      return m_widget;
    }
  virtual void set_widget(Gtk::Widget *);
  const Glib::RefPtr<Gtk::TextMark> & get_widget_location() const
    { 
      return m_widget_location; 
    }
  void set_widget_location(const Glib::RefPtr<Gtk::TextMark> & value)
    { 
      m_widget_location = value; 
    }
////
  TagActivatedHandler & signal_activate()
    { 
      return m_signal_activate;
    }
  sigc::signal<void,const Gtk::TextTag&,bool> & signal_changed()
    { 
      return m_signal_changed;
    }
  ContrastPaletteColor get_palette_foreground() const
    {
      return m_palette_foreground;
    }
  void set_palette_foreground(ContrastPaletteColor value)
    {
      m_palette_foreground = value;
      // XXX We should also watch theme changes.
      property_foreground_rgba().set_value(render_foreground(value));
    }
protected:
  NoteTag(const Glib::ustring & tag_name, int flags = 0);
  NoteTag();
  virtual void initialize(const Glib::ustring & element_name);

  friend class NoteTagTable;

  virtual bool on_event(const Glib::RefPtr<Glib::Object> &, GdkEvent *, const Gtk::TextIter & ) override;
  virtual bool on_activate(const NoteEditor & , const Gtk::TextIter &, const Gtk::TextIter &);
private:
  Gdk::RGBA get_background() const;
  Gdk::RGBA render_foreground(ContrastPaletteColor symbol);

  Glib::ustring       m_element_name;
  Glib::RefPtr<Gtk::TextMark> m_widget_location;
  Gtk::Widget       * m_widget;
  bool                m_allow_middle_activate;
  int                 m_flags;
  TagActivatedHandler m_signal_activate;
  ContrastPaletteColor m_palette_foreground;
  sigc::signal<void,const Gtk::TextTag&,bool> m_signal_changed;
  TagSaveType         m_save_type;
};


class DynamicNoteTag
  : public NoteTag
{
public:
  typedef Glib::RefPtr<DynamicNoteTag> Ptr;
  typedef Glib::RefPtr<const DynamicNoteTag> ConstPtr;
  typedef std::map<Glib::ustring, Glib::ustring> AttributeMap;

  const AttributeMap & get_attributes() const
    {
      return m_attributes;
    }
  AttributeMap & get_attributes()
    {
      return m_attributes;
    }
  virtual void write(sharp::XmlWriter &, bool) const override;
  virtual void read(sharp::XmlReader &, bool) override;
  /// <summary>
  /// Derived classes should override this if they desire
  /// to be notified when a tag attribute is read in.
  /// </summary>
  /// <param name="attributeName">
  /// A <see cref="System.String"/> that is the name of the
  /// newly read attribute.
  /// </param>
  virtual void on_attribute_read(const Glib::ustring &)
    {
    }

private:
  AttributeMap m_attributes;
};


class DepthNoteTag
  : public NoteTag
{
public:
  typedef Glib::RefPtr<DepthNoteTag> Ptr;

  DepthNoteTag(int depth);

  int get_depth() const
    { 
      return m_depth; 
    }
  Pango::Direction get_direction() const
    {
      return Pango::DIRECTION_LTR;
    }
  virtual void write(sharp::XmlWriter &, bool) const override;
private:
  int            m_depth;
};


#if 0
class TagType 
{
public:
  typedef sigc::signal<DynamicNoteTag::Ptr, const Glib::ustring &> Factory;
  typedef sigc::slot<DynamicNoteTag::Ptr, const Glib::ustring &> FactorySlot;
  TagType(const FactorySlot & factory) 
    {
      m_factory.connect(factory);
    }
  TagType(const TagType & rhs) 
    {
      m_factory = rhs.m_factory;
    }
  TagType()
    {}
  DynamicNoteTag::Ptr create(const Glib::ustring & name)
    {
      return m_factory(name);
    }
private:
  Factory m_factory;
};
#endif


class NoteTagTable
  : public Gtk::TextTagTable
{
public:
  typedef Glib::RefPtr<NoteTagTable> Ptr;
  typedef sigc::slot<DynamicNoteTag::Ptr> Factory;

  static const NoteTagTable::Ptr & instance() 
    {
      if(!s_instance) {
        s_instance = NoteTagTable::Ptr(new NoteTagTable);
      }
      return s_instance;
    }
  static bool tag_is_serializable(const Glib::RefPtr<const Gtk::TextTag> & );
  static bool tag_is_growable(const Glib::RefPtr<Gtk::TextTag> & );
  static bool tag_is_undoable(const Glib::RefPtr<Gtk::TextTag> & );
  static bool tag_is_spell_checkable(const Glib::RefPtr<const Gtk::TextTag> & );
  static bool tag_is_activatable(const Glib::RefPtr<Gtk::TextTag> & );
  static bool tag_has_depth(const Glib::RefPtr<Gtk::TextBuffer::Tag> & );
  bool has_link_tag(const Gtk::TextIter & iter);

  /// <summary>
  /// Maps a Gtk.TextTag to ChangeType for saving notes
  /// </summary>
  /// <param name="tag">Gtk.TextTag to map</param>
  /// <returns>ChangeType to save this NoteTag</returns>
  ChangeType get_change_type(const Glib::RefPtr<Gtk::TextTag> &tag);

  DepthNoteTag::Ptr get_depth_tag(int depth);
  DynamicNoteTag::Ptr create_dynamic_tag(const Glib::ustring & );
  void register_dynamic_tag(const Glib::ustring & tag_name, const Factory & factory);
  bool is_dynamic_tag_registered(const Glib::ustring &);


  NoteTag::Ptr get_url_tag() const
    {
      return m_url_tag;
    }
  NoteTag::Ptr get_link_tag() const
    {
      return m_link_tag;
    }
  NoteTag::Ptr get_broken_link_tag() const
    {
      return m_broken_link_tag;
    }
  
protected:
  NoteTagTable()
    {
      _init_common_tags();
    }

  virtual void on_tag_added(const Glib::RefPtr<Gtk::TextTag> &) override;
  virtual void on_tag_removed(const Glib::RefPtr<Gtk::TextTag> &) override;
//  virtual void on_notetag_changed(Glib::RefPtr<Gtk::TextTag>& tag, bool size_changed);

private:
  void _init_common_tags();

  static NoteTagTable::Ptr           s_instance;
  std::map<Glib::ustring, Factory>   m_tag_types;
  std::vector<Glib::RefPtr<Gtk::TextTag> > m_added_tags;

  NoteTag::Ptr m_url_tag;
  NoteTag::Ptr m_link_tag;
  NoteTag::Ptr m_broken_link_tag;
};


}

#endif

