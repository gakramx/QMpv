// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2023 Akram Abdeslem Chaima <akram@riseup.net>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qmpv.h"

#include <MpvController>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "KeyHelper.h"
QMpv::QMpv(QQuickItem * parent)
    : MpvAbstractItem(parent)
{
#ifdef Q_OS_ANDROID
    setProperty(QStringLiteral("vo"), "opengl-cb");
#endif
#ifdef Q_OS_LINUX
    // Force embedded rendering on Linux
  //  setProperty(QStringLiteral("vo"), QStringLiteral("opengl"));
#endif
    QString watchLaterLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                 QDir::separator() + "watch-later";
    QDir watchLaterDir(watchLaterLocation);
    if (!watchLaterDir.exists())
        watchLaterDir.mkpath(".");

    qDebug()<<"Filessss :"<<watchLaterDir;
    //setProperty(QStringLiteral("watch-later-directory"), watchLaterLocation);

    setProperty(QStringLiteral("terminal"), QStringLiteral("yes"));
    setProperty(QStringLiteral("save-position-on-quit"), QStringLiteral("yes"));
    setProperty(QStringLiteral("keep-open"), QStringLiteral("always"));
    setProperty(QStringLiteral("cache"), QStringLiteral("yes"));
    setProperty(QStringLiteral("cache-secs"), 2); // pre-buffer 2s before playback
    setProperty(QStringLiteral("demuxer-max-bytes"), 50000000);      // 50MB forward cache
    setProperty(QStringLiteral("demuxer-max-back-bytes"), 5000000); // 5MB back-seek cache
    setProperty(QStringLiteral("force-seekable"), QStringLiteral("yes"));

    observeProperty(QStringLiteral("duration"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("time-pos"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("pause"), MPV_FORMAT_FLAG);
    observeProperty(QStringLiteral("paused-for-cache"), MPV_FORMAT_FLAG);
    observeProperty(QStringLiteral("core-idle"), MPV_FORMAT_FLAG);

    observeProperty(QStringLiteral("path"), MPV_FORMAT_STRING);
    observeProperty(QStringLiteral("speed"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("volume"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("video-aspect"), MPV_FORMAT_DOUBLE);
    connect(mpvController(), &MpvController::propertyChanged, this,
            &QMpv::onPropertyChanged, Qt::QueuedConnection);
}
void QMpv::resetRenderer() {
    // Clear the current video
    Q_EMIT command(QStringList() << QStringLiteral("stop"));

    // Force libmpv as the video output to ensure embedded rendering
    setProperty(QStringLiteral("vo"), QStringLiteral("libmpv"));

    // Schedule a redraw to recreate the renderer
    update();

    // Let Qt know to invalidate the scene graph
}
QQuickFramebufferObject::Renderer *QMpv::createRenderer() const
{
    // This logs when the renderer is created
    qDebug() << "Creating new MPV renderer";

    // Force update to ensure a clean renderer state
    const_cast<QMpv*>(this)->update();

    // Call the parent implementation after our setup
    return MpvAbstractItem::createRenderer();
}

QMpv::~QMpv()
{
}

qreal QMpv::position()
{
    return m_position;
}

qreal QMpv::duration()
{
    return m_duration;
}

bool QMpv::paused()
{
    return m_paused;
}

bool QMpv::buffering()
{
    return m_buffering;
}

void QMpv::play()
{
    if (!paused()) {
        return;
    }
    setProperty(QStringLiteral("pause"), false);
    m_playbackState = PlayingState;

    Q_EMIT playbackStateChanged();
    Q_EMIT pausedChanged();
}

void QMpv::pause()
{
    if (paused()) {
        return;
    }
    setProperty(QStringLiteral("pause"), true);
    m_playbackState = PausedState;

    Q_EMIT playbackStateChanged();
    Q_EMIT pausedChanged();
}

void QMpv::stop() {
    setPosition(0);
    setProperty(QStringLiteral("stop"), true);
    m_stopped = true;
    m_playbackState = StoppedState;

    Q_EMIT playbackStateChanged();
    Q_EMIT stoppedChanged();
}

void QMpv::setPosition(double value)
{
    if (value == position()) {
        return;
    }
    setProperty(QStringLiteral("time-pos"), value);
    Q_EMIT positionChanged();
}

void QMpv::seek(qreal offset)
{
    Q_EMIT command(QStringList() << QStringLiteral("add") << QStringLiteral("time-pos") << QString::number(offset));
}


void QMpv::setSource(const QUrl &url) {
    // Normalize: if the caller passed a plain file path (no scheme), convert it
    // to a proper local file URL so url.isLocalFile() works correctly later.
    QUrl normalizedUrl = url;
    if (url.scheme().isEmpty() && !url.path().isEmpty()) {
        normalizedUrl = QUrl::fromLocalFile(url.path());
    } else if (!url.isValid() || (url.scheme() != "file" && !url.scheme().startsWith("http"))) {
        // Treat any unrecognized string as a local path
        normalizedUrl = QUrl::fromLocalFile(url.toString());
    }

    // Store the new source URL
    if (m_source != normalizedUrl) {
        m_source = normalizedUrl;
        Q_EMIT sourceChanged();
    }

    // Reset the renderer state
    resetRenderer();

    // apply decryption key always - the server streams encrypted files,
    // external CDN plain .mp4 URLs that don't need it are harmlessly rejected by lavf.
    // We force the 'mov' demuxer and Use massive probesize/analyzeduration for fragmented CENC compatibility.
    setProperty(QStringLiteral("demuxer-lavf-format"), QStringLiteral("mov"));
    
    QString kid, cencKey;
    if (!m_masterKey.isEmpty() && m_libraryId > 0) {
        kid = KeyHelper::deriveKid(m_masterKey, m_libraryId).toLower();
        cencKey = KeyHelper::deriveCencKey(m_masterKey, m_libraryId).toLower();
        qDebug() << "QMpv: Using dynamic CENC key for library" << m_libraryId;
    } else {
        qDebug() << "QMpv: WARNING: No master key assigned, using static fallback key!";
        kid = QStringLiteral("e40c050015175ead3b2de4dd94bd1360");
        cencKey = QStringLiteral("b45b4a1c441d30ea134075e3cde260d3");
    }
    
    QString demuxerParams = QString("decryption_key=%1,decryption_key_id=%2,probesize=50000000,analyzeduration=5000000").arg(cencKey, kid);
    setProperty(QStringLiteral("demuxer-lavf-o"), demuxerParams);
    
    setProperty(QStringLiteral("demuxer-max-bytes"), QStringLiteral("2048MiB"));
    setProperty(QStringLiteral("demuxer-readahead-secs"), 30);

    // Use a short delay to ensure the renderer has time to reset
    QTimer::singleShot(100, this, [this, normalizedUrl]() {
        // mpv requires a valid URI. For local files, use the percent-encoded file:// URL.
        // For remote streams (http/https) pass the URL string directly.
        QString uriToLoad;
        if (normalizedUrl.isLocalFile()) {
            // QUrl::toEncoded() produces a properly percent-encoded file:/// URI
            // e.g. "file:///storage/emulated/0/My%20Documents/TestLib/DIM3.mp4"
            uriToLoad = QString::fromUtf8(normalizedUrl.toEncoded());
        } else {
            uriToLoad = normalizedUrl.toString();
        }
        qDebug() << "Loading video:" << uriToLoad;
        Q_EMIT command(QStringList() << QStringLiteral("loadfile") << uriToLoad);
    });
}



QUrl QMpv::source() const {
    return m_source;
}

void QMpv::setplaybackRate(qreal rate){
    qDebug()<<"C++ setplaybackRate : "<<rate;
    if (rate == playbackRate()) {
        return;
    }
    setProperty(QStringLiteral("speed"), rate);
    Q_EMIT playbackRateChanged();
}

qreal QMpv::playbackRate(){
    return m_playbackrate;
}

void QMpv::setVolume(qreal vol){
    qDebug()<<"C++ setvolume : "<<vol;
    if (vol == m_volume) {
        return;
    }

    setProperty(QStringLiteral("volume"), vol*100);
    Q_EMIT volumeChanged();
}

qreal QMpv::volume(){
    return m_volume;
}

QMpv::PlaybackState QMpv::playbackState(){
    return m_playbackState;
}
QMpv::FillMode QMpv::fillMode(){
    return m_fillMode;
}
void QMpv::setFillMode(FillMode mode) {
    switch (mode) {
    case Stretch:
        setProperty(QStringLiteral("video-aspect"), 2.0);
        m_fillMode = PreserveAspectCrop;
        break;
    case PreserveAspectCrop:
        setProperty(QStringLiteral("video-aspect"),1.3333);
        m_fillMode =PreserveAspectFit;
        break;
    case PreserveAspectFit:
        setProperty(QStringLiteral("video-aspect"),1.7777);
        m_fillMode =Stretch;
        break;
    default:
        break;
    }
    Q_EMIT fillModeChanged();
}
void QMpv::updateBuffering()
{
    // Buffering = network ran dry OR mpv core idle without the user having paused
    bool shouldBuffer = m_paused_for_cache || (m_core_idle && !m_paused);
    if (m_buffering != shouldBuffer) {
        m_buffering = shouldBuffer;
        Q_EMIT bufferingChanged();
    }
}
void QMpv::onPropertyChanged(const QString &property, const QVariant &value)
{

    if (property == QStringLiteral("time-pos")) {
        double time = value.toDouble();
        m_position = time;
        Q_EMIT positionChanged();
        if (m_position > 0.0 && m_stopped) {
            m_stopped = false;
            Q_EMIT stoppedChanged();
        }
    } else if (property == QStringLiteral("duration")) {
        double time = value.toDouble();
        m_duration = time;
        Q_EMIT durationChanged();
    } else if (property == QStringLiteral("pause")) {
        m_paused = value.toBool();
        m_playbackState = m_paused ? PausedState : PlayingState;
        Q_EMIT pausedChanged();
        Q_EMIT playbackStateChanged();
        updateBuffering(); // re-evaluate: user-pause should clear the spinner
    } else if (property == QStringLiteral("paused-for-cache")) {
        m_paused_for_cache = value.toBool();
        updateBuffering();
    } else if (property == QStringLiteral("core-idle")) {
        m_core_idle = value.toBool();
        updateBuffering();
    }
    else if (property == QStringLiteral("path")) {
        m_source = value.toString();
        Q_EMIT sourceChanged();
    }else if (property == QStringLiteral("speed")) {
        double rate = value.toDouble();
        m_playbackrate = rate;

    }else if (property == QStringLiteral("volume")) {
        qDebug()<<"C++ volume value : "<<value.toDouble();
        qreal volume =  value.toDouble() / 100;
        qDebug()<<"C++ volume : "<<volume;
        m_volume=volume;
        Q_EMIT volumeChanged();
    }
}

bool QMpv::stopped() { return m_stopped; }

int QMpv::libraryId() const {
    return m_libraryId;
}

void QMpv::setLibraryId(int id) {
    if (m_libraryId != id) {
        m_libraryId = id;
        Q_EMIT libraryIdChanged();
    }
}

QString QMpv::masterKey() const {
    return m_masterKey;
}

void QMpv::setMasterKey(const QString &key) {
    if (m_masterKey != key) {
        m_masterKey = key;
        Q_EMIT masterKeyChanged();
    }
}
