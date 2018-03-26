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

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this->ui->textEdit); //If pressed Enter - Send message
    QObject::connect(shortcut, SIGNAL(activated()), this, SLOT(sendMessage()));

    QObject::connect(PTOX, SIGNAL(appendText(QString)), this, SLOT(appendText(QString))); //Add notifications / messages
    QObject::connect(PTOX, SIGNAL(changeTable()), this, SLOT(changeTable())); //Change table if something changes

    QObject::connect(this, SIGNAL(setName(std::string)), PTOX, SLOT(setName(std::string)));
    QObject::connect(this, SIGNAL(setStatus(TOX_USER_STATUS)), PTOX, SLOT(setStatus(TOX_USER_STATUS)));
    QObject::connect(this, SIGNAL(setStatus(std::string)), PTOX, SLOT(setStatus(std::string)));

    QObject::connect(PTOX, SIGNAL(friendRequestRecived(std::string,QString)), this, SLOT(friendRequestRecived(std::string,QString)));//New friend request
    QObject::connect(this, SIGNAL(friendRequest(std::string, std::string)), PTOX, SLOT(sendRequest(std::string,std::string)));
    QObject::connect(this, SIGNAL(addFriend(uint32_t)), PTOX, SLOT(addFriend(uint32_t)));
    QObject::connect(this, SIGNAL(removeFriend(uint32_t)), PTOX, SLOT(removeFriend(uint32_t)));

    QObject::connect(this, SIGNAL(sendMessage(std::string)), PTOX, SLOT(sendMessage(std::string)));
    QObject::connect(this, SIGNAL(sendMessage(uint32_t,std::string)), PTOX, SLOT(sendMessage(uint32_t,std::string)));
    QObject::connect(this, SIGNAL(clearFriendVector()), PTOX, SLOT(clearFriendVector()));

    QObject::connect(PTOX, SIGNAL(newAudioCall(uint32_t)), this, SLOT(newAudioCall(uint32_t)));
    QObject::connect(PTOX, SIGNAL(endAudioCall(uint32_t)), this, SLOT(endAudioCall(uint32_t)));

    changeTable();
}

MainWindow::~MainWindow()
{
    delete PTOX;
    delete ui;
}

void MainWindow::sendMessage()
{
    std::string TEXT;
    try {
        TEXT = this->ui->textEdit->toPlainText().toStdString();
        TEXT.erase(std::remove(TEXT.begin(), TEXT.end(), '\n'), TEXT.end());
    } catch (...) {}
    if (!TEXT.empty()) {
        if (this->ui->comboBox->currentIndex() == 0) {
            emit sendMessage(TEXT);
        } else {
            size_t index = this->ui->comboBox->currentIndex()-1;
            QString text = this->ui->tableWidget->item(index, 1)->text();
            uint32_t val = (uint32_t)text.toLongLong();
            emit sendMessage(val, TEXT);
        }
    }
    this->ui->textEdit->clear();
}

void MainWindow::sendMessage(uint32_t val, QString TEXT)
{
    if (TEXT.size() > 1370) {
        QMessageBox::warning(this, "ERROR", "Your message is too long, try again!", QMessageBox::Ok);
    } else {
        emit sendMessage(val, TEXT.toStdString());
    }
}

void MainWindow::changeTable()
{
    this->ui->tableWidget->clear();
    this->ui->comboBox->clear();
    this->ui->tableWidget->setColumnCount(3);
    size_t FriendSize = PTOX->friendVectorSize();
    this->ui->tableWidget->setRowCount(FriendSize);
    this->ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "Nick" << "ID" << "STATUS");
    this->ui->tableWidget->verticalHeader()->hide();
    this->ui->tableWidget->setShowGrid(false);

    this->ui->comboBox->addItem("ALL");

    for (size_t i = 0; i < FriendSize; ++i) {
        pTox::toxFriend f = PTOX->friendVectorData(i);
        QTableWidgetItem *itemName = new QTableWidgetItem(QString::fromStdString(f.friendName));
        QTableWidgetItem *itemID = new QTableWidgetItem(QString("%1").arg(f.friendNumber, 0, 10));
        QTableWidgetItem *itemStatus = new QTableWidgetItem(QString("%1").arg(QString::fromStdString(Status2String(f.friendConnectionStatus))));

        itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
        itemID->setFlags(itemID->flags() & ~Qt::ItemIsEditable);
        itemStatus->setFlags(itemStatus->flags() & ~Qt::ItemIsEditable);

        itemName->setToolTip(QString::fromStdString(f.friendStatus));
        itemID->setToolTip(QString::fromStdString(f.friendStatus));
        itemStatus->setToolTip(QString::fromStdString(f.friendStatus));

        this->ui->tableWidget->setItem(i, 0, itemName);
        this->ui->tableWidget->setItem(i, 1, itemID);
        this->ui->tableWidget->setItem(i, 2, itemStatus);

        this->ui->comboBox->addItem(QString::fromStdString(f.friendName));
    }
    for (int c = 0; c < this->ui->tableWidget->horizontalHeader()->count(); ++c) {
        this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    }
}


