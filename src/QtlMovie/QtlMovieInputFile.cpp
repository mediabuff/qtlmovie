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
// Define the class QtlMovieInputFile.
//
//----------------------------------------------------------------------------

#include "QtlMovieInputFile.h"
#include "QtlMovieFFmpeg.h"
#include "QtlMovieDvd.h"
#include "QtlMovieTeletextSearch.h"
#include "QtlMovie.h"
#include "QtlProcess.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

QtlMovieInputFile::QtlMovieInputFile(const QString& fileName,
                                     const QtlMovieSettings* settings,
                                     QtlLogger* log,
                                     QObject *parent) :
    QtlFile(fileName, parent),
    _log(log),
    _settings(settings),
    _ffmpegInput(),
    _ffInfo(),
    _streams(),
    _dvdIfoStreams(),
    _dvdPalette(),
    _teletextSearch(0),
    _selectedVideoStreamIndex(-1),
    _selectedAudioStreamIndex(-1),
    _selectedSubtitleStreamIndex(-1),
    _externalSubtitleFileName()
{
    Q_ASSERT(log != 0);
    Q_ASSERT(settings != 0);

    // Update media info when the file name is changed.
    connect(this, SIGNAL(fileNameChanged(QString)), this, SLOT(updateMediaInfo(QString)));

    // Initial update of the media info.
    updateMediaInfo(this->fileName());
}

QtlMovieInputFile::QtlMovieInputFile(const QtlMovieInputFile& other, QObject* parent) :
    QtlFile(other, parent),
    _log(other._log),
    _settings(other._settings),
    _ffmpegInput(other._ffmpegInput),
    _ffInfo(other._ffInfo),
    _streams(other._streams),
    _dvdIfoStreams(other._dvdIfoStreams),
    _dvdPalette(other._dvdPalette),
    _teletextSearch(0), // don't copy
    _selectedVideoStreamIndex(other._selectedVideoStreamIndex),
    _selectedAudioStreamIndex(other._selectedAudioStreamIndex),
    _selectedSubtitleStreamIndex(other._selectedSubtitleStreamIndex),
    _externalSubtitleFileName(other._externalSubtitleFileName)
{
    // Update media info when the file name is changed.
    connect(this, SIGNAL(fileNameChanged(QString)), this, SLOT(updateMediaInfo(QString)));
}


//----------------------------------------------------------------------------
// Invoked when the file name has changed, update file info.
//----------------------------------------------------------------------------

void QtlMovieInputFile::updateMediaInfo(const QString& fileName)
{
    // By default, the ffmpeg input spec is the file name.
    _ffmpegInput = fileName;

    // Clear all previous media info.
    const bool wasNone = _streams.isEmpty();
    _streams.clear();
    _ffInfo.clear();

    // If there was some info, they changed (we just cleared them).
    if (!wasNone) {
        emit mediaInfoChanged();
    }

    // If no file specified or not existent or no ffprobe available, nothing to analyze.
    const QString ffprobe(_settings->ffprobe()->fileName());
    if (fileName.isEmpty() || !QFile(fileName).exists() || ffprobe.isEmpty()) {
        return;
    }

    // Check if the file belongs to a DVD structure and collect file names.
    QString ifoFileName;
    QStringList vobFiles;
    if (!QtlMovieDvd::buildFileNames(fileName, ifoFileName, vobFiles, _log)) {
        // Found an invalid DVD structure.
        return;
    }

    // Load DVD .IFO file if one was found.
    _dvdIfoStreams.clear();
    _dvdPalette.clear();
    if (!ifoFileName.isEmpty()) {
        if (!QtlMovieDvd::readIfoFile(ifoFileName, _dvdIfoStreams, _dvdPalette, _log)) {
            _log->line(tr("DVD IFO file not correctly analyzed, audio and subtitle languages are unknown"));
        }
        else {
            // Convert palette to RGB format.
            QtlMovieDvd::convertPaletteYuvToRgb(_dvdPalette, _log);
        }
    }

    // Build the ffmpeg input file specification for multiple DVD VOB files.
    if (vobFiles.size() == 1) {
        // Only one file to transcode. Specify it since fileName was maybe the IFO file.
        _ffmpegInput = vobFiles.first();
    }
    else if (!vobFiles.isEmpty()) {
        // More than one VOB file are present.
        // Use the "concat" protocol to specify a list of file to concatenate as input.
        // Example: ffmpeg -i "concat:vts_01_1.vob|vts_01_2.vob|vts_01_3.vob"
        _ffmpegInput = "concat:" + vobFiles.join('|');
    }

    // Create the process object. It will automatically delete itself after completion.
    const QStringList args(QtlMovieFFmpeg::ffprobeArguments(_settings, _ffmpegInput));
    _log->debug(ffprobe + " " + args.join(' '));
    QtlProcess* process = QtlProcess::newInstance(ffprobe, // command
                                                  args,    // command line arguments
                                                  10000,   // execution timeout: 10 seconds
                                                  40000,   // max output size: 40 kB
                                                  this);   // parent object

    // Get notified of process termination and starts the process.
    connect(process, SIGNAL(terminated(QtlProcessResult)), this, SLOT(ffprobeTerminated(QtlProcessResult)));
    process->start();
}


