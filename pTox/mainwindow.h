#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>


#include "ptox.h"
#include "audiocall.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    pTox *PTOX;
    std::string FilePath;

    AudioCall *call = NULL;
    bool isCall = false;
    uint32_t callFriendNumber;
public:
    /**
     * @brief MainWindow Main constructor of GUI and connection between classes
     * @param parent
     */
    explicit MainWindow(QWidget *parent = 0);
    /**
     * Destructor
     */
    ~MainWindow();
signals:
    void setName(std::string Name);
    void setStatus(TOX_USER_STATUS status);
    void setStatus(std::string StatusMsg);

    void addFriend(uint32_t);
    void removeFriend(uint32_t);
    void clearFriendVector();
    void friendRequest(std::string, std::string);

    void sendMessage(std::string);
    void sendMessage(uint32_t, std::string);

    void audioCall(uint32_t, bool);
    void sendAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
public slots:
    void sendMessage();
    void sendMessage(uint32_t, QString);

    void changeTable();
    void appendText(QString);
    void friendRequestRecived(std::string, QString);

    void newAudioCall(uint32_t);
    void endAudioCall(uint32_t);
    void reviveAudioframe(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
    void closeCall();
private slots:
    void on_actionSet_name_triggered();

    void on_actionSet_status_triggered();

    void on_actionView_triggered();

    void on_actionAdd_new_triggered();

    void on_actionRemove_triggered();

    void on_actionVersion_triggered();

    void on_actionAuthor_triggered();

    void on_actionelp_triggered();

    void on_actionEXIT_triggered();

    void on_pushButton_clicked();

    void on_pushButton_audio_clicked();

    void on_pushButton_Clear_clicked();

    void on_pushButton_Update_clicked();

protected:
    void startCall(uint32_t FriendNumber);
    void keyReleaseEvent(QKeyEvent *event);
    bool fileExists(QString);
    std::string Status2String(TOX_USER_STATUS);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
