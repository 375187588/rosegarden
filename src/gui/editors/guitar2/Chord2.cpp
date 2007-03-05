/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Chord2.h"
#include "base/Event.h"

#include <qstring.h>

namespace Rosegarden
{

const std::string Chord2::EventType              = "guitarchord";
const short Chord2::EventSubOrdering             = -60;
const PropertyName Chord2::RootPropertyName      = "root";
const PropertyName Chord2::ExtPropertyName       = "ext";
const PropertyName Chord2::FingeringPropertyName = "fingering";


Chord2::Chord2()
 : m_selectedFingeringIdx(-1)
{
}

Chord2::Chord2(const QString& root, const QString& ext)
    : m_root(root),
      m_ext(ext),
      m_selectedFingeringIdx(-1)
{
}

Chord2::Chord2(const Event& e)
{
    std::string f;
    bool ok;

    ok = e.get<String>(RootPropertyName, f);
    if (ok)
        m_root = f;

    ok = e.get<String>(ExtPropertyName, f);
    if (ok)
        m_ext = f;

    ok = e.get<String>(FingeringPropertyName, f);
    if (ok) {
        QString qf(f);
        QString errString;
    
        Fingering2 fingering = Fingering2::parseFingering(qf, errString);    
        addFingering(fingering);
    }
}

Event* Chord2::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(RootPropertyName, m_root);
    e->set<String>(ExtPropertyName, m_ext);
    e->set<String>(FingeringPropertyName, getSelectedFingering().toString());
    return e;
}

void Chord2::setSelectedFingeringIdx(int i)
{
    m_selectedFingeringIdx = std::min(i, int(m_fingerings.size()) - 1);
}

void Chord2::setFingering(unsigned int idx, Fingering2 f)
{
    m_fingerings[idx] = f;

    if (m_selectedFingeringIdx < 0)
        m_selectedFingeringIdx = 0;
}

void Chord2::addFingering(Fingering2 f)
{
    m_fingerings.push_back(f);

    if (m_selectedFingeringIdx < 0)
        m_selectedFingeringIdx = 0;
}

void Chord2::removeFingering(unsigned int idx)
{
    std::vector<Fingering2>::iterator i = m_fingerings.begin();
    i += idx;
    m_fingerings.erase(i);
}

bool operator<(const Chord2& a, const Chord2& b)
{
    if (a.m_root != b.m_root) {
        return a.m_root < b.m_root;
    } else if (a.m_ext != b.m_ext) {
        if (a.m_ext.isEmpty()) // chords with no ext need to be stored first
            return true;
        if (b.m_ext.isEmpty())
            return false;
        return a.m_ext < b.m_ext;
    }

    return false;
}

}
