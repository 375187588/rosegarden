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

#include <vector>


#include <qgroupbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "Quantizer.h"
#include "widgets.h"


// Provides a mechanism for viewing and modifying the parameters
// associated with a Rosegarden Segment.
//
//

namespace Rosegarden { class Segment; }
class RosegardenGUIView;
class MultiViewCommandHistory;
class Command;

#ifndef _SEGMENTPARAMETERBOX_H_
#define _SEGMENTPARAMETERBOX_H_

class SegmentParameterBox : public QGroupBox
{
Q_OBJECT

public:

    typedef enum
    {
        None,
        Some,
        All
    } Tristate;

    SegmentParameterBox(RosegardenGUIView *view,
                        QWidget *parent=0,
                        const char *name=0,
                        WFlags f=0);
    ~SegmentParameterBox();

    // Use Segments to update GUI parameters
    //
    void useSegment(Rosegarden::Segment *segment);
    void useSegments(std::vector<Rosegarden::Segment*> segments);

    // Command history stuff
    MultiViewCommandHistory* getCommandHistory();
    void addCommandToHistory(Command *command);


public slots:
    void slotRepeatPressed();
    void slotQuantizeSelected(int);

    void slotTransposeSelected(int);
    void slotTransposeTextChanged(const QString &);

    void slotDelaySelected(int);
    void slotDelayTextChanged(const QString &);

    // catching commands
    void slotCommandExecuted(Command *command);

private:
    void initBox();
    void populateBoxFromSegments();

    RosegardenTristateCheckBox *m_repeatValue;
    RosegardenComboBox         *m_quantizeValue;
    RosegardenComboBox         *m_transposeValue;
    RosegardenComboBox         *m_delayValue;

    std::vector<Rosegarden::Segment*> m_segments;

    std::vector<Rosegarden::StandardQuantization>
    m_standardQuantizations;

    RosegardenGUIView          *m_view;

    Rosegarden::MidiByte        m_tranposeRange;
};


#endif // _SEGMENTPARAMETERBOX_H_
