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
// Define the QtlMovieSettings class, the description of the global settings
// of the application.
//
//----------------------------------------------------------------------------

#include "QtlMovieSettings.h"
#include "QtlStringList.h"
#include "QtlMovie.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

QtlMovieSettings::QtlMovieSettings(QtlLogger* log, QObject* parent) :
    QObject(parent),
    _isModified(false),
    _log(log),
    _defaultFileName(QDir::homePath() + QDir::separator() + ".qtlmovie"),
    _fileError(),
    _ffmpegDefault(new QtlMovieExecFile("FFmpeg",
                                        "http://ffmpeg.org/",
                                        "http://ffmpeg.zeranoe.com/builds/",
                                        "ffmpeg",
                                        QStringList("-version"),
                                        QString(),
                                        QString(),
                                        log,
                                        this)),
    _ffmpegExplicit(new QtlMovieExecFile(*_ffmpegDefault, "", this)),
    _ffprobeDefault(new QtlMovieExecFile("FFprobe",
                                         "http://ffmpeg.org/",
                                         "http://ffmpeg.zeranoe.com/builds/",
                                         "ffprobe",
                                         QStringList("-version"),
                                         QString(),
                                         QString(),
                                         log,
                                         this)),
    _ffprobeExplicit(new QtlMovieExecFile(*_ffprobeDefault, "", this)),
    _dvdauthorDefault(new QtlMovieExecFile("DvdAuthor",
                                           "http://dvdauthor.sourceforge.net/",
                                           "http://www.paehl.com/open_source/?DVDAuthor",
                                           "dvdauthor",
                                           QStringList("-h"),
                                           QString(),
                                           "INFO: ",
                                           log,
                                           this)),
    _dvdauthorExplicit(new QtlMovieExecFile(*_dvdauthorDefault, "", this)),
    _mkisofsDefault(new QtlMovieExecFile("Mkisofs",
                                         "http://cdrecord.berlios.de/",
                                         "http://fy.chalmers.se/~appro/linux/DVD+RW/tools/win32/",
                                         "mkisofs",
                                         QStringList("-version"),
                                         QString(),
                                         QString(),
                                         log,
                                         this)),
    _mkisofsExplicit(new QtlMovieExecFile(*_mkisofsDefault, "", this)),
    _growisofsDefault(new QtlMovieExecFile("Growisofs",
                                           "http://freecode.com/projects/dvdrw-tools",
                                           "http://fy.chalmers.se/~appro/linux/DVD+RW/tools/win32/",
                                           "growisofs",
                                           QStringList("-version"),
                                           QString(),
                                           QString(),
                                           log,
                                           this)),
    _growisofsExplicit(new QtlMovieExecFile(*_growisofsDefault, "", this)),
    _telxccDefault(new QtlMovieExecFile("Telxcc",
                                        "https://github.com/forers/telxcc",
                                        QString(),
                                        "telxcc",
                                        QStringList("-h"),
                                        QString(),
                                        "Usage:",
                                        log,
                                        this)),
    _telxccExplicit(new QtlMovieExecFile(*_telxccDefault, "", this)),
    _dvddecrypterDefault(new QtlMovieExecFile("DVD Decrypter",
                                              "http://www.videohelp.com/tools/DVD-Decrypter",
                                              "http://www.videohelp.com/download/SetupDVDDecrypter_3.5.4.0.exe",
                                              "dvddecrypter",
                                              QStringList(),
                                              QString(),
                                              QString(),
                                              log,
                                              this)),
    _dvddecrypterExplicit(new QtlMovieExecFile(*_dvddecrypterDefault, "", this)),
    _maxLogLines(1000),
    _initialInputDir(),
    _defaultOutDirIsInput(true),
    _defaultOutDir(),
    _dvdBurner(),
    _audienceLanguages(),
    _transcodeComplete(true),
    _transcodeSeconds(0),
    _dvdVideoBitRate(QTL_DVD_DEFAULT_VIDEO_BITRATE),
    _ipadVideoBitRate(QTL_IPAD_DEFAULT_VIDEO_BITRATE),
    _keepIntermediateFiles(false),
    _ffmpegProbeSeconds(200),
    _srtUseVideoSizeHint(true),
    _chapterMinutes(5),
    _dvdRemuxAfterTranscode(true),
    _createPalDvd(true),
    _ipadScreenSize(Ipad12Size),
    _forceDvdTranscode(false)
{
    Q_ASSERT(log != 0);

    // Default audience is French-speaking.
    _audienceLanguages << "fr" << "fre" << "fra" << "french";
    normalize(_audienceLanguages);
}