//----------------------------------------------------------------------------
// Invoked when the ffprobe process completes.
//----------------------------------------------------------------------------

void QtlMovieInputFile::ffprobeTerminated(const QtlProcessResult& result)
{
    // Filter ffprobe process execution.
    if (result.hasError()) {
        _log->line(tr("FFprobe error: %1").arg(result.errorMessage()));
    }

    // The standard output from ffprobe contains the media info in the form of "key=value".
    // Fill the _ffInfo data with the parsed info.
    _ffInfo.loadFFprobeOutput(result.standardOutput());

    // Build the stream information from the ffprobe output.
    _ffInfo.buildStreamInfo(_streams);

    // Post-processing when the input has a DVD structure.
    if (!_dvdIfoStreams.isEmpty()) {

        // Merge the info which were previously collected in
        // the .IFO file with the stream info from ffprobe.
        QtlMovieStreamInfo::merge(_streams, _dvdIfoStreams);

        // Sort streams in the DVD order for user's convenience.
        qSort(_streams.begin(), _streams.end(), QtlMovieDvd::lessThan);
    }

    // Analyze TS file if subtitles with unknown types are detected.
    // Usually, this is due to Teletext subtitles.
    bool searchTeletext = false;
    if (_ffInfo.value("format.format_name").toLower() == "mpegts") {
        foreach (const QtlMovieStreamInfoPtr& stream, _streams) {
            if (!stream.isNull() && stream->streamType() == QtlMovieStreamInfo::Subtitle && stream->subtitleType() == QtlMovieStreamInfo::SubOther) {
                searchTeletext = true;
                break;
            }
        }
    }

    // Start TS analysis when unknown subtitle types are found.
    if (searchTeletext) {

        // Cleanup previous search (should not be any).
        if (_teletextSearch != 0) {
            delete _teletextSearch;
        }

        // Create a search action for teletext.
        // Will be deleted no later than this object.
        _teletextSearch = new QtlMovieTeletextSearch(fileName(), _settings, _log, this);

        // Get notifications from the Teletext searcher.
        connect(_teletextSearch, SIGNAL(foundTeletextSubtitles(QtlMovieStreamInfoPtr)), this, SLOT(foundTeletextSubtitles(QtlMovieStreamInfoPtr)));
        connect(_teletextSearch, SIGNAL(completed(bool)), this, SLOT(teletextSearchTerminated(bool)));

        // Start the search.
        if (!_teletextSearch->start()) {
            _log->line(tr("Failed to start search for Teletext subtitles"));
            searchTeletext = false;
            delete _teletextSearch;
            _teletextSearch = 0;
        }
    }

    // Notify the new media information only when nothing more to do.
    if (!searchTeletext) {
        emit mediaInfoChanged();
    }
}


//----------------------------------------------------------------------------
// Invoked when a Teletext subtitle stream is found.
//----------------------------------------------------------------------------

