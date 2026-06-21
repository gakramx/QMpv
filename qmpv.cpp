// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2023 Akram Abdeslem Chaima <akram@riseup.net>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qmpv.h"

#include <MpvController>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
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
    // Store the new source URL
    if (m_source != url) {
        m_source = url;
        Q_EMIT sourceChanged();
    }

    // Reset the renderer state
    resetRenderer();

    // apply decryption key always - the server streams encrypted files,
    // external CDN plain .mp4 URLs that don't need it are harmlessly rejected by lavf.
    // We force the 'mov' demuxer and Use massive probesize/analyzeduration for fragmented CENC compatibility.
    // We also provide the decryption_key_id which is essential for matching keys to fragments in a fragmented stream.
    setProperty(QStringLiteral("demuxer-lavf-format"), QStringLiteral("mov"));
    setProperty(QStringLiteral("demuxer-lavf-o"), QStringLiteral("decryption_key=b45b4a1c441d30ea134075e3cde260d3,decryption_key_id=e40c050015175ead3b2de4dd94bd1360,probesize=50000000,analyzeduration=50000000"));
    setProperty(QStringLiteral("demuxer-max-bytes"), QStringLiteral("2048MiB"));
    setProperty(QStringLiteral("demuxer-readahead-secs"), 30);

    // Use a short delay to ensure the renderer has time to reset
    QTimer::singleShot(100, this, [this, url]() {
        qDebug() << "Loading video:" << url.toString();
        Q_EMIT command(QStringList() << QStringLiteral("loadfile") << url.toString());
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
        Q_EMIT pausedChanged();
    } else if (property == QStringLiteral("paused-for-cache") || property == QStringLiteral("core-idle")) {
        bool idle = value.toBool();
        if (m_buffering != idle) {
            m_buffering = idle;
            Q_EMIT bufferingChanged();
        }
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

