/***************************************************************************
                          rosegardenguiview.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 23:41:03 CEST 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdio>
#include <vector>
#include <iostream>
#include <sys/times.h>

// include files for Qt
#include <qprinter.h>
#include <qpainter.h>
#include <qcanvas.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>

// application specific includes
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "qcanvasspritegroupable.h"
#include "qcanvaslinegroupable.h"
#include "pitchtoheight.h"
#include "staff.h"
#include "chord.h"
#include "notepixmapfactory.h"

RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char *name)
    : QCanvasView(new QCanvas(parent->width(), parent->height()),
                  parent, name),
    m_movingItem(0),
    m_draggingItem(false),
    m_hlayout(0),
    m_vlayout(0)
{

    setBackgroundMode(PaletteBase);


}

RosegardenGUIView::~RosegardenGUIView()
{
}

RosegardenGUIDoc *RosegardenGUIView::getDocument() const
{
    RosegardenGUIApp *theApp=(RosegardenGUIApp *) parentWidget();

    return theApp->getDocument();
}

void RosegardenGUIView::print(QPrinter *pPrinter)
{
    QPainter printpainter;
    printpainter.begin(pPrinter);
	
    // TODO: add your printing code here

    printpainter.end();
}


void
RosegardenGUIView::contentsMouseReleaseEvent (QMouseEvent *e)
{
    m_draggingItem = false;
    canvas()->update();
}

void
RosegardenGUIView::contentsMouseMoveEvent (QMouseEvent *e)
{
    if(m_draggingItem) {
        m_movingItem->move(e->x(), e->y());
        canvas()->update();
    }
}

void
RosegardenGUIView::contentsMousePressEvent (QMouseEvent *e)
{
    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty()) { // click was not on an item
        return;
    }

    QCanvasItem *item = itemList.first();

    QCanvasGroupableItem *gitem;

    if((gitem = dynamic_cast<QCanvasGroupableItem*>(item))) {
        kdDebug(KDEBUG_AREA) << "Groupable item" << endl;
        QCanvasItemGroup *t = gitem->group();

        if(t->active())
            m_movingItem = t;
        else {
            kdDebug(KDEBUG_AREA) << "Unmoveable groupable item" << endl;
            m_movingItem = 0; // this is not a moveable item
            return;
        }
    } else {
        m_movingItem = item;
    }

    m_draggingItem = true;
    m_movingItem->move(e->x(), e->y());
    canvas()->update();
}



void
RosegardenGUIView::perfTest()
{
    // perf test - add many many notes
    clock_t st, et;
    struct tms spare;
    st = times(&spare);


    cout << "Adding 1000 notes" << endl;
    setUpdatesEnabled(false);

    QCanvasPixmapArray *notePixmap = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");

    for(unsigned int x = 0; x < 1000; ++x) {
        for(unsigned int y = 0; y < 100; ++y) {


            QCanvasSprite *clef = new QCanvasSprite(notePixmap, canvas());

            clef->move(x * 10, y * 10);
        }
    }
    setUpdatesEnabled(true);

    cout << "Done adding 1000 notes" << endl;
    et = times(&spare);

    cout << (et-st)*10 << "ms" << endl;
}

void
RosegardenGUIView::test()
{
    //     QCanvasEllipse *t = new QCanvasEllipse(10, 10, canvas());
    //     t->setX(50);
    //     t->setY(50);

    //     QBrush brush(blue);
    //     t->setBrush(brush);

    Staff *staff = new Staff(canvas());
    staff->move(20, 15);
		
    staff = new Staff(canvas());
    staff->move(20, 130);
		
    // Add some notes

    QCanvasPixmapArray *notePixmap = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");

    const vector<int>& pitchToHeight(PitchToHeight::instance());

    for(unsigned int i = 0; i < pitchToHeight.size(); ++i) {
        QCanvasSprite *note = new QCanvasSprite(notePixmap, canvas());
        note->move(20,14);
        note->moveBy(40 + i * 20, pitchToHeight[i]);
    }

    //     Chord *chord = new Chord(canvas());
    //     chord->addNote(0);
    //     chord->addNote(4);
    //     chord->addNote(6);

    //     chord->move(50,50);


    NotePixmapFactory npf;
#if 0
    for(unsigned int i = 0; i < 7; ++i) {


        QPixmap note(npf.makeNotePixmap(i, true, true));

        QList<QPixmap> pixlist;

        pixlist.append(&note);

        QList<QPoint> spotlist;
        spotlist.append(new QPoint(0,0));

        QCanvasPixmapArray *notePixmap2 = new QCanvasPixmapArray(pixlist,
                                                                 spotlist);

        QCanvasSprite *noteSprite = new QCanvasSprite(notePixmap2, canvas());

        noteSprite->move(50 + i * 20, 100);

    }


    for(unsigned int i = 0; i < 7; ++i) {


        QPixmap note(npf.makeNotePixmap(i, true, false));

        QList<QPixmap> pixlist;

        pixlist.append(&note);

        QList<QPoint> spotlist;
        spotlist.append(new QPoint(0,0));

        QCanvasPixmapArray *notePixmap2 = new QCanvasPixmapArray(pixlist,
                                                                 spotlist);

        QCanvasSprite *noteSprite = new QCanvasSprite(notePixmap2, canvas());

        noteSprite->move(50 + i * 20, 150);

    }
#endif

    chordpitches pitches;
    pitches.push_back(6); // something wrong here - pitches aren't in the right order
    pitches.push_back(4);
    pitches.push_back(0);

    QPixmap chord(npf.makeChordPixmap(pitches, 6, true, false));
    QList<QPixmap> pixlist;

    pixlist.append(&chord);

    QList<QPoint> spotlist;
    spotlist.append(new QPoint(0,0));

    QCanvasPixmapArray *chordPixmap = new QCanvasPixmapArray(pixlist,
                                                             spotlist);

    QCanvasSprite *chordSprite = new QCanvasSprite(chordPixmap, canvas());

    chordSprite->move(50, 50);
   
}


bool
RosegardenGUIView::applyLayout()
{
    bool rch = applyHorizontalLayout();
    bool rcv = applyVerticalLayout();

    return rch && rcv;
}


bool
RosegardenGUIView::applyHorizontalLayout()
{
    if (!m_hlayout) {
        KMessageBox::error(0, "No Horizontal Layout engine");
        return false;
    }

    RosegardenGUIDoc::ElementList& elements(getDocument()->getElements());
    
    for_each(elements.begin(), elements.end(), *m_hlayout);
    
    return m_hlayout->status();
}


bool 
RosegardenGUIView::applyVerticalLayout()
{
    if (!m_vlayout) {
        KMessageBox::error(0, "No Vertical Layout engine");
        return false;
    }

    RosegardenGUIDoc::ElementList& elements(getDocument()->getElements());
    
    for_each(elements.begin(), elements.end(), *m_vlayout);
    
    return m_vlayout->status();
}


LayoutEngine::LayoutEngine()
    : m_status(0)
{
}

LayoutEngine::~LayoutEngine()
{
}

NotationHLayout::NotationHLayout(unsigned int barWidth)
    : m_barWidth(barWidth),
      m_lastPos(0)
{
}

void
NotationHLayout::layout(Element2 *el)
{
    el->set<Int>("Notation::Y", m_lastPos + 15);
    m_lastPos += 15;
}

NotationVLayout::NotationVLayout()
{
}

void
NotationVLayout::layout(Element2 *el)
{
    el->set<Int>("Notation::X", 10);
}
