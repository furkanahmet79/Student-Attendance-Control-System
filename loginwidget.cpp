#include "loginwidget.h"
#include "./ui_loginwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>
#include <QTextEdit>

LoginWidget::LoginWidget(DatabaseManager& dbManager, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    setupConnections();
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::setupConnections()
{
    // Giriş butonu ve Enter tuşu bağlantıları
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWidget::attemptLogin);
    connect(ui->passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::attemptLogin);
    connect(ui->changePasswordButton, &QPushButton::clicked, this, &LoginWidget::showChangePasswordDialog);
}

void LoginWidget::reset()
{
    ui->usernameEdit->clear();
    ui->passwordEdit->clear();
    ui->usernameEdit->setFocus();
}

void LoginWidget::attemptLogin()
{
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text();
    QString userType = ui->userTypeComboBox->currentText();
    
    if (userType == "Öğrenci") {
        if (username.isEmpty()) {
            QMessageBox::warning(this, "Uyarı", "Lütfen öğrenci numaranızı girin.");
            return;
        }
        User user;
        QVariant result = m_dbManager.authenticateStudent(username, password, user);
        if (result.toBool()) {
            emit loginSuccessful(user);
            reset();
        } else {
            QMessageBox::critical(this, "Hata", "Geçersiz öğrenci numarası.");
            ui->passwordEdit->clear();
            ui->passwordEdit->setFocus();
        }
        return;
    }
    // Admin ve öğretmen için mevcut akış
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Uyarı", "Lütfen kullanıcı adı ve şifrenizi girin.");
        return;
    }
    User user;
    QVariant result = m_dbManager.authenticateUser(username, password, user);
    if (result.toBool()) {
        // Rol kontrolü ekle
        if ((userType == "Admin" && user.role != "admin") ||
            (userType == "Öğretmen" && user.role != "teacher")) {
            QMessageBox::warning(this, "Hata", "Seçili rol ile kullanıcı rolü uyuşmuyor!");
            return;
        }
        emit loginSuccessful(user);
        reset();
    } else {
        QMessageBox::critical(this, "Hata", "Geçersiz kullanıcı adı veya şifre.");
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
    }
}

void LoginWidget::showChangePasswordDialog()
{
    QString userType = ui->userTypeComboBox->currentText();
    if (userType == "Öğrenci") {
        QMessageBox::information(this, "Şifre Değiştirme", "Öğrenci şifre değiştirme özelliği devre dışı bırakıldı.");
        return;
    }
    // ... mevcut öğretmen/admin şifre değiştir kodu ...
} 