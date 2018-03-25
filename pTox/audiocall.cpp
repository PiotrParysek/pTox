#include "audiocall.h"
#include "ui_audiocall.h"

AudioCall::AudioCall(QWidget *parent, uint32_t fn) :
    QDialog(parent),
    ui(new Ui::AudioCall)
{
    ui->setupUi(this);
    if (!sf::SoundRecorder::isAvailable()) {
        QMessageBox::critical(this, "ERROR", "Cannot stream audio without audio recorder!");
        this->close();
    }
    this->friend_number = fn;
    QObject::connect(this->ui->pushButton_Send, SIGNAL(pressed()), this, SLOT(sendMessage()));
}

AudioCall::~AudioCall()
{
    emit closeConnection();
    delete ui;
}

void AudioCall::reciveMessage(QString TEXT)
{
    this->ui->textBrowser->append(TEXT);
}

void AudioCall::reviveAudioframe(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate)
{
    while (sound.getStatus() == sf::Sound::Playing) { }

    sf::SoundBuffer outBuffer;
    outBuffer.loadFromSamples(pcm, sample_count, channels, sampling_rate);
    sound.setBuffer(outBuffer);
    sound.play();
}

void AudioCall::sendMessage()
{
    emit sendMessage(friend_number,this->ui->textEdit->toPlainText());
    this->ui->textEdit->clear();
}

void AudioCall::on_pushButton_talk_clicked()
{
    if (RECORD) { // There is recording - stop!
        this->ui->pushButton_talk->setText("talk");
        RECORD = !RECORD;

        recorder.stop();

        const sf::SoundBuffer& buffer = recorder.getBuffer();
        emit sendAudioFrame(friend_number, buffer.getSamples(), buffer.getSampleCount(), buffer.getChannelCount(), sampling_rate);
    } else { // There isnot recording - start!
        this->ui->pushButton_talk->setText("STOP!");
        RECORD = !RECORD;

        recorder.start(sampling_rate);
    }
}

void AudioCall::on_pushButton_close_clicked()
{
    emit closeConnection();
    this->close();
}

void AudioCall::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit sendMessage(friend_number, this->ui->textEdit->toPlainText());
        this->ui->textEdit->clear();
    } else if (event->key() == Qt::Key_Control && event->key() == Qt::Key_Q) {
        emit closeConnection();
        this->close();
    }
}

