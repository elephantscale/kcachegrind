/* This file is part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/*
 * Trace Item View
 */

#include "traceitemview.h"

#include <QWidget>

#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>

#include "toplevelbase.h"


#define TRACE_UPDATES 0

TraceItemView::TraceItemView(TraceItemView* parentView, TopLevelBase* top)
{
  _parentView = parentView;
  _topLevel = top ? top : parentView->topLevel();

  _data = _newData = 0;
  // _partList and _newPartList is empty
  _activeItem = _newActiveItem = 0;
  _selectedItem = _newSelectedItem = 0;
  _eventType = _newEventType = 0;
  _eventType2 = _newEventType2 = 0;
  _groupType = _newGroupType = TraceItem::NoCostType;

  _status = nothingChanged;
  _inUpdate = false;
  _pos = Hidden;
}

QString TraceItemView::whatsThis() const
{
    return i18n("No description available");
}

void TraceItemView::select(TraceItem* i)
{
    _newSelectedItem = i;
}

KConfigGroup TraceItemView::configGroup(KConfig* c,
                                        const QString& group, const QString& post)
{
    QStringList gList = c->groupList();
    QString g = group;
    if (gList.contains(group+post) ) g += post;
    return KConfigGroup(c, g);
}

void TraceItemView::writeConfigEntry(KConfigGroup &c, const char* pKey,
				     const QString& value, const char* def, bool bNLS)
{
    if ((value.isEmpty() && ((def == 0) || (*def == 0))) ||
	(value == QString(def)))
	c.deleteEntry(pKey);
    else
	{
		if( bNLS )
			c.writeEntry(pKey, value, KConfigBase::Normal|KConfigBase::Localized);
		else
			c.writeEntry(pKey, value);
	}
}

void TraceItemView::writeConfigEntry(KConfigGroup& c, const char* pKey,
				     int value, int def)
{
    if (value == def)
	c.deleteEntry(pKey);
    else
	c.writeEntry(pKey, value);
}

void TraceItemView::writeConfigEntry(KConfigGroup& c, const char* pKey,
				     double value, double def)
{
    if (value == def)
	c.deleteEntry(pKey);
    else
	c.writeEntry(pKey, value);
}

void TraceItemView::writeConfigEntry(KConfigGroup& c, const char* pKey,
				     bool value, bool def)
{
    if (value == def)
	c.deleteEntry(pKey);
    else
	c.writeEntry(pKey, value);
}

void TraceItemView::readViewConfig(KConfig*, const QString&, const QString&, bool)
{}

#if 1
void TraceItemView::saveViewConfig(KConfig*, const QString&, const QString&, bool)
{}
#else
void TraceItemView::saveViewConfig(KConfig* c,
                                   const QString& prefix, const QString& postfix, bool)
{
    // write a dummy config entry to see missing virtual functions
    KConfigGroup g(c, prefix+postfix);
    g.writeEntry("SaveNotImplemented", true);
}
#endif

bool TraceItemView::activate(TraceItem* i)
{
    i = canShow(i);
    _newActiveItem = i;

    return (i != 0);
}

TraceFunction* TraceItemView::activeFunction()
{
    TraceItem::CostType t = _activeItem->type();
    switch(t) {
    case TraceItem::Function:
    case TraceItem::FunctionCycle:
	return (TraceFunction*) _activeItem;
    default:
	break;
    }
    return 0;
}

bool TraceItemView::set(int changeType, TraceData* d,
			TraceEventType* t1, TraceEventType* t2,
			TraceItem::CostType g, const TracePartList& l,
                        TraceItem* a, TraceItem* s)
{
  _status |= changeType;
  _newData = d;
  _newGroupType = g;
  _newEventType = t1;
  _newEventType2 = t2;
  _newPartList = l;
  _newSelectedItem = s;
  _newActiveItem = canShow(a);
  if (!_newActiveItem) {
      _newSelectedItem = 0;
      return false;
  }

  return true;
}


bool TraceItemView::isViewVisible()
{
  QWidget* w = widget();
  if (w)
    return w->isVisible();
  return false;
}

void TraceItemView::setData(TraceData* d)
{
  _newData = d;

  // invalidate all pointers to old data
  _activeItem = _newActiveItem = 0;
  _selectedItem = _newSelectedItem = 0;
  _eventType = _newEventType = 0;
  _eventType2 = _newEventType2 = 0;
  _groupType = _newGroupType = TraceItem::NoCostType;
  _partList.clear();
  _newPartList.clear();

  // updateView will change this to dataChanged
  _status = nothingChanged;
}

