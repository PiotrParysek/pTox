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

    QObject::connect(PTOX, SIGNAL(changeTable()), this, SLOT(changeTable())); //Change table if something changes
    QObject::connect(PTOX, SIGNAL(friendRequestRecived(std::string,QString)), this, SLOT(friendRequestRecived(std::string,QString)));
    QObject::connect(PTOX, SIGNAL(appendText(QString)), this, SLOT(appendText(QString))); //Add notification

    QObject::connect(this, SIGNAL(setName(std::string)), PTOX, SLOT(setName(std::string)));
    QObject::connect(this, SIGNAL(setStatus(pTox::STATUS)), PTOX, SLOT(setStatus(pTox::STATUS)));
    QObject::connect(this, SIGNAL(setStatus(std::string)), PTOX, SLOT(setStatus(std::string)));
    QObject::connect(this, SIGNAL(friendRequest(std::string, std::string)), PTOX, SLOT(sendRequest(std::string,std::string)));
    QObject::connect(this, SIGNAL(addFriend(uint32_t)), PTOX, SLOT(addFriend(uint32_t)));
    QObject::connect(this, SIGNAL(removeFriend(uint32_t)), PTOX, SLOT(removeFriend(uint32_t)));
    QObject::connect(this, SIGNAL(sendMessage(std::string)), PTOX, SLOT(sendMessage(std::string)));
    QObject::connect(this, SIGNAL(sendMessage(uint32_t,std::string)), PTOX, SLOT(sendMessage(uint32_t,std::string)));
    QObject::connect(this, SIGNAL(clearFriendVector()), PTOX, SLOT(clearFriendVector()));

    changeTable();
}

MainWindow::~MainWindow()
{
    delete PTOX;
    delete ui;
}

void MainWindow::sendMessage()
{
    std::string TEXT = this->ui->textEdit->toPlainText().toStdString();
    TEXT.erase(std::remove(TEXT.begin(), TEXT.end(), '\n'), TEXT.end());
    if (!TEXT.empty()) {
        if (this->ui->comboBox->currentIndex() == 0) {
            emit sendMessage(TEXT);
        } else {
            size_t index = this->ui->comboBox->currentIndex()+1;
            QString text = this->ui->tableWidget->item(index, 1)->text();
            uint32_t val = (uint32_t)text.toLongLong();
            emit sendMessage(val, TEXT);
        }
    }
    this->ui->textEdit->clear();
}

void MainWindow::changeTable()
{
    this->ui->tableWidget->clear();
    this->ui->comboBox->clear();
    this->ui->tableWidget->setColumnCount(2);
    size_t FriendSize = PTOX->friendVectorSize();
    this->ui->tableWidget->setRowCount(FriendSize);
    this->ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "Nick" << "ID");
    this->ui->tableWidget->verticalHeader()->hide();
    this->ui->tableWidget->setShowGrid(false);

    this->ui->comboBox->addItem("ALL");

    for (size_t i = 0; i < FriendSize; ++i) {
        pTox::toxFriend f = PTOX->friendVectorData(i);
        QTableWidgetItem *itemName = new QTableWidgetItem(QString::fromStdString(f.friendName));
        QTableWidgetItem *itemID = new QTableWidgetItem(QString("%1").arg(f.friendNumber, 0, 10));
        switch (f.friendConnectionStatus) {
        case pTox::AVAILABLE:
            itemName->setBackground(Qt::green);
            itemID->setBackground(Qt::green);
            break;
        case pTox::AWAY:
            itemName->setBackground(Qt::blue);
            itemID->setBackground(Qt::blue);
            break;
        case pTox::BUSY:
            itemName->setBackground(Qt::red);
            itemID->setBackground(Qt::red);
            break;
        }
        itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
        itemID->setFlags(itemID->flags() & ~Qt::ItemIsEditable);
        this->ui->tableWidget->setItem(i, 0, itemName);
        this->ui->tableWidget->setItem(i, 1, itemID);
        this->ui->comboBox->addItem(QString::fromStdString(f.friendName));
    }
}


void MainWindow::appendText(QString text)
{
    this->ui->textBrowser->append(text);
}

void MainWindow::friendRequestRecived(std::string public_key, QString message)
{
    emit friendRequest(std::string, std::string);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return)) {
        sendMessage();
    }
}

bool MainWindow::fileExists(QString path)
{
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    return check_file.exists() && check_file.isFile();
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
    if (OK && !Name.isEmpty()) {
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
        emit setStatus((pTox::STATUS)box->currentIndex());
        if (!lineEdit->text().isEmpty())
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
    QVBoxLayout *vbox = new QVBoxLayout();

    QHBoxLayout *hbox1 = new QHBoxLayout();
    QLineEdit *lineEdit1 = new QLineEdit("76518406F6A9F2217E8DC487CC783C25CC16A15EB36FF32E335A235342C48A39218F515C39A6"); //ADDRESS
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