//----------------------------------------------------------------------------
// Write an XML element with a "value" integer attribute.
//----------------------------------------------------------------------------

void QtlMovieSettings::setIntAttribute(QXmlStreamWriter& xml, const QString& name, int value)
{
    xml.writeStartElement(name);
    xml.writeAttribute("value", QString::number(value));
    xml.writeEndElement();
}


//----------------------------------------------------------------------------
// Write an XML element with a "value" string attribute.
//----------------------------------------------------------------------------

void QtlMovieSettings::setStringAttribute(QXmlStreamWriter& xml, const QString& name, const QString& value, const QString& type)
{
    xml.writeStartElement(name);
    if (!type.isEmpty()) {
        xml.writeAttribute("type", type);
    }
    xml.writeAttribute("value", value);
    xml.writeEndElement();
}


//----------------------------------------------------------------------------
// Write an XML element with a "value" boolean attribute.
//----------------------------------------------------------------------------

void QtlMovieSettings::setBoolAttribute(QXmlStreamWriter& xml, const QString& name, bool value)
{
    xml.writeStartElement(name);
    xml.writeAttribute("value", value ? "true": "false");
    xml.writeEndElement();
}


//----------------------------------------------------------------------------
// Decode an XML element with a "value" integer attribute.
//----------------------------------------------------------------------------

bool QtlMovieSettings::getIntAttribute(QXmlStreamReader& xml, const QString& name, int& value)
{
    if (xml.name() != name) {
        // Not the expected element.
        return false;
    }

    // Get the attribute value, skip the rest of the element.
    const QString stringValue(xml.attributes().value("value").toString().trimmed());
    xml.skipCurrentElement();

    // Decode the attribute value (if one was found).
    bool ok = false;
    const int intValue = stringValue.toInt(&ok, 0);
    if (stringValue.isEmpty()) {
        xml.raiseError(tr("No attribute \"%1\" in element <%2>").arg("value").arg(name));
    }
    else if (ok) {
        value = intValue;
    }
    else {
        xml.raiseError(tr("Invalid integer value: %1").arg(stringValue));
    }
    return ok;
}


//----------------------------------------------------------------------------
// Decode an XML element with a "value" string attribute.
//----------------------------------------------------------------------------

bool QtlMovieSettings::getStringAttribute(QXmlStreamReader& xml, const QString& name, QString& value)
{
    if (xml.name() != name) {
        // Not the expected element.
        return false;
    }

    // Get the attribute value, skip the rest of the element.
    value = xml.attributes().value("value").toString();
    xml.skipCurrentElement();
    return true;
}


//----------------------------------------------------------------------------
// Decode an XML element with "type" and "value" string attributes.
//----------------------------------------------------------------------------

bool QtlMovieSettings::getStringAttribute(QXmlStreamReader& xml, const QString& name, QMap<QString, QString>& valueMap)
{
    if (xml.name() != name) {
        // Not the expected element.
        return false;
    }

    // Get the attributes, skip the rest of the element.
    const QString type(xml.attributes().value("type").toString());
    const QString value(xml.attributes().value("value").toString());
    xml.skipCurrentElement();

    // Check that the type attribute is defined.
    if (type.isEmpty()) {
        xml.raiseError(tr("No attribute \"%1\" in element <%2>").arg("tyoe").arg(name));
        return false;
    }
    else {
        valueMap[type] = value;
        return true;
    }
}


