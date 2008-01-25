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

#ifndef _RG_AUDIOCONFIGURATIONPAGE_H_
#define _RG_AUDIOCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <klocale.h>
#include <qlineedit.h>

class QWidget;
class QSpinBox;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class KConfig;
class KComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;


class AudioConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioConfigurationPage(RosegardenGUIDoc *doc,
                               KConfig *cfg,
                               QWidget *parent=0,
                               const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Audio"); }
    static QString title()     { return i18n("Audio Settings"); }
    static QString iconName()  { return "configure-audio"; }

#ifdef HAVE_LIBJACK
    QString getJackPath() { return m_jackPath->text(); }
#endif // HAVE_LIBJACK

    static QString getBestAvailableAudioEditor();

protected slots:
    void slotFileDialog();

protected:
    QString getExternalAudioEditor() { return m_externalAudioEditorPath->text(); }


    //--------------- Data members ---------------------------------

#ifdef HAVE_LIBJACK
    QCheckBox *m_startJack;
    QLineEdit *m_jackPath;
#endif // HAVE_LIBJACK


#ifdef HAVE_LIBJACK
    // Number of JACK input ports our RG client creates - 
    // this decides how many audio input destinations
    // we have.
    //
    QCheckBox    *m_createFaderOuts;
    QCheckBox    *m_createSubmasterOuts;

    QComboBox    *m_audioRecFormat;

#endif // HAVE_LIBJACK

    QLineEdit* m_externalAudioEditorPath;
    QComboBox* m_previewStyle;

};
 


}

#endif
