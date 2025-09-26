#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWidget; }
QT_END_NAMESPACE

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(DatabaseManager& dbManager, QWidget* parent = nullptr);
    ~LoginWidget();
    void reset();

signals:
    void loginSuccessful(const User& user);

private slots:
    void attemptLogin();
    void showChangePasswordDialog();

private:
    void setupConnections();

    Ui::LoginWidget *ui;
    DatabaseManager& m_dbManager;
};

#endif // LOGINWIDGET_H 