//----------------------------------------------------------------------------
// Decode an XML element with a "value" boolean attribute.
//----------------------------------------------------------------------------

bool QtlMovieSettings::getBoolAttribute(QXmlStreamReader& xml, const QString& name, bool& value)
{
    if (xml.name() != name) {
        // Not the expected element.
        return false;
    }

    // Get the attribute value in lowercase, skip the rest of the element.
    const QString stringValue(xml.attributes().value("value").toString().toLower().trimmed());
    xml.skipCurrentElement();

    // Decode the attribute value (if one was found).
    if (stringValue.isEmpty()) {
        xml.raiseError(tr("No attribute \"%1\" in element <%2>").arg("value").arg(name));
        return false;
    }
    else if (stringValue == "true") {
        value = true;
        return true;
    }
    else if (stringValue == "false") {
        value = false;
        return true;
    }
    else {
        xml.raiseError(tr("Invalid boolean value: %1").arg(stringValue));
        return false;
    }
}


//----------------------------------------------------------------------------
// Save the settings in an XML file.
//----------------------------------------------------------------------------

bool QtlMovieSettings::save(const QString& fileName)
{
    // Clear error.
    _fileError.clear();

    // Save the file.
    const QString actualFileName (fileName.isEmpty() ? _defaultFileName : fileName);
    QFile file(actualFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        _fileError = tr("Error creating file %1").arg(actualFileName);
        return false;
    }

    // Write the content as XML.
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    xml.writeStartElement("QtlMovieSettings");
    xml.writeAttribute("version", "1.0");

    // Add an entry for each options.
    setStringAttribute(xml, "ffmpegExecutable", _ffmpegExplicit->fileName());
    setStringAttribute(xml, "ffprobeExecutable", _ffprobeExplicit->fileName());
    setStringAttribute(xml, "dvdauthorExecutable", _dvdauthorExplicit->fileName());
    setStringAttribute(xml, "mkisofsExecutable", _mkisofsExplicit->fileName());
    setStringAttribute(xml, "growisofsExecutable", _growisofsExplicit->fileName());
    setStringAttribute(xml, "telxccExecutable", _telxccExplicit->fileName());
    setStringAttribute(xml, "dvddecrypterExecutable", _dvddecrypterExplicit->fileName());
    setIntAttribute(xml, "maxLogLines", _maxLogLines);
    setStringAttribute(xml, "initialInputDir", _initialInputDir);
    setBoolAttribute(xml, "defaultOutDirIsInput", _defaultOutDirIsInput);
    foreach (const QString& key, _defaultOutDir.keys()) {
        setStringAttribute(xml, "defaultOutDir", _defaultOutDir[key], key);
    }
    setStringAttribute(xml, "dvdBurner", _dvdBurner);
    setStringAttribute(xml, "audienceLanguages", _audienceLanguages.join(','));
    setBoolAttribute(xml, "transcodeComplete", _transcodeComplete);
    setIntAttribute(xml, "transcodeSeconds", _transcodeSeconds);
    setIntAttribute(xml, "dvdVideoBitRate", _dvdVideoBitRate);
    setIntAttribute(xml, "ipadVideoBitRate", _ipadVideoBitRate);
    setBoolAttribute(xml, "keepIntermediateFiles", _keepIntermediateFiles);
    setIntAttribute(xml, "ffmpegProbeSeconds", _ffmpegProbeSeconds);
    setBoolAttribute(xml, "srtUseVideoSizeHint", _srtUseVideoSizeHint);
    setIntAttribute(xml, "chapterMinutes", _chapterMinutes);
    setBoolAttribute(xml, "dvdRemuxAfterTranscode", _dvdRemuxAfterTranscode);
    setBoolAttribute(xml, "createPalDvd", _createPalDvd);
    setIntAttribute(xml, "ipadScreenSize", int(_ipadScreenSize));
    setBoolAttribute(xml, "forceDvdTranscode", _forceDvdTranscode);

    // Finalize the XML document.
    xml.writeEndElement();
    xml.writeEndDocument();
    file.close();

    // Project is safe on disk.
    _isModified = false;

    return true;
}


