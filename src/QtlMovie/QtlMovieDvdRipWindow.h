//----------------------------------------------------------------------------
//
// Copyright (c) 2016, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//! @file QtlMovieDvdRipWindow.h
//!
//! Declare the class QtlMovieDvdRipWindow, an instance of the UI for the
//! DVD Ripper in QtlMovie application.
//!
//----------------------------------------------------------------------------

#ifndef QTLMOVIEDVDRIPWINDOW_H
#define QTLMOVIEDVDRIPWINDOW_H

#include "ui_QtlMovieDvdRipWindow.h"
#include "QtlMovieMainWindowBase.h"

//!
//! A subclass of QMainWindow which implements the UI for the DVD Ripper application.
//! The design of the UI is done using Qt Designer.
//!
class QtlMovieDvdRipWindow : public QtlMovieMainWindowBase
{
    Q_OBJECT

public:
    //!
    //! Constructor.
    //! @param [in] parent Optional parent widget.
    //! @param [in] logDebug If true, set initial log mode to debug.
    //!
    explicit QtlMovieDvdRipWindow(QWidget *parent = 0, bool logDebug = false);

private slots:
    //!
    //! Invoked by the "Start ..." buttons.
    //!
    void startExtraction();
    //!
    //! Invoked by the "Cancel ..." buttons.
    //!
    void cancelExtraction();
    //!
    //! Invoked when extraction starts.
    //!
    void extractionStarted();
    //!
    //! Invoked when extraction stops.
    //! @param [in] success Indicates whether the extraction succeeded or failed.
    //!
    void extractionStopped(bool success);
    //!
    //! Invoked when some progress is made in the extraction process.
    //! @param [in] description The description of the current process.
    //! @param [in] current Current value (whatever unit it means).
    //! @param [in] maximum Value indicating full completion.
    //! If zero, we cannot evaluate the progress.
    //! @param [in] elapsedSeconds Elapsed seconds since the process started.
    //! @param [in] remainingSeconds Estimated remaining seconds to process.
    //! Negative if the remaining time cannot be estimated.
    //!
    void extractionProgress(const QString& description, int current, int maximum, int elapsedSeconds, int remainingSeconds);

private:
    //!
    //! Check if an extraction is currently in progress and propose to abort it.
    //! If the user accept to abort it, start the cancelation. The completion
    //! of the cancelation will be notified later by extractionStopped().
    //! @return Cancel status.
    //!
    virtual CancelStatus proposeToCancel();

    //!
    //! Apply the settings which affect the UI.
    //!
    void applyUserInterfaceSettings();

    //!
    //! Update the UI when extraction starts or stops.
    //! @param [in] started True if extraction starts, false if it stops.
    //!
    void extractionUpdateUi(bool started);

    Ui::QtlMovieDvdRipWindow _ui;  //!< UI from Qt Designer.

    // Unaccessible operations.
    Q_DISABLE_COPY(QtlMovieDvdRipWindow)
};

#endif // QTLMOVIEDVDRIPWINDOW_H