#ifndef AUDIOCALL_H
#define AUDIOCALL_H

#include <QDialog>
#include <QMessageBox>
#include <QKeyEvent>

#include <SFML/Audio.hpp>

#include "definitions.h"

namespace Ui {
class AudioCall;
}

class AudioCall : public QDialog
{
    Q_OBJECT
    uint32_t friend_number;
    int16_t *pcm;
    size_t sample_count;
    uint8_t channels = AUDIOCHANNELS;
    uint32_t sampling_rate = AUDIOSAMPLERATE;

    bool RECORD = false;
    sf::Sound sound;
    sf::SoundBufferRecorder recorder;
public:
    explicit AudioCall(QWidget *parent = 0, uint32_t fn = UINT32_MAX);
    ~AudioCall();

signals:
    void sendMessage(uint32_t, QString);
    void sendAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
    void closeConnection();
public slots:
    void reciveMessage(QString);
    void reviveAudioframe(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
    void sendMessage();

private slots:
    void on_pushButton_talk_clicked();

    void on_pushButton_close_clicked();
protected:
    void keyReleaseEvent(QKeyEvent *);
private:
    Ui::AudioCall *ui;
};



#endif // AUDIOCALL_H
