// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2023 Akram Abdeslem Chaima <akram@riseup.net>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QMPV_H
#define QMPV_H

#include <MpvAbstractItem>
#include <QQuickFramebufferObject>


class QMpv : public MpvAbstractItem
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool stopped READ stopped NOTIFY stoppedChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setplaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)

    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };
    Q_ENUM(QMpv::PlaybackState)
    enum FillMode {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop
    };
    Q_ENUM(QMpv::FillMode)


public:

    QMpv(QQuickItem * parent = 0);
    virtual ~QMpv();

    qreal position();
    qreal duration();
    QUrl source() const;
    bool paused();
    bool stopped();
    qreal playbackRate();
    qreal volume();
    PlaybackState playbackState();
    FillMode fillMode();

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void setPosition(double value);
    void seek(qreal offset);
    void setSource(const QUrl &url);
    void setplaybackRate(qreal rate);
    void setVolume(qreal vol);
    void setFillMode(FillMode mode);

Q_SIGNALS:
    void positionChanged();
    void durationChanged();
    void pausedChanged();
    void stoppedChanged();
    void sourceChanged();
    void playbackRateChanged();
    void volumeChanged();
    void playbackStateChanged();
    void fillModeChanged();

private:
    void onPropertyChanged(const QString &property, const QVariant &value);
    bool m_paused = true;
    qreal m_position = 0;
    qreal m_duration = 0;
    bool m_stopped = true;
    QUrl m_source;
    qreal m_playbackrate = 1.0;
    qreal m_volume = 1.0;
    PlaybackState m_playbackState;
    FillMode m_fillMode=Stretch;

};
#endif // QMPV_H