//----------------------------------------------------------------------------
// Load the settings from an XML file.
//----------------------------------------------------------------------------

bool QtlMovieSettings::load(const QString& fileName)
{
    // Clear error.
    _fileError.clear();

    // Open the file.
    const QString actualFileName(fileName.isEmpty() ? _defaultFileName : fileName);
    QFile file(actualFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _fileError = tr("Error opening file %1").arg(actualFileName);
        return false;
    }

    // Default values.
    QString ffmpegExecutable;
    QString ffprobeExecutable;
    QString dvdauthorExecutable;
    QString mkisofsExecutable;
    QString growisofsExecutable;
    QString telxccExecutable;
    QString dvddecrypterExecutable;
    int maxLogLines =_maxLogLines;
    QString initialInputDir(_initialInputDir);
    bool defaultOutDirIsInput = _defaultOutDirIsInput;
    QMap<QString,QString> defaultOutDir(_defaultOutDir);
    QString dvdBurner(_dvdBurner);
    QString audienceLanguages(_audienceLanguages.join(','));
    bool transcodeComplete = _transcodeComplete;
    int transcodeSeconds = _transcodeSeconds;
    int dvdVideoBitRate = _dvdVideoBitRate;
    int ipadVideoBitRate = _ipadVideoBitRate;
    bool keepIntermediateFiles = _keepIntermediateFiles;
    int ffmpegProbeSeconds = _ffmpegProbeSeconds;
    bool srtUseVideoSizeHint = _srtUseVideoSizeHint;
    int chapterMinutes = _chapterMinutes;
    bool dvdRemuxAfterTranscode = _dvdRemuxAfterTranscode;
    bool createPalDvd = _createPalDvd;
    int ipadScreenSize = int(_ipadScreenSize);
    bool forceDvdTranscode = _forceDvdTranscode;

    // Read the XML document.
    QXmlStreamReader xml(&file);
    if (xml.readNextStartElement()) {
        // Check the value of the root element.
        if (xml.name() != "QtlMovieSettings" || xml.attributes().value("version") != "1.0") {
            xml.raiseError(tr("The file is not a valid QtlMovie settings file"));
        }
        else {
            // Read all settings.
            while (xml.readNextStartElement()) {
                if (!getStringAttribute(xml, "ffmpegExecutable", ffmpegExecutable) &&
                    !getStringAttribute(xml, "ffprobeExecutable", ffprobeExecutable) &&
                    !getStringAttribute(xml, "dvdauthorExecutable", dvdauthorExecutable) &&
                    !getStringAttribute(xml, "mkisofsExecutable", mkisofsExecutable) &&
                    !getStringAttribute(xml, "growisofsExecutable", growisofsExecutable) &&
                    !getStringAttribute(xml, "telxccExecutable", telxccExecutable) &&
                    !getStringAttribute(xml, "dvddecrypterExecutable", dvddecrypterExecutable) &&
                    !getIntAttribute(xml, "maxLogLines", maxLogLines) &&
                    !getStringAttribute(xml, "initialInputDir", initialInputDir) &&
                    !getBoolAttribute(xml, "defaultOutDirIsInput", defaultOutDirIsInput) &&
                    !getStringAttribute(xml, "defaultOutDir", defaultOutDir) &&
                    !getStringAttribute(xml, "dvdBurner", dvdBurner) &&
                    !getStringAttribute(xml, "audienceLanguages", audienceLanguages) &&
                    !getBoolAttribute(xml, "transcodeComplete", transcodeComplete) &&
                    !getIntAttribute(xml, "transcodeSeconds", transcodeSeconds) &&
                    !getIntAttribute(xml, "dvdVideoBitRate", dvdVideoBitRate) &&
                    !getIntAttribute(xml, "ipadVideoBitRate", ipadVideoBitRate) &&
                    !getBoolAttribute(xml, "keepIntermediateFiles", keepIntermediateFiles) &&
                    !getIntAttribute(xml, "ffmpegProbeSeconds", ffmpegProbeSeconds) &&
                    !getBoolAttribute(xml, "srtUseVideoSizeHint", srtUseVideoSizeHint) &&
                    !getIntAttribute(xml, "chapterMinutes", chapterMinutes) &&
                    !getBoolAttribute(xml, "dvdRemuxAfterTranscode", dvdRemuxAfterTranscode) &&
                    !getBoolAttribute(xml, "createPalDvd", createPalDvd) &&
                    !getIntAttribute(xml, "ipadScreenSize", ipadScreenSize) &&
                    !getBoolAttribute(xml, "forceDvdTranscode", forceDvdTranscode) &&
                    !xml.error()) {
                    // Unexpected element, ignore it.
                    xml.skipCurrentElement();
                }
            }
        }
    }

    // Process end of XML stream.
    const bool ok = !xml.error();
    if (ok) {
        // Project is safe on disk.
        _isModified = false;

        // Apply values.
        setFFmpegExplicitExecutable(ffmpegExecutable);
        setFFprobeExplicitExecutable(ffprobeExecutable);
        setDvdAuthorExplicitExecutable(dvdauthorExecutable);
        setMkisofsExplicitExecutable(mkisofsExecutable);
        setGrowisofsExplicitExecutable(growisofsExecutable);
        setTelxccExplicitExecutable(telxccExecutable);
        setDvdDecrypterExplicitExecutable(dvddecrypterExecutable);
        setMaxLogLines(maxLogLines);
        setInitialInputDir(initialInputDir);
        setDefaultOutputDirIsInput(defaultOutDirIsInput);
        setAudienceLanguages(audienceLanguages.split(',', QString::SkipEmptyParts));
        foreach (const QString& type, defaultOutDir.keys()) {
            setDefaultOutputDir(type, defaultOutDir[type]);
        }
        setDvdBurner(dvdBurner);
        setTranscodeComplete(transcodeComplete);
        setTranscodeSeconds(transcodeSeconds);
        setDvdVideoBitRate(dvdVideoBitRate);
        setIpadVideoBitRate(ipadVideoBitRate);
        setKeepIntermediateFiles(keepIntermediateFiles);
        setFFmpegProbeSeconds(ffmpegProbeSeconds);
        setSrtUseVideoSizeHint(srtUseVideoSizeHint);
        setChapterMinutes(chapterMinutes);
        setDvdRemuxAfterTranscode(dvdRemuxAfterTranscode);
        setCreatePalDvd(createPalDvd);
        setIpadScreenSize(IpadScreenSize(ipadScreenSize));
        setForceDvdTranscode(forceDvdTranscode);
    }
    else {
        // Format an error string.
        _fileError = tr("Error loading %1\n%2\nLine %3, column %4")
                .arg(actualFileName)
                .arg(xml.errorString())
                .arg(xml.lineNumber())
                .arg(xml.columnNumber());
    }

    file.close();
    return ok;
}