void QtlMovieInputFile::foundTeletextSubtitles(QtlMovieStreamInfoPtr stream)
{
    _log->debug(tr("Found one Teletext subtitle stream, page %1").arg(stream->teletextPage()));

    // The first time a Teletext stream is found on a PID, there should be
    // one stream which was built from the ffprobe output with that PID
    // with unknown subtitle type. We need to remove it.
    if (stream->streamId() >= 0) {
        // We know the PID of the stream in the TS file (should be always the case).
        // Look for previous streams with the same PID.
        for (QtlMovieStreamInfoPtrVector::Iterator it = _streams.begin(); it != _streams.end(); ++it) {
            const QtlMovieStreamInfoPtr& s(*it);
            if (s->streamId() == stream->streamId()) {
                // Found a previous stream with same PID.
                // If the new stream's ffmpeg index is unknow, get it from the previous stream on same PID.
                if (stream->ffIndex() < 0) {
                    stream->setFFIndex(s->ffIndex());
                }
                // If the previous stream was unknown subtitle, remove it
                // since we know have a better characterization of the stream.
                if (s->streamType() == QtlMovieStreamInfo::Subtitle && s->subtitleType() == QtlMovieStreamInfo::SubOther) {
                    _streams.erase(it);
                    // We can stop now and we must since our iterator is broken by erase().
                    break;
                }
            }
        }
    }

    // Append the new stream in the input file.
    _streams.append(stream);
}


//----------------------------------------------------------------------------
// Invoked when the search for Teletext subtitles completes.
//----------------------------------------------------------------------------

void QtlMovieInputFile::teletextSearchTerminated(bool success)
{
    // Cleanup the teletext searcher.
    if (_teletextSearch != 0) {
        _log->debug(tr("Search for Teletext subtitles completed"));
        _teletextSearch->deleteLater();
        _teletextSearch = 0;
    }

    // Notify the new media information.
    emit mediaInfoChanged();
}


//----------------------------------------------------------------------------
// Get the number of streams in the file matching a given type.
//----------------------------------------------------------------------------

