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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    pTox *PTOX;
    std::string FilePath;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
    void setName(std::string Name);
    void setStatus(pTox::STATUS status);
    void setStatus(std::string StatusMsg);

    void addFriend(uint32_t);
    void removeFriend(uint32_t);
    void clearFriendVector();
    void friendRequest(std::string, std::string);

    void sendMessage(std::string);
    void sendMessage(uint32_t, std::string);
public slots:
    void sendMessage();

    void changeTable();
    void appendText(QString);
    void friendRequestRecived(std::string, QString);
protected:
    void keyReleaseEvent(QKeyEvent *event);
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

private:
    bool fileExists(QString);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
