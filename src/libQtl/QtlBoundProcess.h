//----------------------------------------------------------------------------
//
// Copyright (c) 2013-2017, Thierry Lelegard
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
//! @file QtlBoundProcess.h
//!
//! Declare the class QtlBoundProcess, an encapsulation of QProcess.
//! Qtl, Qt utility library.
//!
//----------------------------------------------------------------------------

#ifndef QTLBOUNDPROCESS_H
#define QTLBOUNDPROCESS_H

#include "QtlCore.h"
#include "QtlBoundProcessResult.h"

//!
//! An encapsulation of QProcess which reports only the final state.
//!
//! A QtlBoundProcess instance deletes itself automatically after process
//! termination (after start()) or cancelation (after cancel()).
//! It is both useless and invalid to explicitely delete the object.
//! To enfore object creation on the heap, there is no public constructor.
//! Always use the static method newInstance().
//!
class QtlBoundProcess : public QObject
{
    Q_OBJECT

public:
    //!
    //! Create a new instance.
    //! Enforce object creation on the heap.
    //! @param [in] program Executable file path.
    //! @param [in] arguments Command line arguments.
    //! @param [in] msRunTestTimeout Timeout of process execution in milliseconds. Ignored if negative or zero.
    //! @param [in] maxProcessOutputSize Maximum size in bytes of process output. Ignored if negative or zero.
    //! @param [in] parent Optional parent object.
    //! @param [in] env Environment for the process. Current environment by default.
    //! @param [in] pipeInput If true, the standard input of the process can be fed using inputDevice().
    //! @return The new QtlBoundProcess instance.
    //!
    static QtlBoundProcess* newInstance(const QString& program,
                                        const QStringList& arguments = QStringList(),
                                        int msRunTestTimeout = 0,
                                        int maxProcessOutputSize = 0,
                                        QObject* parent = 0,
                                        const QProcessEnvironment& env = QProcessEnvironment(),
                                        bool pipeInput = false);

    //!
    //! Start the process.
    //!
    void start();

    //!
    //! Cancel the start of the process.
    //! If the process is not yet started, it will never be and the terminated()
    //! signal is sent with a cancelation message.
    //!
    void cancel();

    //!
    //! Get the device representing the input pipe of the process.
    //! The returned QIODevice is writeable only.
    //! @return The input pipe as a QIODevice or zero if @a pipeInput
    //! was set to false in the constructor.
    //!
    QIODevice* inputDevice() const
    {
        return _pipeInput ? _process : 0;
    }

signals:
    //!
    //! This signal is emitted when the process is terminated or failed to start.
    //! @param [in] result Process execution results.
    //!
    void terminated(const QtlBoundProcessResult& result);

private slots:
    //!
    //! Read as much data as possible from the process standard error and output.
    //!
    void readOutputData();
    //!
    //! Invoked when the process is finished.
    //! @param [in] exitCode Process exit code.
    //! @param [in] exitStatus QProcess exit status.
    //!
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    //!
    //! Invoked when an error occurs on the process.
    //! @param [in] error QProcess error code.
    //!
    void processError(QProcess::ProcessError error);
    //!
    //! Invoked when the process execution is too long.
    //!
    void processTimeout();

private:
    const int             _msRunTestTimeout;     //!< Timeout of process execution in milliseconds. Ignored if negative or zero.
    const int             _maxProcessOutputSize; //!< Maximum size in bytes of process output. Ignored if negative or zero.
    QtlBoundProcessResult _result;               //!< Process execution results.
    bool                  _pipeInput;            //!< Process input is an accessible pipe.
    bool                  _started;              //!< start() was called.
    bool                  _terminated;           //!< terminated() has been signaled.
    QProcess*             _process;              //!< Actual QProcess object.
    QTimer*               _timer;                //!< Process execution timeout.

    //!
    //! Private constructor.
    //! @param [in] program Executable file path.
    //! @param [in] arguments Command line arguments.
    //! @param [in] msRunTestTimeout Timeout of process execution in milliseconds. Ignored if negative or zero.
    //! @param [in] maxProcessOutputSize Maximum size in bytes of process output. Ignored if negative or zero.
    //! @param [in] parent Optional parent object.
    //! @param [in] env Optional process environment.
    //! @param [in] pipeInput If true, the standard input of the process can be fed using inputDevice().
    //!
    QtlBoundProcess(const QString& program,
                    const QStringList& arguments,
                    int msRunTestTimeout,
                    int maxProcessOutputSize,
                    QObject* parent,
                    const QProcessEnvironment& env = QProcessEnvironment(),
                    bool pipeInput = false);

    //!
    //! Send the terminated() signal.
    //! @param [in] message Error message if process did not complete normally.
    //! Empty if process completed normally.
    //!
    void sendTerminated(const QString& message);

    // Unaccessible operations.
    QtlBoundProcess() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(QtlBoundProcess)
};

#endif // QTLBOUNDPROCESS_H
