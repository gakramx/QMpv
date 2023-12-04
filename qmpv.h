// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2023 Akram Abdeslem Chaima <akram@riseup.net>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QMPV_H
#define QMPV_H

#include <QQuickFramebufferObject>

#include <mpv/client.h>
#include <mpv/render_gl.h>


class MpvRenderer;

class QMpv : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool stopped READ stopped NOTIFY stoppedChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setplaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setvolume NOTIFY volumeChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)

    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };
    Q_ENUM(QMpv::PlaybackState)

    mpv_handle *mpv;
    mpv_render_context *mpv_gl;

    friend class MpvRenderer;

public:
    static void on_update(void *ctx);

    QMpv(QQuickItem * parent = 0);
    virtual ~QMpv();
    Renderer *createRenderer() const override;

    qreal position();
    qreal duration();
    QString source();
    bool paused();
    bool stopped();
    qreal playbackRate();
    qreal volume();
    PlaybackState playbackState();

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void setPosition(double value);
    void seek(qreal offset);
    void command(const QVariant& params);
    void setOption(const QString &name, const QVariant &value);
    void setProperty(const QString &name, const QVariant &value);
    void setSource(const QString &file);
    QVariant getProperty(const QString& name);
    void setplaybackRate(qreal rate);
    void setvolume(qreal vol);

Q_SIGNALS:
    void positionChanged();
    void durationChanged();
    void pausedChanged();
    void onUpdate();
    void stoppedChanged();
    void sourceChanged();
    void playbackRateChanged();
    void volumeChanged();
    void playbackStateChanged();

private Q_SLOTS:
    void onMpvEvents();
    void doUpdate();

private:
    bool m_paused = true;
    qreal m_position = 0;
    qreal m_duration = 0;
    bool m_stopped = true;
    QString m_source;
    qreal m_playbackrate = 1.0;
    qreal m_volume = 1.0;
    PlaybackState m_playbackState;

};
#endif // QMPV_H