//----------------------------------------------------------------------------
// Get the HTML description of the actual FFmpeg and FFprobe products.
//----------------------------------------------------------------------------

QString QtlMovieSettings::ffmpegHtmlDescription() const
{
    return (_ffmpegExplicit->isSet() ? _ffmpegExplicit->htmlDescription() : _ffmpegDefault->htmlDescription()) +
            (_ffprobeExplicit->isSet() ? _ffprobeExplicit->htmlDescription() : _ffprobeDefault->htmlDescription());
}


//----------------------------------------------------------------------------
// Report missing media tools in the log.
//----------------------------------------------------------------------------

void QtlMovieSettings::reportMissingTools() const
{
    if (!ffmpeg()->isSet()) {
        _log->line(tr("Error searching for FFmpeg, install FFmpeg or set explicit path in settings."));
    }
    if (!ffprobe()->isSet()) {
        _log->line(tr("Error searching for FFprobe, install FFmpeg or set explicit path in settings."));
    }
    if (!dvdauthor()->isSet()) {
        _log->line(tr("Error searching for DvdAuthor, install it or set explicit path in settings."));
    }
    if (!mkisofs()->isSet()) {
        _log->line(tr("Error searching for mkisofs, install it or set explicit path in settings."));
    }
    if (!growisofs()->isSet()) {
        _log->line(tr("Error searching for growisofs, install it or set explicit path in settings."));
    }
    if (!telxcc()->isSet()) {
        _log->line(tr("Error searching for telxcc, install it or set explicit path in settings."));
    }
    if (!dvddecrypter()->isSet()) {
        _log->line(tr("Error searching for DVD Decrypter, install it or set explicit path in settings."));
    }
}


