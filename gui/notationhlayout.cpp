// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "notationhlayout.h"
#include "notationstaff.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"
#include "Quantizer.h"
#include "Composition.h"
#include "SegmentNotationHelper.h"

#include <cmath> // for fabs()

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

std::vector<double> NotationHLayout::m_availableSpacings;


static int rwms;
static int rwc;

static int nwms;
static int nwc;

static int scms;
static int scc;

static int srms;
static int src;

static int abms;
static int abc;

NotationHLayout::NotationHLayout(Composition *c, NotePixmapFactory *npf) :
    Rosegarden::HorizontalLayoutEngine<NotationElement>(c),
    m_totalWidth(0.),
    m_pageMode(false),
    m_pageWidth(0.),
    m_spacing(1.0),
    m_npf(npf)
{
//    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

std::vector<double>
NotationHLayout::getAvailableSpacings()
{
    if (m_availableSpacings.size() == 0) {
        m_availableSpacings.push_back(0.3);
        m_availableSpacings.push_back(0.6);
        m_availableSpacings.push_back(0.85);
        m_availableSpacings.push_back(1.0);
        m_availableSpacings.push_back(1.3);
        m_availableSpacings.push_back(1.7);
        m_availableSpacings.push_back(2.2);
    }
    return m_availableSpacings;
}

const Quantizer *
NotationHLayout::getQuantizer() const
{
    return getComposition()->getLegatoQuantizer();
}

NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
	m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}


const NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff) const
{
    return ((NotationHLayout *)this)->getBarData(staff);
}


// To find the "ideal" width of a bar, we need the sum of the minimum
// widths of the elements in the bar, plus as much extra space as
// would be needed if the bar was made up entirely of repetitions of
// its shortest note.  This space is the product of the number of the
// bar's shortest notes that will fit in the duration of the bar and
// the comfortable gap for each.

// Some ground rules for "ideal" layout:
// 
// -- The shortest notes in the bar need to have a certain amount of
//    space, but if they're _very_ short compared to the total bar
//    duration then we can probably afford to squash them somewhat
// 
// -- If there are lots of the shortest note duration, then we
//    should try not to squash them quite so much
// 
// -- If there are not very many notes in the bar altogether, we can
//    squash things up a bit more perhaps
// 
// -- Similarly if they're dotted, we need space for the dots; we
//    can't risk making the dots invisible
// 
// -- In theory we don't necessarily want the whole bar width to be
//    the product of the shortest-note width and the number of shortest
//    notes in the bar.  But it's difficult to plan the spacing
//    otherwise.  One possibility is to augment the fixedWidth with a
//    certain proportion of the width of each note, and make that a
//    higher proportion for very short notes than for long notes.

//!!! The algorithm below does not implement most of these rules; it
// can probably be improved dramatically without too much work

double NotationHLayout::getIdealBarWidth(StaffType &staff,
					 int fixedWidth,
					 int baseWidth,
					 NotationElementList::iterator shortest,
					 int shortCount,
					 int /*totalCount*/,
					 const TimeSignature &timeSignature)
    const
{
//    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: shortCount is "
//                         << shortCount << ", fixedWidth is "
//                         << fixedWidth << ", barDuration is "
//                         << timeSignature.getBarDuration() << endl;

    if (shortest == staff.getViewElementList()->end()) {
//        kdDebug(KDEBUG_AREA) << "First trivial return" << endl;
        return fixedWidth;
    }

    int d = getQuantizer()->getQuantizedDuration((*shortest)->event());

    if (d == 0) {
//        kdDebug(KDEBUG_AREA) << "Second trivial return" << endl;
        return fixedWidth;
    }

    int smin = getMinWidth(**shortest);
    if (!(*shortest)->event()->get<Int>(NOTE_DOTS)) {
        smin += m_npf->getDotWidth()/2;
    }

    /* if there aren't many of the shortest notes, we don't want to
       allow so much space to accommodate them */
    if (shortCount < 3) smin -= 3 - shortCount;

    int gapPer = 
        getComfortableGap((*shortest)->event()->get<Int>(NOTE_TYPE)) +
        smin;

//    kdDebug(KDEBUG_AREA) << "d is " << d << ", gapPer is " << gapPer << endl;

    double w = fixedWidth + ((timeSignature.getBarDuration() * gapPer) / d);

//    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: returning "
//                         << w << endl;

    w *= m_spacing;
    if (w < (fixedWidth + baseWidth)) w = (double)(fixedWidth + baseWidth);
    return w;
} 

NotationElementList::iterator
NotationHLayout::getStartOfQuantizedSlice(const NotationElementList *notes,
					  timeT t)
    const
{
    NotationElementList::iterator i = notes->findTime(t);
    NotationElementList::iterator j(i);

    while (true) {
	if (i == notes->begin()) return i;
	--j;
	if (getQuantizer()->getQuantizedAbsoluteTime((*j)->event()) < t) {
	    return i;
	}
	i = j;
    }
}


