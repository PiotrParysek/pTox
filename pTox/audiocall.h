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
    /**
     * @brief sendMessage Send message to contact
     */
    void sendMessage(uint32_t, QString);
    /**
     * @brief sendAudioFrame Send audio frame to contact
     * @param friend_number buddy ID
     * @param pcm the array of samples
     * @param sample_count number of samples
     * @param channels number of channels
     * @param sampling_rate number of samples per second
     */
    void sendAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
    /**
     * @brief closeConnection send Close connection communication
     */
    void closeConnection();
public slots:
    /**
     * @brief reciveMessage Recive and display message
     */
    void reciveMessage(QString);
    /**
     * @brief reviveAudioframe Recive and play audio frame
     * @param friend_number buddy ID
     * @param pcm the array of samples
     * @param sample_count number of samples
     * @param channels number of channels
     * @param sampling_rate number of samples per second
     */
    void reviveAudioframe(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
    /**
     * @brief sendMessage Slot to activate sendMessage signal
     */
    void sendMessage();

private slots:
    /**
     * @brief on_pushButton_talk_clicked Start recordifn or Stop recording and send frames to the buddy
     */
    void on_pushButton_talk_clicked();

    /**
     * @brief on_pushButton_close_clicked Close private connection
     */
    void on_pushButton_close_clicked();
protected:
    /**
     * @brief keyReleaseEvent Event to send message by pressing Enter / Return
     */
    void keyReleaseEvent(QKeyEvent *);
private:
    Ui::AudioCall *ui;
};



#endif // AUDIOCALL_H
