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
    COPYING included with this distribution for more m_information.
*/

#include "ProjectPackager.h"

#include "document/RosegardenDocument.h"
#include "base/Composition.h"
#include "base/Track.h"
#include "gui/general/IconLoader.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QProcess>
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QLabel>
#include <QProgressBar>
#include <QMessageBox>

#include <iostream>

namespace Rosegarden
{

ProjectPackager::ProjectPackager(QWidget *parent, RosegardenDocument *document,  int mode, QString filename) :
        QDialog(parent),
        m_doc(document),
        m_mode(mode),
        m_filename(filename)
{
    // (I'm not sure why RG_DEBUG didn't work from in here.  Having to use
    // iostream is mildly irritating, as QStrings have to be converted, but
    // whatever, I'll figure that out later, or just leave well enough alone)
    std::cout << "ProjectPackager::ProjectPackager():  mode: " << mode << " m_filename: " << m_filename.toStdString() << std::endl;

    this->setModal(false);

    setIcon(IconLoader().loadPixmap("window-packager"));

    QGridLayout *layout = new QGridLayout;
    this->setLayout(layout);

    QLabel *icon = new QLabel(this);
    icon->setPixmap(IconLoader().loadPixmap("rosegarden-packager"));
    layout->addWidget(icon, 0, 0);

    QString modeStr;
    switch (mode) {
        case ProjectPackager::Unpack:  modeStr = tr("Unpack"); break;
        case ProjectPackager::Pack:    modeStr = tr("Pack");   break;
    }
    this->setWindowTitle(tr("Rosegarden - %1 Project Package...").arg(modeStr));

    m_info = new QLabel(this);
    m_info->setWordWrap(true);
    layout->addWidget(m_info, 0, 1);

    m_progress = new QProgressBar(this);
    m_progress->setMinimum(0);
    m_progress->setMaximum(100);
    layout->addWidget(m_progress, 1, 1);

    QPushButton *ok = new QPushButton(tr("Cancel"), this);
    connect(ok, SIGNAL(clicked()), this, SLOT(reject()));
    layout->addWidget(ok, 3, 1); 

    switch (mode) {
        case ProjectPackager::Unpack:  runUnpack();    break;
        case ProjectPackager::Pack:    runPack();  break;
    }
}

void
ProjectPackager::puke(QString error)
{
    m_progress->setMaximum(100);
    m_progress->hide();

    m_info->setText(tr("Fatal error.  Processing aborted."));
    QMessageBox::critical(this, tr("Rosegarden - Fatal processing error!"), error, QMessageBox::Ok, QMessageBox::Ok);

    // abort processing after a fatal error, so calls to puke() abort the whole
    // process in its tracks
    reject();

    // Well, that was the theory.  In practice it apparently isn't so easy to do
    // the bash equivalent of a spontaneous "exit 1" inside a QDialog.  Hrm.
}

QStringList
ProjectPackager::getAudioFiles()
{
    QStringList list;

    // get the Composition
    Composition *comp = &m_doc->getComposition();

    Track *track = 0;

    for (int trackPos = 0;
        (track = comp->getTrackByPosition(trackPos)) != 0; ++trackPos) {

        for (Composition::iterator i = comp->begin(); i != comp->end(); ++i) {

            if ((*i)->getTrack() != track->getId())
                continue;

            // Check whether the track is an audio track
            InstrumentId instrumentId = track->getInstrument();
            bool isAudioTrack = (instrumentId >= AudioInstrumentBase &&
                                 instrumentId < MidiInstrumentBase);

            if (isAudioTrack) {

                std::cout << "trackPos: " << trackPos << " is an audio track" << std::endl;

            }
        }
    }



    // 1. Set up a loop to iterate through all segments on all tracks from
    // LilyPondExporter
    //
    // 2. Find audio segments (should be obvious enough, and the opposite of
    // what LilyPondExporter does)
    //
    // 3. Figure out what audio file is associated with that segment (surely
    // must be discoverable and fairly obvious, though I have no idea how that
    // works off hand)
    //
    // 4. Build up a QStringList somehow or other
    //
    // (Is it cleaner/faster to build the entire list and then |sort | uniq it
    // at the end, or do we
    //
    //   for (each new file I have)
    //       if (file isn't already in the growing longer list)
    //           add_it_to_the_list
    //
    // all on one pass?  I really have no idea off hand what kind of
    // "|sort|uniq" features QStringList might already have, etc. so homework
    // required.
    //
    // I'm really not sure whether it would be faster to iterate through the
    // final 1000 file list one time to prune 900 duplicate files out of it, or
    // to check a short list that winds up being 100 long 1000 times in total.
    // That's probably the kind of thing you'd need a profiler and a glob of
    // different test cases to really pin down, but maybe it's only because I'm
    // so math challenged I don't see the obvious here.
    //
    // Oh well, if the first version of the algorithm is crap, it's bound to be
    // simple enough just to refactor the algorithm later, so just go with
    // whichever approach seems closest to hand and easiest to get written.
    //
}

void
ProjectPackager::runPack()
{
    m_info->setText(tr("Packing project..."));

    getAudioFiles();

    /*
    m_process = new QProcess;
    m_process->start("convert-ly", QStringList() << "-e" << m_filename);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(runUnpack(int, QProcess::ExitStatus)));

    // wait up to 30 seconds for process to start
    if (m_process->waitForStarted()) {
        m_info->setText(tr("<b>convert-ly</b> started..."));
    } else {
        puke(tr("<qt><p>Could not run <b>convert-ly</b>!</p><p>Please install LilyPond and ensure that the \"convert-ly\" and \"lilypond\" commands are available on your path.  If you perform a <b>Run Command</b> (typically <b>Alt+F2</b>) and type \"convert-ly\" into the box, you should not get a \"command not found\" error.  If you can do that without getting an error, but still see this error message, please consult <a href=\"mailto:rosegarden-user@lists.sourceforge.net\">rosegarden-user@lists.sourceforge.net</a> for additional help.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    m_progress->setValue(25);
    /* */
}

void
ProjectPackager::runUnpack()
{
    std::cout << "ProjectPackager::runUnpack()" << std::endl;
    return;

    if (m_process->exitCode() == 0) {
        m_info->setText(tr("<b>convert-ly</b> finished..."));
        delete m_process;
    } else {
        puke(tr("<qt><p>Ran <b>convert-ly</b> successfully, but it terminated with errors.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    m_progress->setValue(50);

    m_process = new QProcess;
    m_info->setText(tr("Running <b>lilypond</b>..."));
    m_process->start("lilypond", QStringList() << "--pdf" << m_filename);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(runFinalStage(int, QProcess::ExitStatus)));
            

    if (m_process->waitForStarted()) {
        m_info->setText(tr("<b>lilypond</b> started..."));
    } else {
        puke(tr("<qt><p>Could not run <b>lilypond</b>!</p><p>Please install LilyPond and ensure that the \"convert-ly\" and \"lilypond\" commands are available on your path.  If you perform a <b>Run Command</b> (typically <b>Alt+F2</b>) and type \"lilypond\" into the box, you should not get a \"command not found\" error.  If you can do that without getting an error, but still see this error message, please consult <a href=\"mailto:rosegarden-user@lists.sourceforge.net>rosegarden-user@lists.sourceforge.net</a> for additional help.</p><p>Processing terminated due to fatal errors.</p></qt>"));
    }

    // go into Knight Rider mode when chewing on LilyPond, because it can take
    // an eternity, but I don't really want to re-create all the text stream
    // monitoring and guessing code that's easy to do in a script and hell to do
    // in real code
    m_progress->setMaximum(0);
}
/*
void
ProjectPackager::runFinalStage(int exitCode, QProcess::ExitStatus)
{
    if (exitCode == 0) {
        m_info->setText(tr("<b>lilypond</b> finished..."));
        delete m_process;
    } else {

        // read preferences from last export from QSettings to offer clues what
        // failed
        QSettings settings;
        settings.beginGroup(LilyPondExportConfigGroup);
        bool exportedBeams = settings.value("lilyexportbeamings", false).toBool();
        bool exportedBrackets = settings.value("lilyexportstaffbrackets", false).toBool();
        settings.endGroup();

        std::cout << "  finalStage: exportedBeams == " << (exportedBeams ? "true" : "false") << std::endl
                  << " exportedBrackets == " << (exportedBrackets ? "true" : "false") << std::endl;

        QString vomitus = QString(tr("<qt><p>Ran <b>lilypond</b> successfully, but it terminated with errors.</p>"));

        if (exportedBeams) {
            vomitus += QString(tr("<p>You opted to export Rosegarden's beaming, and LilyPond could not process the file.  It is likely that you performed certain actions in the course of editing your file that resulted in hidden beaming properties being attached to events where they did not belong, and this probably caused LilyPond to fail.  The recommended solution is to either leave beaming to LilyPond (whose automatic beaming is far better than Rosegarden's) and un-check this option, or to un-beam everything and then re-beam it all manually inside Rosgarden.  Leaving the beaming up to LilyPond is probaby the best solution.</p>"));
        }

        if (exportedBrackets) {
            vomitus += QString(tr("<p>You opted to export staff group brackets, and LilyPond could not process the file.  Unfortunately, this useful feature can be very fragile.  Please go back and ensure that all the brackets you've selected make logical sense, paying particular attention to nesting.  Also, please check that if you are working with a subset of the total number of tracks, the brackets on that subset make sense together when taken out of the context of the whole.  If you have any doubts, please try turning off the export of staff group brackets to see whether LilyPond can then successfully render the result.</p>"));
        }

        vomitus += QString(tr("<p>Processing terminated due to fatal errors.</p></qt>"));

        puke(vomitus);

        // puke doesn't actually work, so we have to return in order to avoid
        // further processing
        return;
    }

    QString pdfName = m_filename.replace(".ly", ".pdf");

    // retrieve user preferences from QSettings
    QSettings settings;
    settings.beginGroup(ExternalApplicationsConfigGroup);
    int pdfViewerIndex = settings.value("pdfviewer", 0).toUInt();
    int filePrinterIndex = settings.value("fileprinter", 0).toUInt();
    settings.endGroup();

    QString pdfViewer, filePrinter;

    // assumes the PDF viewer is available in the PATH; no provision is made for
    // the user to specify the location of any of these explicitly, and I'd like
    // to avoid having to go to that length if at all possible, in order to
    // reduce complexity both in code and on the user side of the configuration
    // page (I guess arguably the configuration page shouldn't exist, and we
    // should just try things sequentially until something works, but it gets
    // into real headaches trying to guess what someone would prefer based on
    // what desktop they're running, and anyway specifying explicitly avoids the
    // reason why my copy of acroread is normally chmod -x so the script
    // ancestor of this class wouldn't pick it up against my wishes)
    switch (pdfViewerIndex) {
        case 0: pdfViewer = "okular";   break;
        case 1: pdfViewer = "evince";   break;
        case 2: pdfViewer = "acroread"; break;
        case 3: pdfViewer = "kpdf"; 
        default: pdfViewer = "kpdf"; // just because I'm still currently on KDE3
    }

    switch (filePrinterIndex) {
        case 0: filePrinter = "kprinter"; break;
        case 1: filePrinter = "gtklp";    break;
        case 2: filePrinter = "lpr";      break;
        case 3: filePrinter = "lp";       break;
        case 4: filePrinter = "hp-print"; break;
        default: filePrinter = "lpr";     break;
    }

    // So why didn't I just manipulate finalProcessor in the first place?
    // Because I just thought of that, but don't feel like refactoring all of
    // this yet again.  Oh well.
    QString finalProcessor;

    m_process = new QProcess;

    switch (m_mode) {
        case ProjectPackager::Print:
            m_info->setText(tr("Printing %1...").arg(pdfName));
            finalProcessor = filePrinter;
            break;

        // just default to preview (I always use preview anyway, as I never
        // trust the results for a direct print without previewing them first,
        // and in fact the direct print option seems somewhat dubious to me)
        case ProjectPackager::Preview:
        default:
            m_info->setText(tr("Previewing %1...").arg(pdfName));
            finalProcessor = pdfViewer;
    }

    m_process->start(finalProcessor, QStringList() << pdfName);
    if (m_process->waitForStarted()) {
        QString t = QString(tr("<b>%1</b> started...").arg(finalProcessor));
    } else {
        QString t = QString(tr("<qt><p>LilyPond processed the file successfully, but <b>%1</b> did not run!</p><p>Please configure a valid %2 under <b>Settings -> Configure Rosegarden -> General -> External Applications</b> and try again.</p><p>Processing terminated due to fatal errors.</p></qt>")).arg(finalProcessor).arg(
                (m_mode == ProjectPackager::Print ? tr("file printer") : tr("PDF viewer")));
        puke(t);
    }

    m_progress->setMaximum(100);
    m_progress->setValue(100);

    accept();
} */

}

#include "ProjectPackager.moc"