//----------------------------------------------------------------------------
// "Normalize" a string list: lower case, sorted.
//----------------------------------------------------------------------------

void QtlMovieSettings::normalize(QStringList& list)
{
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
        *it = it->toLower().trimmed();
    }
    list.sort();
}


//----------------------------------------------------------------------------
// Set the various elementary properties.
//----------------------------------------------------------------------------

void QtlMovieSettings::setFFmpegExplicitExecutable(const QString& ffmpegExecutable)
{
    if (_ffmpegExplicit->setFileName(ffmpegExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setFFprobeExplicitExecutable(const QString& ffprobeExecutable)
{
    if (_ffprobeExplicit->setFileName(ffprobeExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setDvdAuthorExplicitExecutable(const QString& dvdauthorExecutable)
{
    if (_dvdauthorExplicit->setFileName(dvdauthorExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setMkisofsExplicitExecutable(const QString& mkisofsExecutable)
{
    if (_mkisofsExplicit->setFileName(mkisofsExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setGrowisofsExplicitExecutable(const QString& growisofsExecutable)
{
    if (_growisofsExplicit->setFileName(growisofsExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setTelxccExplicitExecutable(const QString& telxccExecutable)
{
    if (_telxccExplicit->setFileName(telxccExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setDvdDecrypterExplicitExecutable(const QString& dvddecrypterExecutable)
{
    if (_dvddecrypterExplicit->setFileName(dvddecrypterExecutable)) {
        _isModified = true;
    }
}

void QtlMovieSettings::setMaxLogLines(int maxLogLines)
{
    if (_maxLogLines != maxLogLines) {
        _maxLogLines = maxLogLines;
        _isModified = true;
    }
}

QString QtlMovieSettings::initialInputDir() const
{
    // The default initial input directory is user's home.
    return _initialInputDir.isEmpty() ? QtlFile::absoluteNativeFilePath(QDir::homePath()) : _initialInputDir;
}

void QtlMovieSettings::setInitialInputDir(const QString& initialInputDir)
{
    if (_initialInputDir != initialInputDir) {
        _initialInputDir = initialInputDir;
        _isModified = true;
    }
}

void QtlMovieSettings::setDefaultOutputDirIsInput(bool defaultOutDirIsInput)
{
    if (_defaultOutDirIsInput != defaultOutDirIsInput) {
        _defaultOutDirIsInput = defaultOutDirIsInput;
        _isModified = true;
    }
}

QString QtlMovieSettings::defaultOutputDir(const QString& outputType, bool force) const
{
    QMap<QString,QString>::ConstIterator it = _defaultOutDir.find(outputType);
    return (_defaultOutDirIsInput && !force) || it == _defaultOutDir.end() ? "" : it.value();
}

void QtlMovieSettings::setDefaultOutputDir(const QString& outputType, const QString& defaultOutDir)
{
    if (defaultOutDir != defaultOutputDir(outputType)) {
        _defaultOutDir[outputType] = defaultOutDir;
        _isModified = true;
    }
}

void QtlMovieSettings::setAudienceLanguages(const QStringList& audienceLanguages)
{
    // Apply a normalized list.
    QStringList languages(audienceLanguages);
    normalize(languages);
    if (_audienceLanguages != languages) {
        _audienceLanguages = languages;
        _isModified = true;
    }
}

void QtlMovieSettings::setTranscodeSeconds(int transcodeSeconds)
{
    if (_transcodeSeconds != transcodeSeconds) {
        _transcodeSeconds = transcodeSeconds;
        _isModified = true;
    }
}

void QtlMovieSettings::setTranscodeComplete(bool transcodeComplete)
{
    if (_transcodeComplete != transcodeComplete) {
        _transcodeComplete = transcodeComplete;
        _isModified = true;
    }
}

void QtlMovieSettings::setDvdVideoBitRate(int dvdVideoBitRate)
{
    if (_dvdVideoBitRate != dvdVideoBitRate) {
        _dvdVideoBitRate = dvdVideoBitRate;
        _isModified = true;
    }
}

void QtlMovieSettings::setIpadVideoBitRate(int ipadVideoBitRate)
{
    if (_ipadVideoBitRate != ipadVideoBitRate) {
        _ipadVideoBitRate = ipadVideoBitRate;
        _isModified = true;
    }
}

void QtlMovieSettings::setKeepIntermediateFiles(bool keepIntermediateFiles)
{
    if (_keepIntermediateFiles != keepIntermediateFiles) {
        _keepIntermediateFiles = keepIntermediateFiles;
        _isModified = true;
    }
}

void QtlMovieSettings::setFFmpegProbeSeconds(int ffmpegProbeSeconds)
{
    if (_ffmpegProbeSeconds != ffmpegProbeSeconds) {
        _ffmpegProbeSeconds = ffmpegProbeSeconds;
        _isModified = true;
    }
}

void QtlMovieSettings::setSrtUseVideoSizeHint(bool srtUseVideoSizeHint)
{
    if (_srtUseVideoSizeHint != srtUseVideoSizeHint) {
        _srtUseVideoSizeHint = srtUseVideoSizeHint;
        _isModified = true;
    }
}

void QtlMovieSettings::setChapterMinutes(int chapterMinutes)
{
    if (_chapterMinutes != chapterMinutes) {
        _chapterMinutes = chapterMinutes;
        _isModified = true;
    }
}

void QtlMovieSettings::setDvdRemuxAfterTranscode(bool dvdRemuxAfterTranscode)
{
    if (_dvdRemuxAfterTranscode != dvdRemuxAfterTranscode) {
        _dvdRemuxAfterTranscode = dvdRemuxAfterTranscode;
        _isModified = true;
    }
}

void QtlMovieSettings::setDvdBurner(const QString& dvdBurner)
{
    if (_dvdBurner != dvdBurner) {
        _dvdBurner = dvdBurner;
        _isModified = true;
    }
}

void QtlMovieSettings::setCreatePalDvd(bool createPalDvd)
{
    if (_createPalDvd != createPalDvd) {
        _createPalDvd = createPalDvd;
        _isModified = true;
    }
}

void QtlMovieSettings::setIpadScreenSize(QtlMovieSettings::IpadScreenSize ipadScreenSize)
{
    if (_ipadScreenSize != ipadScreenSize) {
        _ipadScreenSize = ipadScreenSize;
        _isModified = true;
    }
}

void QtlMovieSettings::setForceDvdTranscode(bool forceDvdTranscode)
{
    if (_forceDvdTranscode != forceDvdTranscode) {
        _forceDvdTranscode = forceDvdTranscode;
        _isModified = true;
    }
}