int QtlMovieInputFile::streamCount(QtlMovieStreamInfo::StreamType streamType) const
{
    int count = 0;
    foreach (const QtlMovieStreamInfoPtr& s, _streams) {
        if (!s.isNull() && s->streamType() == streamType) {
            count++;
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Get the information about the first stream in the file matching a given type.
//----------------------------------------------------------------------------

QtlMovieStreamInfoPtr QtlMovieInputFile::firstStream(QtlMovieStreamInfo::StreamType streamType) const
{
    foreach (const QtlMovieStreamInfoPtr& s, _streams) {
        if (!s.isNull() && s->streamType() == streamType) {
            return s;
        }
    }
    return 0;
}


//----------------------------------------------------------------------------
// Get the information about a stream in input file.
//----------------------------------------------------------------------------

QtlMovieStreamInfoPtr QtlMovieInputFile::streamInfo(int streamIndex) const
{
    return streamIndex >= 0 && streamIndex < _streams.size() ? _streams[streamIndex] : 0;
}


//----------------------------------------------------------------------------
// Try to evaluate the duration of the media file in seconds.
//----------------------------------------------------------------------------

float QtlMovieInputFile::durationInSeconds() const
{
    // Try duration of file.
    float duration = _ffInfo.floatValue("format.duration");

    // If not found, try all streams duration until one is found.
    const int count = _ffInfo.intValue("format.nb_streams");
    for (int si = 0; duration < 0.001 && si < count; ++si) {
        duration = _ffInfo.floatValueOfStream(si, "duration");
    }

    return duration;
}


//----------------------------------------------------------------------------
// Check if the file seems to be DVD-compliant.
//----------------------------------------------------------------------------

bool QtlMovieInputFile::isDvdCompliant() const
{
    // The DVD-compliant files we generate have the following characteristics:
    // - MPEG-PS file format.
    // - Exactly 2 streams: one video and one audio.
    // - Same video size and aspect ratio.

    const QtlMovieStreamInfoPtr videoStream(firstStream(QtlMovieStreamInfo::Video));
    const QtlMovieStreamInfoPtr audioStream(firstStream(QtlMovieStreamInfo::Audio));

    return
        streamCount(QtlMovieStreamInfo::Video) == 1 &&
        streamCount(QtlMovieStreamInfo::Audio) == 1 &&
        streamCount(QtlMovieStreamInfo::Subtitle) == 0 &&
        !videoStream.isNull() &&
        !audioStream.isNull() &&
        videoStream->width() == QTL_DVD_VIDEO_WIDTH &&
        videoStream->height() == QTL_DVD_VIDEO_HEIGHT &&
        qAbs(videoStream->displayAspectRatio() - QTL_DVD_DAR) < 0.001 &&
        _ffInfo.value("format.format_name") == "mpeg" &&
        _ffInfo.valueOfStream(videoStream->ffIndex(), "codec_name") == "mpeg2video" &&
        _ffInfo.valueOfStream(audioStream->ffIndex(), "codec_name") == "mp2";
}


//----------------------------------------------------------------------------
// Select the default video, audio and subtitle streams.
//----------------------------------------------------------------------------

void QtlMovieInputFile::selectDefaultStreams(const QStringList& audienceLanguages)
{
    // Reset selected streams.
    _selectedVideoStreamIndex = -1;
    _selectedAudioStreamIndex = -1;
    _selectedSubtitleStreamIndex = -1;

    int highestFrameSize = -1;  // Largest video frame size (width x height).
    int firstAudio = -1;        // First audio stream index.
    int originalAudio = -1;     // First original audio stream index (and not impaired).
    int notAudienceAudio = -1;  // First audio stream index not in intended audience languages (and not impaired).
    int notImpairedAudio = -1;  // First audio stream index not for hearing/visual impaired.
    int firstSubtitle = -1;     // First subtitles in intended audience languages.
    int completeSubtitle = -1;  // First complete subtitles in intended audience languages.
    int forcedSubtitle = -1;    // First forced subtitles in intended audience languages.
    int impairedSubtitle = -1;  // First subtitles for impaired in intended audience languages.

    // Loop on all streams.
    const int streamMax = streamCount();
    for (int streamIndex = 0; streamIndex < streamMax; streamIndex++) {

        // Stream description.
        const QtlMovieStreamInfoPtr& stream(_streams[streamIndex]);

        // Is this stream in the intended audience languages?
        const bool inAudienceLanguages = audienceLanguages.contains(stream->language(), Qt::CaseInsensitive);

        // Type-specific processing.
        switch (stream->streamType()) {
        case QtlMovieStreamInfo::Video: {
            // Compute video frame size (width x height). Zero if width or height undefined.
            const int frameSize = stream->width() * stream->height();
            if (frameSize > highestFrameSize) {
                // Largest frame size so far or first video stream, keep it.
                _selectedVideoStreamIndex = streamIndex;
                highestFrameSize = frameSize;
            }
            break;
        }

        case QtlMovieStreamInfo::Audio: {
            if (firstAudio < 0) {
                // Found first audio track.
                firstAudio = streamIndex;
            }
            if (originalAudio < 0 && !stream->impaired() && stream->isOriginalAudio() && !stream->isDubbedAudio()) {
                // Found the first original audio track.
                originalAudio = streamIndex;
            }
            if (notAudienceAudio < 0 && !stream->impaired() && !inAudienceLanguages) {
                // Found the first audio track not in the audience languages.
                notAudienceAudio = streamIndex;
            }
            if (notImpairedAudio < 0 && !stream->impaired()) {
                // Found the first audio track for non-impaired.
                notImpairedAudio = streamIndex;
            }
            break;
        }

        case QtlMovieStreamInfo::Subtitle: {
            if (inAudienceLanguages) {
                // Keep only subtitles for intended audience.
                if (firstSubtitle < 0) {
                    // Found first subtitles.
                    firstSubtitle = streamIndex;
                }
                if (forcedSubtitle < 0 && stream->forced()) {
                    // Found first forced subtitle.
                    forcedSubtitle = streamIndex;
                }
                if (impairedSubtitle < 0 && stream->impaired()) {
                    // Found first subtitle stream for visual/hearing impaired.
                    impairedSubtitle = streamIndex;
                }
                if (completeSubtitle < 0 && !stream->forced() && !stream->impaired()) {
                    // Found first complete subtitle.
                    completeSubtitle = streamIndex;
                }
            }
            break;
        }

        default:
            // Other stream, ignore it.
            break;
        }
    }

    // Select the default audio stream.
    if (originalAudio >= 0) {
        // Use original audio stream.
        _selectedAudioStreamIndex = originalAudio;
    }
    else if (notAudienceAudio >= 0) {
        // Use first audio stream index not in intended audience languages.
        _selectedAudioStreamIndex = notAudienceAudio;
    }
    else if (notImpairedAudio >= 0) {
        // Use first audio stream index not for hearing/visual impaired.
        _selectedAudioStreamIndex = notImpairedAudio;
    }
    else {
        // Use first audio stream index (if any).
        _selectedAudioStreamIndex = firstAudio;
    }

    // Check if the selected audio stream is in a language for the intended audience.
    // This will impact the selection of the subtitle stream.
    const bool audioInAudienceLanguages =
            _selectedAudioStreamIndex >= 0 &&
            audienceLanguages.contains(_streams[_selectedAudioStreamIndex]->language(), Qt::CaseInsensitive);

    // Select the default subtitle stream.
    if (audioInAudienceLanguages) {
        // Use only "forced" subtitles (if any) when audio is for the intended audience.
        _selectedSubtitleStreamIndex = forcedSubtitle;
    }
    else if (completeSubtitle >= 0) {
        // Use complete subtitles.
        _selectedSubtitleStreamIndex = completeSubtitle;
    }
    else if (impairedSubtitle >= 0) {
        // Use subtitles for hearing/visual impaired.
        _selectedSubtitleStreamIndex = impairedSubtitle;
    }
    else {
        // Use first subtitles stream (if any).
        _selectedSubtitleStreamIndex = firstSubtitle;
    }

    // Debug display.
    _log->debug(tr("Default streams: video: %1, audio: %2, subtitles: %3")
                .arg(_selectedVideoStreamIndex)
                .arg(_selectedAudioStreamIndex)
                .arg(_selectedSubtitleStreamIndex));
}


//----------------------------------------------------------------------------
// Get or set the various stream index to transcode.
//----------------------------------------------------------------------------

int QtlMovieInputFile::selectedVideoStreamIndex() const
{
    return _selectedVideoStreamIndex < streamCount() ? _selectedVideoStreamIndex : -1;
}

void QtlMovieInputFile::setSelectedVideoStreamIndex(int selectedVideoStreamIndex)
{
    _selectedVideoStreamIndex = selectedVideoStreamIndex;
}

int QtlMovieInputFile::selectedAudioStreamIndex() const
{
    return _selectedAudioStreamIndex < streamCount() ? _selectedAudioStreamIndex : -1;
}

void QtlMovieInputFile::setSelectedAudioStreamIndex(int selectedAudioStreamIndex)
{
    _selectedAudioStreamIndex = selectedAudioStreamIndex;
}

int QtlMovieInputFile::selectedSubtitleStreamIndex() const
{
    return _selectedSubtitleStreamIndex < streamCount() ? _selectedSubtitleStreamIndex : -1;
}

void QtlMovieInputFile::setSelectedSubtitleStreamIndex(int selectedSubtitleStreamIndex)
{
    _selectedSubtitleStreamIndex = selectedSubtitleStreamIndex;
}

QString QtlMovieInputFile::externalSubtitleFileName() const
{
    return selectedSubtitleStreamIndex() < 0 ? _externalSubtitleFileName : QString();
}

void QtlMovieInputFile::setExternalSubtitleFileName(const QString& subtitleFileName)
{
    _externalSubtitleFileName = subtitleFileName;
    if (!subtitleFileName.isEmpty()) {
        _selectedSubtitleStreamIndex = -1;
    }
}
