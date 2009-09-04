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

#ifndef _PROJECT_PACKAGER_H_
#define _PROJECT_PACKAGER_H_

#include "document/RosegardenDocument.h"

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QStringList>


namespace Rosegarden
{

/** Implement functionality equivalent to the old external
 *  rosegarden-project-package script.  The script used the external dcop and
 *  kdialog command line utlities to provide a user interface.  We'll do the
 *  user interface in real code, though we still have to use external helper
 *  applications for various purposes to make the thing run.
 *
 *  \author D. Michael McIntyre
 */

class ProjectPackager : public QDialog
{
    Q_OBJECT

public:
    /** The old command line arguments are replaced with an int passed into the
     * ctor, using the following named constants to replace them, and avoid a
     * bunch of string parsing nonsense.  We no longer need a startup ConfTest
     * target.  We'll do the conftest every time we run instead, and only
     * complain if there is a problem.  We no longer need a version target,
     * since the version is tied to Rosegarden itself.
     *
     * The filename parameter should be a temporary file set elsewhere and
     * passed in.
     */
    static const int ConfTest  = 0;
    static const int Pack      = 1;
    static const int Unpack    = 2;

    ProjectPackager(QWidget *parent,
                    RosegardenDocument *document,
                    int mode,
                    QString filename);
    ~ProjectPackager() { };

protected:
    RosegardenDocument *m_doc;
    int                 m_mode;
    QString             m_filename;
    QProgressBar       *m_progress;
    QLabel             *m_info;
    QProcess           *m_process;

    /** Returns a QStringList containing a sorted|uniqed list of audio files
     * used by m_doc
     *
     * Problems to solve: how do we |sort|uniq a QStringList?  The same audio
     * file might be used by hundreds of audio segments (eg. Emergence) and
     * while we could pull it out and overwrite the same file 100 times to
     * result in only one final copy, that's a big waste.
     */
    QStringList getAudioFiles();

/* General questions not resolved yet:
 *
 * When do we change the audio file path from whatever it was to the new one
 * we're creating?  At pack time or unpack time?  Did the old script even do
 * this?
 *
 * I suppose we could do it in both places.  Harmless enough isn't it?  No,
 * scratch that, do it when we pack, because we're packing with a live document,
 * but doing it on an unpack requires...  Well either code hooks (signals and
 * slots?) or some XML hacking.
 *
 */

protected slots:
    /**
     * Display an explanatory failure message and terminate processing
     */
    void puke(QString error);

    /**
     * Begin the packing process, which will entail:
     *
     *   1. discover audio files used by the composition
     *
     *   2. pull out copies to a tmp directory
     *
     *   3. compress audio files with FLAC (preferably using the library,
     *   although that is quite hopeless for the moment, due to the assert()
     *   conflict)
     *
     *   4. prompt user for additional files
     *
     *   5. add them
     *
     *   6. final directory structure looks like:
     *
     *       ./export_filename.rg
     *       ./export_filename/[wav files compressed with FLAC]
     *       ./export_filename/[misc files]
     *
     *   7. tarball this (tar czf)
     *
     *   8. rename it to .rgp from .tar.gz
     *
     *   9. ???
     */
    void runPack();

    /** Create a temporary working directory and assemble copied files to that
     * location for further processing.  The copy operations could take some
     * time, so we probably need a QProcess slot for this, and assemble the
     * various copy operations into one backend script to execute from that
     * QProcess.
     *
     * When complete, trigger startFlacEncoder()
     */
    void assembleFiles(QString path, QStringList files);

    /** Assemble the various flac encoding operations into one backend script to
     * operate from a single QProcess, so this chewing can take place without
     * blocking the GUI.  Since it's possible to track the total number of
     * files, and what file out of n we're on, we should work out some nice way
     * to hook this up to m_status and give some indication of progress,
     * instead of just leaving it in "busy" mode.
     *
     * When complete, trigger promptAdditionalFiles()
     */
    void startFlacEncoder(QString path, QStringList files);

    // flac may leave the original .wav files intact, in which case we need to
    // clean them up, but we can probably build that into the purpose-built
    // script created by the preceding chain link without having to insert
    // another one here.  Probably.

    /** Prompt the user for any additional files they wish to include in the
     * project package.  This will require a bit of thought.  Once the list is
     * assembled, copy the files to the temporary working location.
     *
     * When complete, trigger compressPackage()
     */
    void promptAdditionalFiles();

    /** Turn the whole assembled shebang into a completed .rgp file.  If this
     * would require more than one QProcess, perhaps do another temporary
     * backend script, since we will have had so many of them by then, what the
     * hell.
     *
     * We can probably arrange it so this is the final link in the chain, and
     * signals the end of the pack operation.
     */
    void compressPackage();

    /**
     * Try to unpack an existing .rgd file, which will entail:
     *
     *   1. QProcess a tar xzf command
     *
     *   2. decompress the included FLAC files back to .wav
     *
     *   3. ???
     *
     */
    /** Begin the unpacking process by running 'tar xzf' against the incoming
     * filename in situ.  That should be fine.
     *
     * When complete, trigger startFlacDecoder()
     */
    void runUnpack();

    /** We don't have a live document from which to discover the files this time
     * around. We'll need to look at the disk after unpacking the tarball.  We
     * may be able to use QFile methods for this and avoid using a QProcess, and
     * an extra link in the processing chain.
     */
    //void discoverFiles()

    /** Discover the flac files and assemble a purpose-built script to decode
     * them.  If flac leaves the original .flac files intact, we should
     * incorporate removing them into this script, and get it all done in one
     * QProcess operation.
     *
     * It seems like changing out the audio path would best be done here at this
     * stage too, but we don't have a live document at this point, so I guess we
     * want to incorporate that step into the code behind File -> Import Project
     * Package out at the RosegardenMainWindow level instead, and just make sure
     * to keep track of what it should be set to for when that step arrives.
     *
     * When complete, trigger?  Anything else?
     */
    void runFlacDecoder();
    void startFlacDecoder(QString path, QStringList files);
    // similar to startFlacEncoder
    void updateAudioPath(int exitCode, QProcess::ExitStatus);

};


}

#endif