void TraceItemView::updateView(bool force)
{
  if (!force && !isViewVisible()) return;

  if (_newData != _data) {
    _status |= dataChanged;
    _data = _newData;
  }
  else {
    _status &= ~dataChanged;

    // if there is no data change and data is 0, no update needed
    if (!_data) return;
  }

  if (!(_newPartList == _partList)) {
    _status |= partsChanged;
    _partList = _newPartList;
  }
  else
    _status &= ~partsChanged;

  if (_newActiveItem != _activeItem) {

      // when setting a new active item, there is no selection
      _selectedItem = 0;

      _status |= activeItemChanged;
      _activeItem = _newActiveItem;
  }
  else
    _status &= ~activeItemChanged;

  if (_newEventType != _eventType) {
    _status |= eventTypeChanged;
    _eventType = _newEventType;
  }
  else
    _status &= ~eventTypeChanged;

  if (_newEventType2 != _eventType2) {
    _status |= eventType2Changed;
    _eventType2 = _newEventType2;
  }
  else
    _status &= ~eventType2Changed;

  if (_newGroupType != _groupType) {
    _status |= groupTypeChanged;
    _groupType = _newGroupType;
  }
  else
    _status &= ~groupTypeChanged;


  if (_newSelectedItem != _selectedItem) {
    _status |= selectedItemChanged;
    _selectedItem = _newSelectedItem;
  }
  else
    _status &= ~selectedItemChanged;


  if (!force && (_status == nothingChanged)) return;

#if TRACE_UPDATES
  kDebug() << (widget() ? widget()->name() : "TraceItemView")
            << "::doUpdate ( "
            << ((_status & dataChanged) ? "data ":"")
            << ((_status & configChanged) ? "config ":"")
            << ")" << endl;

  if (_status & partsChanged)
    kDebug() << "  Part List "
              << _partList.names()
              << endl;

  if (_status & eventTypeChanged)
    kDebug() << "  Cost type "
              << (_eventType ? qPrintable( _eventType->name() ) : "?")
              << endl;

  if (_status & eventType2Changed)
    kDebug() << "  Cost type 2 "
              << (_eventType2 ? qPrintable( _eventType2->name() ) : "?")
              << endl;

  if (_status & groupTypeChanged)
    kDebug() << "  Group type "
              << TraceItem::typeName(_groupType)
              << endl;

  if (_status & activeItemChanged)
    kDebug() << "  Active: "
              << (_activeItem ? qPrintable( _activeItem->fullName() ) : "?")
              << endl;

  if (_status & selectedItemChanged)
    kDebug() << "  Selected: "
              << (_selectedItem ? qPrintable( _selectedItem->fullName() ) : "?")
              << endl;
#endif

  int st = _status;
  _status = nothingChanged;
  doUpdate(st);
  return;

  if (_inUpdate) return;
  _inUpdate = true;
  doUpdate(_status);
  _inUpdate = false;
}


void TraceItemView::selected(TraceItemView* /*sender*/, TraceItem* i)
{
#if TRACE_UPDATES
  kDebug() << (widget() ? widget()->name() : "TraceItemView")
            << "::selected "
            << (i ? qPrintable( i->name() ): "(nil)")
            << ", sender "
            << sender->widget()->name() << endl;
#endif

  if (_parentView) _parentView->selected(this, i);
}

void TraceItemView::partsSelected(TraceItemView* /*sender*/, const TracePartList& l)
{
#if TRACE_UPDATES
  kDebug() << (widget() ? widget()->name() : "TraceItemView")
            << "::selected "
            << l.names()
            << ", sender "
            << sender->widget()->name() << endl;
#endif

  if (_parentView)
    _parentView->partsSelected(this, l);
  else
    if (_topLevel) _topLevel->activePartsChangedSlot(l);
}

void TraceItemView::activated(TraceItemView* /*sender*/, TraceItem* i)
{
#if TRACE_UPDATES
  kDebug() << (widget() ? widget()->name() : "TraceItemView")
            << "::activated "
            << (i ? qPrintable( i->name() ) : "(nil)")
            << ", sender "
            << sender->widget()->name() << endl;
#endif

  if (_parentView)
      _parentView->activated(this, i);
  else
      if (_topLevel) _topLevel->setTraceItemDelayed(i);
}

void TraceItemView::selectedEventType(TraceItemView*, TraceEventType* t)
{
  if (_parentView)
      _parentView->selectedEventType(this, t);
  else
      if (_topLevel) _topLevel->setEventTypeDelayed(t);
}

void TraceItemView::selectedEventType2(TraceItemView*, TraceEventType* t)
{
  if (_parentView)
      _parentView->selectedEventType2(this, t);
  else
      if (_topLevel) _topLevel->setEventType2Delayed(t);
}

void TraceItemView::directionActivated(TraceItemView*, TraceItemView::Direction d)
{
  if (_parentView)
      _parentView->directionActivated(this, d);
  else
      if (_topLevel) _topLevel->setDirectionDelayed(d);
}

void TraceItemView::doUpdate(int)
{
}

void TraceItemView::selected(TraceItem* i)
{
  if (_parentView)
      _parentView->selected(this, i);

}

void TraceItemView::partsSelected(const TracePartList& l)
{
  if (_parentView)
      _parentView->partsSelected(this, l);
  else
      if (_topLevel) _topLevel->activePartsChangedSlot(l);
}

void TraceItemView::activated(TraceItem* i)
{
#if TRACE_UPDATES
  kDebug() << (widget() ? widget()->name() : "TraceItemView")
            << "::activated "
            << (i ? qPrintable( i->name() ): "(nil)") << endl;
#endif

  if (_parentView)
      _parentView->activated(this, i);
  else
      if (_topLevel) _topLevel->setTraceItemDelayed(i);
}

void TraceItemView::selectedEventType(TraceEventType* t)
{
  if (_parentView)
      _parentView->selectedEventType(this, t);
  else
      if (_topLevel) _topLevel->setEventTypeDelayed(t);
}

void TraceItemView::selectedEventType2(TraceEventType* t)
{
  if (_parentView)
      _parentView->selectedEventType2(this, t);
  else
      if (_topLevel) _topLevel->setEventType2Delayed(t);
}

void TraceItemView::directionActivated(TraceItemView::Direction d)
{
  if (_parentView)
      _parentView->directionActivated(this, d);
  else
      if (_topLevel) _topLevel->setDirectionDelayed(d);
}

void TraceItemView::addEventTypeMenu(QMenu* p, bool withCost2)
{
  if (_topLevel) _topLevel->addEventTypeMenu(p, withCost2);
}

void TraceItemView::addGoMenu(QMenu* p)
{
  if (_topLevel) _topLevel->addGoMenu(p);
}