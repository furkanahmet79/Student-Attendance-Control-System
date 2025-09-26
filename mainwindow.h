#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QByteArray>
#include <QDebug>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStatusBar>
#include <QLabel>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include "databasemanager.h"
#include "loginwidget.h"
#include "adminwidget.h"
#include "teacherwidget.h"
#include "studentwidget.h"

// Protokol sabitleri
#define STX 0x02
#define ETX 0x03
#define PCB 0x00
#define INS_DO 0x3E
#define INS_SET 0x3C
#define INS_GET 0x3D
#define INS_NAK 0x15

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void cardScanned(const QString& uid);

private slots:
    void onLoginSuccessful(const User& user);
    void onLogoutRequested();
    void readSerialData();
    void onAttendanceStarted(int sessionId);
    void onAttendanceEnded();
    void onCardDetected(const QString& uid);
    void teacherCardScanRequested();

private:
    void setupUI();
    void loadStylesheet();
    void setupCardReader();
    void startCardPolling();
    void stopCardPolling();
    void pollCard();
    void onSerialPortReadyRead();
    void processCardResponse(const QByteArray &response);
    void processUid(const QString &uid, int sessionId);
    void showQuickAddDialog(const QString &cardUID, int courseId);
    void showQuickEnrollDialog(const Student &student, int courseId);
    void showWelcomeNotification(const QString& studentName);
    quint8 calculateLRC(const QByteArray &data);
    QByteArray createPollPacket();
    void setupSerialPort();
    void setupConnections();
    void debugDatabaseTables();

    Ui::MainWindow *ui;
    DatabaseManager& dbManager;
    
    // Modüler widget'lar
    LoginWidget* m_loginWidget;
    AdminWidget* m_adminWidget;
    TeacherWidget* m_teacherWidget;
    StudentWidget* m_studentWidget = nullptr;
    QStackedWidget* m_stackedWidget;
    
    // Durum değişkenleri
    User currentUser;
    bool m_doubleClickProcessed = false;
    
    // Kart Okuyucu Donanımı
    QSerialPort *m_serialPort;
    QTimer *m_pollTimer;
    QTimer *m_doubleClickTimer;
    QByteArray m_readBuffer;
    QLabel *m_statusLabel;
    QMediaPlayer* m_successSound;
    QAudioOutput* m_audioOutput;
    
    int m_currentAttendanceSessionId;
    bool m_isAttendanceActive;
    bool m_isCardScanModeActive;
};

#endif // MAINWINDOW_H
