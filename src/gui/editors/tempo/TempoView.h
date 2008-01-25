
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#ifndef _RG_TEMPOVIEW_H_
#define _RG_TEMPOVIEW_H_

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/general/EditViewBase.h"
#include <qsize.h>
#include <qstring.h>
#include <vector>
#include "base/Event.h"


class QWidget;
class QListViewItem;
class QCloseEvent;
class QCheckBox;
class QButtonGroup;
class KListView;


namespace Rosegarden
{

class Segment;
class RosegardenGUIDoc;
class Composition;


/**
 * Tempo and time signature list-style editor.  This has some code
 * in common with EventView, but not enough to make them any more
 * shareable than simply through EditViewBase.  Hopefully this one
 * should prove considerably simpler, anyway.
 */

class TempoView : public EditViewBase, public CompositionObserver
{
    Q_OBJECT

    enum Filter
    {
        None          = 0x0000,
        Tempo         = 0x0001,
        TimeSignature = 0x0002
    };

public:
    TempoView(RosegardenGUIDoc *doc, QWidget *parent, timeT);
    virtual ~TempoView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0);

    virtual void updateView();

    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);

    // Set the button states to the current filter positions
    //
    void setButtonsToFilter();

    // Menu creation and show
    //
    void createMenu();

    // Composition Observer callbacks
    //
    virtual void timeSignatureChanged(const Composition *);
    virtual void tempoChanged(const Composition *);

signals:
    // forwarded from tempo dialog:
    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // tempo target
                     TempoDialog::TempoDialogAction); // tempo action

    void closing();

public slots:
    // standard slots
    virtual void slotEditCut();
    virtual void slotEditCopy();
    virtual void slotEditPaste();

    // other edit slots
    void slotEditDelete();
    void slotEditInsertTempo();
    void slotEditInsertTimeSignature();
    void slotEdit();

    void slotSelectAll();
    void slotClearSelection();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    // on double click on the event list
    //
    void slotPopupEditor(QListViewItem*);

    // Change filter parameters
    //
    void slotModifyFilter(int);

protected slots:

    virtual void slotSaveOptions();

protected:

    virtual void readOptions();
    void makeInitialSelection(timeT);
    QString makeTimeString(timeT time, int timeMode);
    virtual Segment *getCurrentSegment();
    virtual void updateViewCaption();

    virtual void closeEvent(QCloseEvent *);

    //--------------- Data members ---------------------------------
    KListView   *m_list;
    int          m_filter;

    static int   m_lastSetFilter;

    QButtonGroup   *m_filterGroup;
    QCheckBox      *m_tempoCheckBox;
    QCheckBox      *m_timeSigCheckBox;

    std::vector<int> m_listSelection;

    bool m_ignoreUpdates;
};



}

#endif
