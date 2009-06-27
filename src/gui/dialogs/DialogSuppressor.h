/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_DIALOG_SUPPRESSOR_H_
#define _RG_DIALOG_SUPPRESSOR_H_

#include <QDialog>
#include <QObject>
#include <QString>

namespace Rosegarden
{
	
class DialogSuppressor
{
public:
    static bool shouldSuppress(QDialog *, QString settingsKey);
    static bool isSuppressed(QString settingsKey);
};
  
class SuppressionTarget : public QObject
{
    Q_OBJECT

public:
    SuppressionTarget(QString key) : m_key(key) { }

public slots:
    void slotSuppressionToggled(bool);

private:
    QString m_key;
};

}

#endif