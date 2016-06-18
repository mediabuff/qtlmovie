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
//
// Main program for the TestDvd tool.
//
//----------------------------------------------------------------------------

#include "QtlFile.h"
#include "QtlStdoutLogger.h"
#include "QtlDvdTitleSet.h"
#include "QtlDvdMedia.h"
#include "QtlDataPullWrapper.h"

#define READ_CHUNK 1000  // Number of sectors per read operation.


//----------------------------------------------------------------------------
// Display a directory structure.
//----------------------------------------------------------------------------

namespace {
    void displayFile(QTextStream& out, const QString& indent, const QtlDvdFile& file)
    {
        out << indent << file.name()
            << ", " << file.sizeInBytes() << " bytes"
            << ", LBA " << file.startSector() << "-" << (file.startSector() + (file.sizeInBytes() - 1) / QtlDvdMedia::DVD_SECTOR_SIZE)
            << endl;
    }

    void displayDirectory(QTextStream& out, const QString& indent, const QtlDvdDirectory& dir)
    {
        displayFile(out, indent + "Directory: ", dir);
        foreach (const QtlDvdFile& file, dir.files()) {
            displayFile(out, indent + "    File: ", file);
        }
        foreach (const QtlDvdDirectory& subdir, dir.subDirectories()) {
            displayDirectory(out, indent + "    ", subdir);
        }
    }
}


//----------------------------------------------------------------------------
// Program entry point.
//----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout, QIODevice::WriteOnly);
    QTextStream err(stderr, QIODevice::WriteOnly);
    QtlStdoutLogger log(0, true);

    // Expect at least one argument, the name of a VTS file on DVD.
    const QStringList args(app.arguments());
    if (args.size() < 2) {
        err << "syntax: " << args[0] << " ifo-or-vob-file [out-file [sector-count]]" << endl;
        return EXIT_FAILURE;
    }
    const QString input(args[1]);
    const QString output(args.size() <= 2 ? "" : (args[2] != "-" ? args[2] : QProcess::nullDevice()));
    int sectorCount = args.size() <= 3 ? -1 : args[3].toInt();

    // Set DVDCSS_VERBOSE=2 for verbose logs from libdvdcss.
    qputenv("DVDCSS_VERBOSE", "2");

    // Load DVD media description.
    QtlDvdMedia dvd(input, &log);

    // Display DVD information.
    out << "Input file: " << input << endl
        << "Is open: " << dvd.isOpen() << endl
        << "Is encrypted: " << dvd.isEncrypted() << endl
        << "Root name: " << dvd.rootName() << endl
        << "Device name: " << dvd.deviceName() << endl
        << "Volume id: \"" << dvd.volumeId() << "\"" << endl
        << "Volume size: " << dvd.volumeSizeInSectors() << " sectors" << endl
        << "File structure:" << endl;
    displayDirectory(out, "", dvd.rootDirectory());
    out << endl;

    // Load VTS description.
    QtlDvdTitleSet vts(input, &log);
    if (!vts.isLoaded()) {
        err << "Error loading " << input << endl;
        return EXIT_FAILURE;
    }

    // Display VTS information.
    out << "IFO file: " << vts.ifoFileName() << endl
        << "VOB size: " << vts.vobSizeInBytes() << " bytes" << endl
        << "Is encrypted: " << vts.isEncrypted() << endl
        << "VTS number: " << vts.vtsNumber() << endl
        << "VOB count: " << vts.vobCount() << endl
        << "VOB files: " << endl << "    " << vts.vobFileNames().join("\n    ") << endl
        << "DVD device name: " << vts.deviceName() << endl
        << "DVD volume id: \"" << vts.volumeId() << "\"" << endl
        << "DVD volume size: " << vts.volumeSizeInSectors() << " sectors" << endl
        << "VOB start sector: " << vts.vobStartSector() << endl
        << "VOB sector count: " << vts.vobSectorCount() << endl
        << "Stream count: " << vts.streamCount() << endl;

    // Display stream information.
    foreach (const QtlMediaStreamInfoPtr& stream, vts.streams()) {
        out << "Stream: " << stream->description(false) << endl
            << "    type: " << stream->streamTypeName() << endl
            << "    stream id: " << stream->streamId() << endl;
        if (stream->streamType() == QtlMediaStreamInfo::Video) {
            out << "    width x height: " << stream->width() << " x " << stream->height() << endl
                << "    aspect ratio: " << stream->displayAspectRatioString() << endl;
        }
        else {
            out << "    language:" << stream->language() << endl
                << "    commentary: " << stream->isCommentary() << endl
                << "    impaired: " << stream->impaired() << endl;
            if (stream->streamType() == QtlMediaStreamInfo::Audio) {
                out << "    audio channels: " << stream->audioChannels() << endl;
            }
            else if (stream->streamType() == QtlMediaStreamInfo::Subtitle) {
                out << "    forced: " << stream->forced() << endl;
            }
        }
    }

    // Extract and decrypt VTS content if required.
    if (output.isEmpty()) {
        // Nothing to do.
    }
    else {
        // Create binary output file.
        QFile file(output);
        if (!file.open(QFile::WriteOnly)) {
            err << "Error creating " << output << endl;
            return EXIT_FAILURE;
        }

        if (sectorCount > 0) {
            out << "Reading at most " << sectorCount << " VTS sectors in " << output << endl;
        }
        else {
            out << "Reading VTS as " << output << endl;
        }

        // Measure transfer bandwidth.
        QTime time;
        time.start();

        // Transfer the file using a wrapper test class.
        QtlDataPull* dataPull = vts.dataPull(&log, &vts);
        if (sectorCount > 0) {
            dataPull->setMaxPulledSize(qint64(sectorCount) * QtlDvdMedia::DVD_SECTOR_SIZE);
        }
        QtlDataPullWrapper wrapper(dataPull, &file);

        // Report transfer bandwidth.
        const int milliSec = time.elapsed();
        if (milliSec > 0) {
            const qint64 bps = (dataPull->pulledSize() * 8 * 1000) / milliSec;
            out << "Transfer bandwidth: " << bps << " b/s, " << (bps / 8) << " B/s" << endl;
        }

        file.close();
    }

    return EXIT_SUCCESS;
}