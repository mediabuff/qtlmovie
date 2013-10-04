//----------------------------------------------------------------------------
//
// Copyright (c) 2013, Thierry Lelegard
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
//
// Define the class QtlLineEdit.
// Qtl, Qt utility library.
//
//----------------------------------------------------------------------------

#include "QtlLineEdit.h"
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QRegExp>
#include <QDir>


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

QtlLineEdit::QtlLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
}

QtlLineEdit::QtlLineEdit(const QString& contents, QWidget* parent) :
    QLineEdit(contents, parent)
{
}


//----------------------------------------------------------------------------
// Invoked when a drag operation enters the widget.
//----------------------------------------------------------------------------

void QtlLineEdit::dragEnterEvent(QDragEnterEvent* event)
{
    // Get the MIME data from the event.
    const QMimeData* mimeData = event->mimeData();

    if (mimeData != 0 && mimeData->hasUrls()) {
        // The dragged object contains URL's, accept it (QLineEdit does not).
        event->acceptProposedAction();
    }
    else {
        // For all other types of object, delegate to the super class.
        QLineEdit::dragEnterEvent(event);
    }
}


//----------------------------------------------------------------------------
// Invoked when a drop operation is performed.
//----------------------------------------------------------------------------

void QtlLineEdit::dropEvent(QDropEvent* event)
{
    // Get the MIME data from the event.
    const QMimeData* mimeData = event->mimeData();

    if (mimeData != 0 && mimeData->hasUrls()) {
        // The dropped object contains URL's, accept it. QLineEdit also
        // accept it when dragEnterEvent let it passed but it copies the
        // raw URL. We transform the URL into a file path when possible.
        const QList<QUrl> urlList(mimeData->urls());
        if (urlList.isEmpty()) {
            // No URL in fact.
            event->ignore();
        }
        else {
            // Keep only the first URL.
            setText(toText(urlList.first()));
            // Accept the drop action.
            event->acceptProposedAction();
        }
    }
    else {
        // For all other types of object, delegate to the super class.
        QLineEdit::dropEvent(event);
    }
}


//----------------------------------------------------------------------------
// Format the content of an URL into something to drop in the line edit.
//----------------------------------------------------------------------------

QString QtlLineEdit::toText(const QUrl& url)
{
    // For non-file URL, return the full URL.
    if (url.scheme() != "file") {
        return url.url();
    }

    // Get the local file path.
    QString path(url.url(QUrl::PreferLocalFile));

    // On Windows, remove any leading slash.
#if defined(Q_OS_WIN)
    path.replace(QRegExp("^/*"), "");
#endif

    // Try to make the file path native.
    return QDir::toNativeSeparators(path);
}
