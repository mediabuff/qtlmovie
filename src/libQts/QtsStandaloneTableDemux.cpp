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
//
// Qts, the Qt MPEG Transport Stream library.
// Define the class QtsStandaloneTableDemux.
//
//----------------------------------------------------------------------------

#include "QtsStandaloneTableDemux.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

QtsStandaloneTableDemux::QtsStandaloneTableDemux(const QtsPidSet& pidFilter) :
    QtsSectionDemux(this, 0, pidFilter),
    _tables()
{
}


//----------------------------------------------------------------------------
// Inherited reset methods.
//----------------------------------------------------------------------------

void QtsStandaloneTableDemux::reset()
{
    // Reset the demux in superclass.
    QtsSectionDemux::reset();

    // Reset the demuxed tables.
    _tables.clear();
}

void QtsStandaloneTableDemux::resetPid(QtsPid pid)
{
    // Reset the demux for the PID in superclass.
    QtsSectionDemux::resetPid(pid);

    // Removed demuxed tables for this PID.
    int index = 0;
    for (int i = 0; i < _tables.size(); ++i) {
        if (_tables[i]->sourcePid() != pid) {
            _tables[index++] = _tables[i];
        }
    }
    _tables.resize(index);
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void QtsStandaloneTableDemux::handleTable(QtsSectionDemux& demux, const QtsTable& table)
{
    _tables.append(new QtsTable(table));
}
