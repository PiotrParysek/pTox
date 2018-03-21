#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    struct passwd *pw = getpwuid(getuid());
    this->FilePath = pw->pw_dir;
    this->FilePath += "/.config/pTox.tox";
    std::cout << "FilePath: " << this->FilePath;
    if (fileExists(QString::fromStdString(this->FilePath))) {
        std::cout << "FilePath: " << this->FilePath << std::endl;
        PTOX = new pTox(false, this->FilePath);
    } else {
        QMessageBox::information(this, "New account", "There will be created new account in a default path!");
        std::cout << "FilePath: " << this->FilePath << std::endl;
        PTOX = new pTox(true, this->FilePath);
    }
    ui->setupUi(this);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this->ui->textEdit);

}

MainWindow::~MainWindow()
{
    delete ui;
}