void MainWindow::appendText(QString text)
{
    this->ui->textBrowser->append(text);
}

void MainWindow::friendRequestRecived(std::string public_key, QString message)
{
    int ret = QMessageBox::question(this, "New friend?", QString("You have recived new Freiend request with message: %1 Do you accept it?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (ret == QMessageBox::Yes) {
        emit friendRequest(public_key, message.toStdString());
    } else {
        //Send nothing - we won't see his messages :D "Lack of knowlege is blessing"
    }

}

void MainWindow::newAudioCall(uint32_t FriendNumber)
{
    if (isCall) { //There is a call - reject request!
        emit audioCall(FriendNumber, false);
    } else {
        int ret = QMessageBox::question(this, "Incoming call", QString("Friend: %1 wants to connect with you, proceed?"), QMessageBox::Ok | QMessageBox::No, QMessageBox::Ok);
        if (ret == QMessageBox::Ok) {
            emit audioCall(FriendNumber, true);
            startCall(FriendNumber);
        } else {
            emit audioCall(FriendNumber, false);
        }
    }
}

void MainWindow::endAudioCall(uint32_t FriendNumber)
{
    call->close();
    this->appendText(QString("Friend <b>%1</b> ended call!<br />").arg(QString::fromStdString(PTOX->friendName(FriendNumber))));
    isCall = false;
    delete call;
}

void MainWindow::reviveAudioframe(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate)
{
    emit sendAudioFrame(friend_number, pcm, sample_count, channels, sampling_rate);
}

void MainWindow::closeCall()
{
    this->callFriendNumber = UINT32_MAX;
    this->isCall = false;
    while (call->isVisible()) { }
    delete call;
}

bool MainWindow::fileExists(QString path)
{
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    return check_file.exists() && check_file.isFile();
}

std::string MainWindow::Status2String(TOX_USER_STATUS s)
{
    switch (s) {
    case TOX_USER_STATUS_NONE:
        return "AVAILABLE";
    case TOX_USER_STATUS_AWAY:
        return "AWAY";
    case TOX_USER_STATUS_BUSY:
        return "BUSY";
    default:
        return "unknown";
    }
}

void MainWindow::on_actionSet_name_triggered()
{
    QString name;
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
        name = pw->pw_name;

    bool OK = false;
    QString Name = QInputDialog::getText(this, "Name", "Enter your name:", QLineEdit::Normal, name, &OK);
    if (Name.size() > 128) {
        QMessageBox::warning(this, "ERROR Name", "Your chossen name is too long, try again!", QMessageBox::Ok);
    } else if (OK && !Name.isEmpty()) {
        emit setName(Name.toStdString());
    }
}

void MainWindow::on_actionSet_status_triggered()
{
    QDialog *dialog = new QDialog();
    QVBoxLayout *vbox = new QVBoxLayout();

    QHBoxLayout *hbox1 = new QHBoxLayout();
    QComboBox *box = new QComboBox();
    box->addItems(QStringList() << "AVAILABLE" << "AWAY" << "BUSY");
    hbox1->addWidget(new QLabel("Status: "));
    hbox1->addWidget(box);

    QHBoxLayout *hbox2 = new QHBoxLayout();
    QLineEdit *lineEdit = new QLineEdit("I'm using pTOX, it's awesome!");
    hbox2->addWidget(new QLabel("Status: "));
    hbox2->addWidget(lineEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QObject::connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    vbox->addItem(hbox1);
    vbox->addItem(hbox2);
    vbox->addWidget(buttonBox);

    dialog->setLayout(vbox);

    if (dialog->exec() == QDialog::Accepted) {
        emit setStatus((TOX_USER_STATUS)box->currentIndex());
        if (lineEdit->text().size() > 1006) {
            QMessageBox::warning(this, "ERROR Name", "Your chossen status message is too long, try again!", QMessageBox::Ok);
        } else if (!lineEdit->text().isEmpty())
            emit setStatus(lineEdit->text().toStdString());
    }
}

void MainWindow::on_actionView_triggered()
{
    QMessageBox::information(this, "View", QString::fromStdString(PTOX->getConfiguration()), QMessageBox::Ok);
}

void MainWindow::on_actionAdd_new_triggered()
{
    QDialog *dialog = new QDialog();
    dialog->setMinimumSize(400,100);
    QVBoxLayout *vbox = new QVBoxLayout();

    QHBoxLayout *hbox1 = new QHBoxLayout();
    QLineEdit *lineEdit1 = new QLineEdit("Address in HEX"); //ADDRESS
    hbox1->addWidget(new QLabel("ToxID: "));
    hbox1->addWidget(lineEdit1);

    QHBoxLayout *hbox2 = new QHBoxLayout();
    QLineEdit *lineEdit2 = new QLineEdit("I'm using pTOX, it's awesome!");
    hbox2->addWidget(new QLabel("Message: "));
    hbox2->addWidget(lineEdit2);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QObject::connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    vbox->addItem(hbox1);
    vbox->addItem(hbox2);
    vbox->addWidget(buttonBox);

    dialog->setLayout(vbox);

    if (dialog->exec() == QDialog::Accepted) {
        if (lineEdit2->text().isEmpty()) {
            emit friendRequest(lineEdit1->text().toStdString(), "");
        } else {
            emit friendRequest(lineEdit1->text().toStdString(), lineEdit2->text().toStdString());
        }
    }
}

void MainWindow::on_actionRemove_triggered()
{
    QStringList items;
    for (int i = 0; i < this->ui->tableWidget->rowCount(); ++i) {
        QString text;
        text += this->ui->tableWidget->item(i, 0)->text();
        text += "-";
        text += this->ui->tableWidget->item(i, 1)->text();

        items << text;
    }

    bool ok;
    QString item = QInputDialog::getItem(this, "Friend to remove", "Choose friend to remove: ", items, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        QStringList list2 = item.split('-', QString::SkipEmptyParts);
        QString val = list2.at(1);
        emit removeFriend(val.toUInt());
    }
}

void MainWindow::on_actionVersion_triggered()
{

}

void MainWindow::on_actionAuthor_triggered()
{

}

void MainWindow::on_actionelp_triggered()
{

}

void MainWindow::on_actionEXIT_triggered()
{
    delete PTOX;

    this->close();
}

void MainWindow::on_pushButton_clicked()
{
    sendMessage();
}

void MainWindow::on_pushButton_audio_clicked()
{
    //bool one = !isCall;
    bool two = this->ui->comboBox->currentIndex() == 0 ? true : false;
    bool three = false;
    if (!two) {
        three = this->ui->tableWidget->item(this->ui->comboBox->currentIndex()-1, 2)->text() != "AVAILABLE" ? true : false;
    }
    if (!isCall) {
        QMessageBox::critical(this, "ERROR", "You have one call, currently it is impossible to make another call", QMessageBox::Ok);
    } else if (!isCall || two) {
        QMessageBox::critical(this, "ERROR", "You cannot caonnect to 'All' your friends!", QMessageBox::Ok);
    } else if (!isCall || two || three) {
        QMessageBox::critical(this, "ERROR", "You cannot caonnect to 'busy' or 'away' friend!", QMessageBox::Ok);
    } else {
        size_t index = this->ui->comboBox->currentIndex()+1;
        QString text = this->ui->tableWidget->item(index, 1)->text();
        emit newAudioCall((uint32_t)text.toLongLong());
    }
}

void MainWindow::on_pushButton_Clear_clicked()
{
    this->ui->textBrowser->clear();
}

void MainWindow::on_pushButton_Update_clicked()
{
    this->PTOX->updateToxFriendlList();
}

void MainWindow::startCall(uint32_t FriendNumber)
{
    isCall = true;
    this->callFriendNumber = FriendNumber;

    call = new AudioCall(this, this->callFriendNumber);


    QObject::connect(call, SIGNAL(sendAudioFrame(uint32_t,const int16_t*,size_t,uint8_t,uint32_t)), this, SLOT(reviveAudioframe(uint32_t,const int16_t*,size_t,uint8_t,uint32_t)));
    QObject::connect(this, SIGNAL(sendAudioFrame(uint32_t,const int16_t*,size_t,uint8_t,uint32_t)), PTOX, SLOT(reviveAudioframe(uint32_t,const int16_t*,size_t,uint8_t,uint32_t)));

    QObject::connect(call, SIGNAL(sendMessage(uint32_t,QString)), this, SLOT(sendMessage(uint32_t,QString)));
    QObject::connect(call, SIGNAL(closeConnection()), this, SLOT(closeCall()));
    //QObject::connect(, SIGNAL(), , SLOT());

    call->show();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return)) {
        sendMessage();
    }
}

