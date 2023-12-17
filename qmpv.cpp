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
    setProperty(QStringLiteral("terminal"), QStringLiteral("yes"));
    setProperty(QStringLiteral("keep-open"), QStringLiteral("always"));

    observeProperty(QStringLiteral("duration"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("time-pos"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("pause"), MPV_FORMAT_FLAG);

    observeProperty(QStringLiteral("path"), MPV_FORMAT_STRING);
    observeProperty(QStringLiteral("speed"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("volume"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("video-aspect"), MPV_FORMAT_DOUBLE);
    connect(mpvController(), &MpvController::propertyChanged, this,
            &QMpv::onPropertyChanged, Qt::QueuedConnection);
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
    // Convert the input QString to QUrl
    // QUrl url = QUrl::fromUserInput(url);

    // Check if the source has changed
    if (m_source != url) {
        m_source = url;
        Q_EMIT sourceChanged();
    }

    // Use the QUrl directly when calling the command
    qDebug()<<m_source.toLocalFile();
    Q_EMIT command(QStringList() << QStringLiteral("loadfile") << m_source.toLocalFile());
    //    Q_EMIT command(QStringList() << QStringLiteral("loadfile") << "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4");

    // Update the playback state
    //  m_playbackState = PlayingState;
    // Q_EMIT playbackStateChanged();
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

