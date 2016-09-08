//----------------------------------------------------------------------------
//
// Copyright (c) 2013-2016, Thierry Lelegard
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
// Command line tool to test extraction of a DVD title set.
//
//----------------------------------------------------------------------------

#include "QtlTestCommand.h"
#include "QtlDvdTitleSet.h"
#include "QtlDvdProgramChainDemux.h"
#include "QtlDataPullSynchronousWrapper.h"

//----------------------------------------------------------------------------

class QtlTestDvdVts : public QtlTestCommand
{
    Q_OBJECT

public:
    QtlTestDvdVts() : QtlTestCommand("dvdvts", "ifo-or-vob-file out-file [fix-navpacks|remove-navpacks|copy-navpacks [pgc-number]]") {}
    virtual int run(const QStringList& args) Q_DECL_OVERRIDE;
};

//----------------------------------------------------------------------------

int QtlTestDvdVts::run(const QStringList& args)
{
    if (args.size() < 2) {
        return syntaxError();
    }

    const QString input(args[0]);
    const QString output(args[1]);
    const QString navpackOption(args.size() < 3 ?"" : args[2]);
    int pgcNumber = args.size() < 4 ? 0 : args[3].toInt();

    Qtl::DvdDemuxPolicy demuxPolicy = Qtl::NavPacksFixed;
    if (navpackOption.startsWith('f', Qt::CaseInsensitive)) {
        demuxPolicy = Qtl::NavPacksFixed;
    }
    else if (navpackOption.startsWith('r', Qt::CaseInsensitive)) {
        demuxPolicy = Qtl::NavPacksRemoved;
    }
    else if (navpackOption.startsWith('c', Qt::CaseInsensitive)) {
        demuxPolicy = Qtl::NavPacksUnchanged;
    }

    // Load VTS description.
    QtlDvdTitleSet vts(input, &log);
    if (!vts.isLoaded()) {
        err << "Error loading " << input << endl;
        return EXIT_FAILURE;
    }

    QtlDvdProgramChainDemux demux(vts,
                                  pgcNumber,
                                  1,       // angleNumber
                                  1,       // fallbackPgcNumber
                                  Qtl::DEFAULT_DVD_TRANSFER_SIZE,       // transferSize
                                  QtlDataPull::DEFAULT_MIN_BUFFER_SIZE, // minBufferSize
                                  demuxPolicy,
                                  &log,
                                  0,       // parent
                                  true);   // useMaxReadSpeed

    // Create binary output file.
    QFile file(output);
    if (!file.open(QFile::WriteOnly)) {
        err << "Error creating " << output << endl;
        return EXIT_FAILURE;
    }

    // Transfer the file using a wrapper test class.
    QtlDataPullSynchronousWrapper(&demux, &file);
    out << "Completed, pulled " << (demux.pulledSize() / Qtl::DVD_SECTOR_SIZE) << " sectors, " << demux.pulledSize() << " bytes" << endl;

    file.close();

    return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------

#include "QtlTestDvdVts.moc"
namespace {QtlTestDvdVts thisTest;}

