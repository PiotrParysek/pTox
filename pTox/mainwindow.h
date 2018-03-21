#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ptox.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public pTox
{
    Q_OBJECT
private:
    pTox *PTOX;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
    void setName(std::string Name);
    void setStatus(STATUS status);
    void setStatus(std::string StatusMsg);

    void addFriend(uint32_t);
    void removeFriend(uint32_t);
    void clearFriendVector();

    void sendMessage(std::string);
    void sendMessage(uint32_t, std::string);
public slots:
    void changeTable();
    void appendText(QString);
    void friendRequest(QString);
    void friendRequest(std::string, QString);
    void friendChange(uint32_t);
protected:
    void keyReleaseEvent(QKeyEvent *event);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