void
NotationHLayout::scanStaff(StaffType &staff, timeT startTime, timeT endTime)
{
    START_TIMING;

    rwms = rwc = nwms = nwc = scc = scms = src = srms = abc = abms = 0;

    Segment &t(staff.getSegment());
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    bool isFullScan = (startTime == endTime);

    bool allDone = false; // used in partial scans

    if (isFullScan) {
	barList.clear();
	startTime = t.getStartTime();
	endTime = t.getEndTime();
    }

    int barNo = getComposition()->getBarNumber(t.getStartTime());
    int endBarNo = getComposition()->getBarNumber(t.getEndTime());
    int barCounter = 0;

    SegmentNotationHelper nh(t);
    nh.quantize();

    PRINT_ELAPSED("NotationHLayout::scanStaff: after quantize");

    BarDataList &bdl(m_barData[&staff]);
    int fakeBarCount = m_fakeBarCountMap[&staff];

    // because addNewBar/setNewBar normally makes changes to the next bar,
    // the first real bar has to be a special case
    if (isFullScan) {
        addNewBar(staff, barCounter, notes->begin(), 0, 0, 0, true, 0, 0);
    } else {
        bdl[fakeBarCount].start = notes->begin();
        bdl[fakeBarCount].barNo = 0;
        bdl[fakeBarCount].correct = true;
        bdl[fakeBarCount].needsLayout = true;
    }

    ++barCounter;

    while (barNo <= endBarNo) {

	std::pair<timeT, timeT> barTimes =
	    getComposition()->getBarRange(barNo);

        NotationElementList::iterator to =
	    getStartOfQuantizedSlice(notes, barTimes.second);

        if (!isFullScan) {
            // Full scans set some things in the bar data list during
            // addNewBar and the BarDataList constructor. Since partial
            // scans don't get to do that, they do this:
            bdl[barNo].barNo = barCounter - 1;
            if (barTimes.second >= startTime) {
		bdl[barNo].needsLayout = true;
	    }
            if (allDone) {
		// Need to prevent subsequent layout calls from using
		// stale iterator data from this scan (a bit paranoid)
		if (barNo < bdl.size()) {
		    bdl[barNo].start = to;
		} else {
		    bdl.push_back(BarData(barNo, to, true));
		}
                ++barCounter;
                ++barNo;
                continue;
            }
        }

        NotationElementList::iterator from = 
	    getStartOfQuantizedSlice(notes, barTimes.first);

//	kdDebug(KDEBUG_AREA) << "NotationHLayout::scanStaff: bar " << barNo << ", from " << barTimes.first << ", to " << barTimes.second << " (end " << t.getEndTime() << ")" << endl;

        NotationElementList::iterator shortest = notes->end();
        int shortCount = 0;
        int totalCount = 0;

        // fixedWidth includes clefs, keys &c, but also accidentals
	int fixedWidth = getBarMargin();

        // baseWidth is absolute minimum width of non-fixedWidth elements
        int baseWidth = 0;

        timeT apparentBarDuration = 0;
	timeT actualBarEnd = barTimes.first;

	AccidentalTable accTable(key, clef);

	Event *timeSigEvent = 0;
	bool newTimeSig = false;
	timeSignature = getComposition()->getTimeSignatureInBar(barNo, newTimeSig);

	if (newTimeSig && !timeSignature.isHidden()) {
	    timeSigEvent = timeSignature.getAsEvent(barTimes.first);
	    fixedWidth += getFixedItemSpacing() +
		m_npf->getTimeSigWidth(timeSignature);
	}

        for (NotationElementList::iterator itr = from; itr != to; ++itr) {
        
            NotationElement *el = (*itr);
            int mw = getMinWidth(*el);

            if (el->event()->isa(Clef::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found clef" << endl;

                fixedWidth += mw;
                clef = Clef(*el->event());

                //!!! Probably not strictly the right thing to do
                // here, but I hope it'll do well enough in practice
                accTable = AccidentalTable(key, clef);

            } else if (el->event()->isa(Key::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found key" << endl;

                fixedWidth += mw;
                key = Key(*el->event());

                accTable = AccidentalTable(key, clef);
		
            } else if (!isFullScan &&
		       (barTimes.second < startTime ||
			barTimes.first > endTime)) {

		// partial scans don't need to look at all notes, just clefs
		// and key changes

                // !!! shouldn't have to do all this to find clef and key
                // (and I don't bother to do this in layout() -- eek)
                if (barTimes.first > endTime) allDone = true;
                continue;

	    } else if (el->isNote()) {

		++totalCount;
		
		clock_t c = clock();

		apparentBarDuration +=
		    scanChord(notes, itr, clef, key, accTable,
			      fixedWidth, baseWidth, shortest, shortCount, to);

	++scc;
	scms += (clock() - c) * 1000 / CLOCKS_PER_SEC;

	    } else if (el->isRest()) {

		++totalCount;

clock_t c = clock();

		apparentBarDuration +=
		    scanRest(notes, itr,
			     fixedWidth, baseWidth, shortest, shortCount);

	++src;
	srms += (clock() - c) * 1000 / CLOCKS_PER_SEC;

	    } else if (el->event()->isa(Indication::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found indication" << endl;

		mw = 0;

	    } else {
		
		kdDebug(KDEBUG_AREA) << "Found something I don't know about" << endl;
		fixedWidth += mw;
	    }

	    actualBarEnd = el->getAbsoluteTime() + el->getDuration();
            el->event()->setMaybe<Int>(MIN_WIDTH, mw);
	}

	if (actualBarEnd == barTimes.first) actualBarEnd = barTimes.second;

	clock_t c = clock();

	if (isFullScan) { 
	    addNewBar(staff, barCounter, to,
		      getIdealBarWidth(staff, fixedWidth, baseWidth, shortest, 
				       shortCount, totalCount, timeSignature),
		      fixedWidth, baseWidth,
		      apparentBarDuration == timeSignature.getBarDuration(),
		      timeSigEvent, actualBarEnd - barTimes.first); 

	    ++abc;
	    abms += (clock() - c) * 1000 / CLOCKS_PER_SEC;
	} else {
	    
    	    if (!(barTimes.second < startTime || barTimes.first > endTime)) {
		setBar(staff, barNo, barCounter, to,
                       getIdealBarWidth(staff, fixedWidth, baseWidth, shortest,
                                        shortCount, totalCount, timeSignature),
                       fixedWidth, baseWidth,
                       apparentBarDuration == timeSignature.getBarDuration(),
                       timeSigEvent, actualBarEnd - barTimes.first);
	    } else {
		// If we don't do this, we have a potential problem when
		// something is modified at the start of a bar: the start-
		// iterator that was set by a previous scan won't get
		// reset in this scan because it's "associated" with the
		// previous bar of the scan.  This is a defect arising
		// from the (bad) decision to update both the given and
		// the following bar in addNewBar/setBar.  It all needs to
		// be rationalised rather.
		if (barNo < bdl.size()-1) {
		    bdl[barNo+1].start = to;
		} else {
		    bdl.push_back(BarData(barCounter, to, true));
		}
	    }
        }

	++barCounter;
	++barNo;
    }

    PRINT_ELAPSED("NotationHLayout::scanStaff");

    if (nwc == 0) nwc = 1;
    if (rwc == 0) rwc = 1;
    if (src == 0) src = 1;
    if (scc == 0) scc = 1;
    if (abc == 0) abc = 1;

    kdDebug(KDEBUG_AREA) << "Calls to getNoteBodyWidth: " << nwc
			 << " (total " << nwms << "ms, average "
			 << (nwms/nwc) << "ms)" << endl;

    kdDebug(KDEBUG_AREA) << "Calls to getRestWidth: " << rwc
			 << " (total " << rwms << "ms, average "
			 << (rwms/rwc) << "ms)" << endl;

    kdDebug(KDEBUG_AREA) << "Calls to scanChord: " << scc
			 << " (total " << scms << "ms, average "
			 << (scms/scc) << "ms)" << endl;

    kdDebug(KDEBUG_AREA) << "Calls to scanRest: " << src
			 << " (total " << srms << "ms, average "
			 << (srms/src) << "ms)" << endl;

    kdDebug(KDEBUG_AREA) << "Calls to addNewBar: " << abc
			 << " (total " << abms << "ms, average "
			 << (abms/abc) << "ms)" << endl;

}

//!!! probably better to split this (and setBar) into methods that deal
// with end-of-this-bar and start-of-next-bar separately?  also we have
// significant confusion over difference between barNo and barCounter
// in scanStaff
void
NotationHLayout::addNewBar(StaffType &staff,
			   int barCounter, NotationElementList::iterator i,
                           double width, int fwidth, int bwidth,
			   bool correct, Event *timeSig, timeT actualDuration)
{
    BarDataList &bdl(m_barData[&staff]);

    int s = bdl.size() - 1;
    if (s >= 0) {
        bdl[s].idealWidth = width;
        bdl[s].fixedWidth = fwidth;
        bdl[s].baseWidth = bwidth;
	bdl[s].timeSignature = timeSig;
	bdl[s].actualDuration = actualDuration;
    }

//    if (timeSig) kdDebug(KDEBUG_AREA) << "Adding bar with timesig" << endl;
//    else kdDebug(KDEBUG_AREA) << "Adding bar without timesig" << endl;

    bdl.push_back(BarData(barCounter, i, correct));
}


void
NotationHLayout::setBar(StaffType &staff,
			int barNo, int barCounter,
			NotationElementList::iterator i,
			double width, int fwidth, int bwidth,
			bool correct, Event *timeSig, timeT actualDuration)
{
    BarDataList &bdl(m_barData[&staff]);
    assert(barNo < bdl.size() - 1);

    bdl[barNo].idealWidth = width;
    bdl[barNo].fixedWidth = fwidth;
    bdl[barNo].baseWidth = bwidth;
    bdl[barNo].timeSignature = timeSig;
    bdl[barNo].actualDuration = actualDuration;

    bdl[barNo+1].barNo = barCounter;
    bdl[barNo+1].correct = correct;
    bdl[barNo+1].start = i;
}
 
timeT
NotationHLayout::scanChord(NotationElementList *notes,
			   NotationElementList::iterator &itr,
			   const Rosegarden::Clef &clef,
			   const Rosegarden::Key &key,
			   AccidentalTable &accTable,
			   int &fixedWidth, int &baseWidth,
			   NotationElementList::iterator &shortest,
			   int &shortCount,
			   NotationElementList::iterator &to)
{
    Chord chord(*notes, itr, getQuantizer());
    AccidentalTable newAccTable(accTable);
    Accidental someAccidental = NoAccidental;
    bool barEndsInChord = false;

    for (unsigned int i = 0; i < chord.size(); ++i) {
	
	NotationElement *el = *chord[i];
	
	long pitch = 64;
	if (!el->event()->get<Int>(PITCH, pitch)) {
	    kdDebug(KDEBUG_AREA) <<
		"WARNING: NotationHLayout::scanChord: couldn't get pitch for element, using default pitch of " << pitch << endl;
	}

	Accidental explicitAccidental = NoAccidental;
	(void)el->event()->get<String>(ACCIDENTAL, explicitAccidental);
	
	Rosegarden::NotationDisplayPitch p
	    (pitch, clef, key, explicitAccidental);
	int h = p.getHeightOnStaff();
	Accidental acc = p.getAccidental();

	el->event()->setMaybe<Int>(HEIGHT_ON_STAFF, h);
	el->event()->setMaybe<String>(CALCULATED_ACCIDENTAL, acc);

	// update display acc for note according to the accTable
	// (accidentals in force when the last chord ended) and update
	// newAccTable with accidentals from this note.  (We don't
	// update accTable yet because there may be other notes in
	// this chord that need accTable to be the same as it is for
	// this one)
                    
	Accidental dacc = accTable.getDisplayAccidental(acc, h);
	el->event()->setMaybe<String>(DISPLAY_ACCIDENTAL, dacc);
	if (someAccidental == NoAccidental) someAccidental = dacc;

	newAccTable.update(acc, h);
	if (chord[i] == to) barEndsInChord = true;
    }

    accTable.copyFrom(newAccTable);

    if (someAccidental != NoAccidental) {
	fixedWidth += m_npf->getAccidentalWidth(someAccidental);
    }

    if (chord.hasNoteHeadShifted()) {
	fixedWidth += m_npf->getNoteBodyWidth();
    }

    NotationElementList::iterator myShortest = chord.getShortestElement();

    timeT d = getQuantizer()->getQuantizedDuration((*myShortest)->event());
    baseWidth += getMinWidth(**myShortest);

    timeT sd = 0;
    if (shortest != notes->end()) {
	sd = getQuantizer()->getQuantizedDuration((*shortest)->event());
    }

    if (d > 0 && (sd == 0 || d <= sd)) {
	if (d == sd) {
	    ++shortCount;
	} else {
	    shortest = myShortest;
	    shortCount = 1;
	}
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
    return d;
}

timeT
NotationHLayout::scanRest
(NotationElementList *notes, NotationElementList::iterator &itr,
 int &, int &baseWidth,
 NotationElementList::iterator &shortest, int &shortCount)
{
    timeT d = getQuantizer()->getQuantizedDuration((*itr)->event());
    baseWidth += getMinWidth(**itr);

    timeT sd = 0;
    if (shortest != notes->end()) {
	sd = getQuantizer()->getQuantizedDuration((*shortest)->event());
    }

    if (d > 0 && (sd == 0 || d <= sd)) {

	// assumption: rests are wider than notes
	if (sd == 0 || d < sd || (*shortest)->isNote()) {
	    shortest = itr;
	    if (d != sd) shortCount = 1;
	    else ++shortCount;
	} else {
	    ++shortCount;
	}
    }

    return d;
}


void
NotationHLayout::fillFakeBars()
{
    START_TIMING;

    // Scoot through the staffs, prepending fake bars to the start of
    // the bar list for any staff that doesn't begin right at the
    // start of the composition

    BarDataMap::iterator i;

    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

        StaffType *staff = i->first;
        BarDataList &list = i->second;

        if (list.size() > 0 && list[0].barNo < 0) continue; // done it already

        Segment &segment = staff->getSegment();

	int b = 0;
	// no initialization (saving b for after the loop)

	for ( ; b < getComposition()->getNbBars(); ++b) {

	    if (getComposition()->getBarRange(b).first
		>= segment.getStartTime()) break;

            list.push_front(BarData(-1, staff->getViewElementList()->end()));
        }

	m_fakeBarCountMap[staff] = b;
    }
    
    PRINT_ELAPSED("NotationHLayout::fillFakeBars");
}    


NotationHLayout::StaffType *
NotationHLayout::getStaffWithWidestBar(int barNo)
{
    double maxWidth = -1;
    StaffType *widest = 0;
    BarDataMap::iterator i;

    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	BarDataList &list = i->second;

	if ((int)list.size() > barNo) {
	    if (list[barNo].idealWidth > maxWidth) {
		maxWidth = list[barNo].idealWidth;
		widest = i->first;
	    }
	}
    }

    return widest;
}


void
NotationHLayout::reconcileBarsLinear()
{
    START_TIMING;

    BarDataMap::iterator i;

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff.

    int barNo = 0;
    bool aWidthChanged = false;

    for (;;) {

	StaffType *widest = getStaffWithWidestBar(barNo);

	if (!widest) break; // reached end of piece
	double maxWidth = m_barData[widest][barNo].idealWidth;

	// Now apply width to this bar on all staffs

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if (list.size() > barNo) {

		BarData &bd(list[barNo]);

		if (bd.idealWidth != maxWidth) {
		    if (bd.idealWidth > 0.0) {
			double ratio = maxWidth / bd.idealWidth;
			bd.fixedWidth +=
			    bd.fixedWidth * (int)((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
                    aWidthChanged = true;
		}

                if (aWidthChanged) bd.needsLayout = true;
	    }
	}

	++barNo;
    }

    PRINT_ELAPSED("NotationHLayout::reconcileBarsLinear");
}	


void
NotationHLayout::reconcileBarsPage()
{
    START_TIMING;

    unsigned int barNo = 0;
    unsigned int barNoThisRow = 0;
    
    // pair of the recommended number of bars with those bars'
    // original total width, for each row
    std::vector<std::pair<int, double> > rowData;

    double pageWidthSoFar = 0.0;
    double stretchFactor = 10.0;

    BarDataMap::iterator i;

    for (;;) {
	
	StaffType *widest = getStaffWithWidestBar(barNo);
	if (!widest) break; // reached end of piece
	double maxWidth = m_barData[widest][barNo].idealWidth;

	// Work on the assumption that this bar is the last in the
	// row.  How would that make things look?

	double nextPageWidth = pageWidthSoFar + maxWidth;
	double nextStretchFactor = m_pageWidth / nextPageWidth;

	// We have to have at least one bar per row
	
	bool tooFar = false;

	if (barNoThisRow >= 1) {

	    // If this stretch factor is "worse" than the previous
	    // one, we've come too far and have too many bars

	    if (fabs(1.0 - nextStretchFactor) > fabs(1.0 - stretchFactor)) {
		tooFar = true;
	    }

	    // If the next stretch factor is less than 1 and would
	    // make this bar on any of the staffs narrower than it can
	    // afford to be, then we've got too many bars

	    if (!tooFar && (nextStretchFactor < 1.0)) {

		for (i = m_barData.begin(); i != m_barData.end(); ++i) {

		    BarDataList &list = i->second;

		    if (list.size() > signed(barNo)) {
			BarData &bd(list[barNo]);
			if ((nextStretchFactor * bd.idealWidth) <
			    (double)(bd.fixedWidth + bd.baseWidth)) {
			    tooFar = true;
			    break;
			}
		    }
		}
	    }
	}

	if (tooFar) {
	    rowData.push_back(std::pair<int, double>(barNoThisRow,
						     pageWidthSoFar));
	    barNoThisRow = 1;
	    pageWidthSoFar = maxWidth;
	    stretchFactor = m_pageWidth / maxWidth;
	} else {
	    ++barNoThisRow;
	    pageWidthSoFar = nextPageWidth;
	    stretchFactor = nextStretchFactor;
	}

	++barNo;
    }

    if (barNoThisRow > 0) {
	rowData.push_back(std::pair<int, double>(barNoThisRow,
						 pageWidthSoFar));
    }

    // Now we need to actually apply the widths

    barNo = 0;

    for (unsigned int row = 0; row < rowData.size(); ++row) {

	barNoThisRow = barNo;
	unsigned int finalBarThisRow = barNo + rowData[row].first - 1;

	pageWidthSoFar = 0;
	stretchFactor = m_pageWidth / rowData[row].second;

	for (; barNoThisRow <= finalBarThisRow; ++barNoThisRow, ++barNo) {

	    bool finalRow = (row == rowData.size()-1);

	    StaffType *widest = getStaffWithWidestBar(barNo);
	    if (!widest) break; // reached end of piece (shouldn't happen)
	    if (finalRow && (stretchFactor > 1.0)) stretchFactor = 1.0;
	    double maxWidth = 
		(stretchFactor * m_barData[widest][barNo].idealWidth);

	    if (barNoThisRow == finalBarThisRow) {
		if (!finalRow ||
		    (maxWidth > (m_pageWidth - pageWidthSoFar))) {
		    maxWidth = m_pageWidth - pageWidthSoFar;
		}
	    }
	    
	    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

		BarDataList &list = i->second;

		if (list.size() > signed(barNo)) {

		    BarData &bd(list[barNo]);

		    bd.needsLayout = true;
		    if (bd.idealWidth > 0) {
			double ratio = maxWidth / bd.idealWidth;
			bd.fixedWidth +=
			    bd.fixedWidth * (int)((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
		}
	    }

	    pageWidthSoFar += maxWidth;
	}
    }

    PRINT_ELAPSED("NotationHLayout::reconcileBarsPage");
}


// and for once I swear things will still be good tomorrow

NotationHLayout::AccidentalTable::AccidentalTable(Key key, Clef clef) :
    m_key(key), m_clef(clef)
{
    std::vector<int> heights(key.getAccidentalHeights(clef));
    unsigned int i;

    for (i = 0; i < 7; ++i) m_accidentals[i] = NoAccidental;
    for (i = 0; i < heights.size(); ++i) {
        m_accidentals[Key::canonicalHeight(heights[i])] =
            (key.isSharp() ? Sharp : Flat);
    }
}

NotationHLayout::AccidentalTable::AccidentalTable(const AccidentalTable &t) :
    m_key(t.m_key), m_clef(t.m_clef)
{
    copyFrom(t);
}

NotationHLayout::AccidentalTable &
NotationHLayout::AccidentalTable::operator=(const AccidentalTable &t)
{
    if (&t != this) {
	m_key = t.m_key;
	m_clef = t.m_clef;
	copyFrom(t);
    }
    return *this;
}

Accidental
NotationHLayout::AccidentalTable::getDisplayAccidental(Accidental accidental,
                                                       int height) const
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "accidental = " << accidental << ", stored accidental at height " << height << " is " << (*this)[height] << endl;

    if (m_accidentals[height] != NoAccidental) {

        if (accidental == m_accidentals[height]) {
            return NoAccidental;
        } else if (accidental == NoAccidental || accidental == Natural) {
            return Natural;
        } else {
            //!!! aargh.  What we really want to do now is have two
            //accidentals shown: first a natural, then the one
            //required for the note.  But there's no scope for that in
            //our accidental structure (RG2.1 is superior here)
            return accidental;
        }
    } else {
        return accidental;
    }
}

void
NotationHLayout::AccidentalTable::update(Accidental accidental, int height)
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "updating height" << height << " from " << (*this)[height] << " to " << accidental << endl;


    //!!! again, we can't properly deal with the difficult case where
    //we already have an accidental at height but it's not the same
    //accidental

    m_accidentals[height] = accidental;
}

void
NotationHLayout::AccidentalTable::copyFrom(const AccidentalTable &t)
{
    for (int i = 0; i < 7; ++i) {
	m_accidentals[i] = t.m_accidentals[i];
    }
}

void
NotationHLayout::finishLayout(timeT startTime, timeT endTime)
{
    fillFakeBars();
    if (m_pageMode && (m_pageWidth > 0.1)) reconcileBarsPage();
    else reconcileBarsLinear();
    
    for (BarDataMap::iterator i(m_barData.begin()); i != m_barData.end(); ++i)
	layout(i, startTime, endTime);
}

void
NotationHLayout::layout(BarDataMap::iterator i, timeT startTime, timeT endTime)
{
    START_TIMING;

    StaffType &staff = *(i->first);
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    bool isFullLayout = (startTime == endTime);

    // these two are for partial layouts:
    bool haveSimpleOffset = false;
    double simpleOffset = 0;

    if (!isFullLayout) cerr << "NotationHLayout::layout: doing partial layout" << endl;

    double x = 0, barX = 0;
    TieMap tieMap;

    for (BarDataList::iterator bdi = barList.begin();
         bdi != barList.end(); ++bdi) {

        NotationElementList::iterator from = bdi->start;
        NotationElementList::iterator to;

//        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting barNo " << bdi->barNo << ", x = " << barX << ", width = " << bdi->idealWidth << ", time = " << (from == notes->end() ? -1 : (*from)->getAbsoluteTime()) << endl;

        BarDataList::iterator nbdi(bdi);
        if (++nbdi == barList.end()) {
            to = notes->end();
        } else {
            to = nbdi->start;
        }

        if (from == notes->end()) {
//            kdDebug(KDEBUG_AREA) << "Start is end" << endl;
        }
        if (from == to) {
//            kdDebug(KDEBUG_AREA) << "Start is to" << endl;
        }

        if ((!isFullLayout) &&
	    (from == notes->end() ||
	     (*from)->event()->getAbsoluteTime() > endTime)) {

            // Find how far to move everything if necessary
            if (!haveSimpleOffset) {
                simpleOffset = barX + getPreBarMargin() - bdi->x;
                haveSimpleOffset = true;
            }

            // Move all elements

            bdi->x += simpleOffset;
	    
            if (bdi->timeSignature)
                bdi->timeSigX += (int) simpleOffset;

            for (NotationElementList::iterator it = from;
		 it != notes->end() && it != to; ++it) {
                (*it)->setLayoutX((*it)->getLayoutX() + simpleOffset);
		double airX, airWidth;
		(*it)->getLayoutAirspace(airX, airWidth);
		(*it)->setLayoutAirspace(airX + simpleOffset, airWidth);
            }

            // And skip the real layout work
            bdi->needsLayout = false;
            continue;
        }

	x = barX;
	x += getPreBarMargin();
        bdi->x = x;
        x += getPostBarMargin();
	barX += bdi->idealWidth;

	bool timeSigToPlace = false;
	if (bdi->timeSignature) {
	    timeSignature = TimeSignature(*(bdi->timeSignature));
	    timeSigToPlace = true;
	}

        if (bdi->barNo < 0) { // fake bar
//            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): fake bar " << bdi->barNo << endl;
            continue;
        }
        if (!bdi->needsLayout) {
            //!!! clef and key may not be right
            // need a better way to find them than keeping track through the
            // whole staff
//            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): bar " << bdi->barNo << " has needsLayout false" << endl;
            continue;
        }

        if (timeSigToPlace) {
//	    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): there's a time sig in this bar" << endl;
	}


        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
//            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;
	    long delta;

	    if (timeSigToPlace && !el->event()->isa(Clef::EventType)) {
//		kdDebug(KDEBUG_AREA) << "Placing timesig at " << x << endl;
		bdi->timeSigX = (int)x;
		x += getFixedItemSpacing() +
		    m_npf->getTimeSigWidth(timeSignature);
//		kdDebug(KDEBUG_AREA) << "and moving next elt to " << x << endl;
		el->setLayoutX(x);
		timeSigToPlace = false;
	    }

	    if (el->isNote()) {

		// This modifies "it" and "tieMap"
		delta = positionChord
		    (staff, it, bdi, timeSignature, clef, key, tieMap, to);

	    } else if (el->isRest()) {

		delta = positionRest(staff, it, bdi, timeSignature);

	    } else if (el->event()->isa(Clef::EventType)) {
		
		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
//		kdDebug(KDEBUG_AREA) << "Found clef" << endl;
		clef = Clef(*el->event());

	    } else if (el->event()->isa(Key::EventType)) {

		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
//		kdDebug(KDEBUG_AREA) << "Found key" << endl;
		key = Key(*el->event());

	    } else {
		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
	    }

            x += delta;
        }

        bdi->needsLayout = false;
    }

    if (!isFullLayout && haveSimpleOffset) m_totalWidth += simpleOffset;
    else if (x > m_totalWidth) m_totalWidth = x;

    PRINT_ELAPSED("NotationHLayout::layout");
}


long
NotationHLayout::positionRest(StaffType &staff,
                              const NotationElementList::iterator &itr,
                              const BarDataList::iterator &bdi,
                              const TimeSignature &timeSignature)
{
    NotationElement *rest = *itr;

    // To work out how much space to allot a rest, as for a note,
    // start with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // (We use the actual duration of the bar, not the nominal time-
    // signature duration.)

    timeT barDuration = bdi->actualDuration;
    if (barDuration == 0) barDuration = timeSignature.getBarDuration();

    long delta = (((int)bdi->idealWidth - bdi->fixedWidth) *
		  getSpacingDuration(staff, itr)) /
	barDuration;
    rest->setLayoutAirspace(rest->getLayoutX(), delta);

    // Situate the rest somewhat further into its allotted space.  Not
    // convinced this is the right thing to do

    int baseWidth = m_npf->getRestWidth
	(Note(rest->event()->get<Int>(NOTE_TYPE),
	      rest->event()->get<Int>(NOTE_DOTS)));

    if (delta > 2 * baseWidth) {
        int shift = (delta - 2 * baseWidth) / 4;
        shift = std::min(shift, (baseWidth * 3));
        rest->setLayoutX(rest->getLayoutX() + shift);
    }
                
    return delta;
}


timeT
NotationHLayout::getSpacingDuration(StaffType &staff,
				    const NotationElementList::iterator &i)
{
    timeT t(getQuantizer()->getQuantizedAbsoluteTime((*i)->event()));
    timeT d(getQuantizer()->getQuantizedDuration((*i)->event()));

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e &&
	   getQuantizer()->getQuantizedAbsoluteTime((*j)->event()) == t) {
	++j;
    }
    if (j == e) return d;
    else return (*j)->getAbsoluteTime() - (*i)->getAbsoluteTime();
}


long
NotationHLayout::positionChord(StaffType &staff,
                               NotationElementList::iterator &itr,
			       const BarDataList::iterator &bdi,
			       const TimeSignature &timeSignature,
			       const Clef &clef, const Key &key,
			       TieMap &tieMap,
			       NotationElementList::iterator &to)
{
    Chord chord(*staff.getViewElementList(), itr, getQuantizer(), clef, key);
    double baseX = (*itr)->getLayoutX();

    // To work out how much space to allot a note (or chord), start
    // with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // (We use the actual duration of the bar, not the nominal time-
    // signature duration.)

    // In case this chord has various durations in it, we choose an
    // effective duration based on the absolute time of the first
    // following event not in the chord (see getSpacingDuration)

    timeT barDuration = bdi->actualDuration;
    if (barDuration == 0) barDuration = timeSignature.getBarDuration();

    long delta = (((int)bdi->idealWidth - bdi->fixedWidth) *
		  getSpacingDuration(staff, itr)) /
	barDuration;

    int noteWidth = m_npf->getNoteBodyWidth();

    // If the chord's allowed a lot of space, situate it somewhat
    // further into its allotted space.  Not convinced this is always
    // the right thing to do.

    double unmodifiedBaseX = baseX;
    if (delta > 2 * noteWidth) {
        int shift = (delta - 2 * noteWidth) / 5;
	baseX += std::min(shift, (m_npf->getNoteBodyWidth() * 3 / 2));
    }

    // Find out whether the chord contains any accidentals, and if so,
    // make space, and also shift the notes' positions right somewhat.
    // (notepixmapfactory quite reasonably places the hot spot at the
    // start of the note head, not at the start of the whole pixmap.)
    // Also use this loop to check for beamed-group information.

    unsigned int i;
    int accWidth = 0;
    long groupId = -1;

    for (i = 0; i < chord.size(); ++i) {

	(*chord[i])->setLayoutAirspace(unmodifiedBaseX, delta);

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	Accidental acc = NoAccidental;
	if (note->event()->get<String>(DISPLAY_ACCIDENTAL, acc) &&
	    acc != NoAccidental) {
            accWidth = std::max(accWidth, m_npf->getAccidentalWidth(acc));
	}

	if (groupId != -2) {
	    long myGroupId = -1;
	    if (note->event()->get<Int>(BEAMED_GROUP_ID, myGroupId) &&
		(groupId == -1 || myGroupId == groupId)) {
		groupId = myGroupId;
	    } else {
		groupId = -2; // not all note-heads think they're in the group
	    }
	}
    }

    baseX += accWidth;
    delta += accWidth;

    // Cope with the presence of shifted note-heads

    bool shifted = chord.hasNoteHeadShifted();

    if (shifted) {
	if (delta < noteWidth * 2) delta = noteWidth * 2;
	if (!chord.hasStemUp()) baseX += noteWidth;
    }

    // Check for any ties going back, and if so work out how long it
    // must have been and assign accordingly.

    for (i = 0; i < chord.size(); ++i) {

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	bool tiedForwards = false;
	bool tiedBack = false;

	note->event()->get<Bool>(TIED_FORWARD,  tiedForwards);
	note->event()->get<Bool>(TIED_BACKWARD, tiedBack);

	int pitch = note->event()->get<Int>(PITCH);
	if (tiedForwards) {
	    note->event()->setMaybe<Int>(TIE_LENGTH, 0);
	    tieMap[pitch] = chord[i];
	} else {
	    note->event()->unset(TIE_LENGTH);
	}

	if (tiedBack) {
	    TieMap::iterator ti(tieMap.find(pitch));

	    if (ti != tieMap.end()) {
		NotationElementList::iterator otherItr(ti->second);
		
		if ((*otherItr)->getAbsoluteTime() +
		    (*otherItr)->getDuration() ==
		    note->getAbsoluteTime()) {
		    
		    (*otherItr)->event()->setMaybe<Int>
			(TIE_LENGTH,
			 (int)(baseX - (*otherItr)->getLayoutX()));
		    
		} else {
		    tieMap.erase(pitch);
		}
	    }
	}
    }

    // Now set the positions of all the notes in the chord.  We don't
    // need to shift the positions of shifted notes, because that will
    // be taken into account when making their pixmaps later (in
    // NotationStaff::makeNoteSprite / NotePixmapFactory::makeNotePixmap).

    bool barEndsInChord = false;
    for (i = 0; i < chord.size(); ++i) {
	NotationElementList::iterator subItr = chord[i];
	if (subItr == to) barEndsInChord = true;
	(*subItr)->setLayoutX(baseX);
	if (groupId < 0) (*chord[i])->event()->unset(BEAMED);
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
    if (groupId < 0) return delta;

    // Finally set the beam data

    long nextGroupId = -1;
    NotationElementList::iterator scooter(itr);
    ++scooter;

    if (scooter == staff.getViewElementList()->end() ||
	!(*scooter)->event()->get<Int>(BEAMED_GROUP_ID, nextGroupId) ||
	nextGroupId != groupId) {
	
	NotationStaff &notationStaff = dynamic_cast<NotationStaff &>(staff);
	NotationGroup group(*staff.getViewElementList(), itr,
			    getQuantizer(), clef, key);
	group.applyBeam(notationStaff);
	group.applyTuplingLine(notationStaff);
    }

    return delta;
}


int NotationHLayout::getMinWidth(NotationElement &e) const
{
    int w = 0;

    if (e.isNote()) {

        long noteType = e.event()->get<Int>(NOTE_TYPE, noteType);


	clock_t c = clock();

        w += m_npf->getNoteBodyWidth(noteType);

	++nwc;
	nwms += (clock() - c) * 1000 / CLOCKS_PER_SEC;


        long dots;
        if (e.event()->get<Int>(NOTE_DOTS, dots)) {
            w += m_npf->getDotWidth() * dots;
        }

        return w;

    } else if (e.isRest()) {

	clock_t c = clock();

        w += m_npf->getRestWidth(Note(e.event()->get<Int>(NOTE_TYPE),
                                     e.event()->get<Int>(NOTE_DOTS)));

	++rwc;
	rwms += (clock() - c) * 1000 / CLOCKS_PER_SEC;

        return w;
    }

    w = getFixedItemSpacing();

    if (e.event()->isa(Clef::EventType)) {

        w += m_npf->getClefWidth(Clef(*e.event()));

    } else if (e.event()->isa(Key::EventType)) {

        w += m_npf->getKeyWidth(Key(*e.event()));

    } else if (e.event()->isa(Indication::EventType)) {

	w = 0;

    } else {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::getMinWidth(): no case for event type " << e.event()->getType() << endl;
        w += 24;
    }

    return w;
}

int NotationHLayout::getComfortableGap(Note::Type type) const
{
    int bw = m_npf->getNoteBodyWidth();
    if (type < Note::Quaver) return 1;
    else if (type == Note::Quaver) return (bw / 2);
    else if (type == Note::Crotchet) return (bw * 3) / 2;
    else if (type == Note::Minim) return (bw * 3);
    else if (type == Note::Semibreve) return (bw * 9) / 2;
    else if (type == Note::Breve) return (bw * 7);
    return 1;
}        

int NotationHLayout::getBarMargin() const
{
    return (int)(m_npf->getBarMargin() * m_spacing);
}

int NotationHLayout::getPreBarMargin() const
{
    return (int)(m_npf->getBarMargin()) / 2;
}

int NotationHLayout::getPostBarMargin() const
{
    return getBarMargin() - getPreBarMargin();
}

void
NotationHLayout::reset()
{
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {

	BarDataList &bdl = i->second;

	for (int j = 0; j < bdl.size(); ++j) {
	    delete bdl[j].timeSignature;
	}

	bdl.erase(bdl.begin(), bdl.end());
    }

    m_barData.clear();
    m_totalWidth = 0;
}

void
NotationHLayout::resetStaff(StaffType &staff, timeT startTime, timeT endTime)
{
    if (startTime == endTime) {
        getBarData(staff).clear();
        m_totalWidth = 0;
    }
}

int
NotationHLayout::getFirstVisibleBar()
{
    int bar = -1;
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = getFirstVisibleBarOnStaff(*i->first);
	if (barHere >= 0) {
	    if (barHere < bar || bar < 0) bar = barHere;
	}
    }
    return bar;
}

int
NotationHLayout::getFirstVisibleBarOnStaff(StaffType &staff)
{
    BarDataList &bdl(getBarData(staff));
    for (int i = 0; i < bdl.size(); ++i) {
	if (bdl[i].barNo >= 0) return i;
    }
    return -1;
}

int
NotationHLayout::getLastVisibleBar()
{
    int bar = -1;
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = i->second.size() - 1;
	if (barHere > bar) bar = barHere;
    }
    return bar;
}

int
NotationHLayout::getLastVisibleBarOnStaff(StaffType &staff)
{
    BarDataList &bdl(getBarData(staff));
    return bdl.size() - 1;
}

double
NotationHLayout::getBarPosition(int bar)
{
    if (bar < 0) bar = 0;

    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = i->second.size() - 1;
	if (barHere >= bar) return i->second[bar].x;
    }

    return getBarPosition(getLastVisibleBar());
}

bool
NotationHLayout::isBarCorrectOnStaff(StaffType &staff, int i)
{
    BarDataList &bdl(getBarData(staff));
    ++i;
    if (i <= 0 || i >= bdl.size()) return true;
    else return getBarData(staff)[i].correct;
}

Event *NotationHLayout::getTimeSignaturePosition(StaffType &staff,
						 int i, double &timeSigX)
{
    BarDataList &bdl(getBarData(staff));
    if (i < 0) i = 0;
    if (i >= bdl.size()) i = bdl.size() - 1;
    timeSigX = (double)(bdl[i]).timeSigX;
    return (bdl[i]).timeSignature;
}

