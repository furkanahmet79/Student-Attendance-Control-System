#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QTableWidget>
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QHBoxLayout>
#include <QFile>
#include <QStackedWidget>
#include <QThread>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QUrl>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , dbManager(DatabaseManager::instance())
    , m_serialPort(new QSerialPort(this))
    , m_pollTimer(new QTimer(this))
    , m_doubleClickTimer(new QTimer(this))
    , m_readBuffer()
    , m_isCardScanModeActive(false)
    , m_successSound(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
{
    qDebug() << "MainWindow yapıcısı başladı";

    // UI dosyasını yükle (sadece temel pencere ayarları için)
    ui->setupUi(this);
    
    // Minimum pencere boyutunu ayarla
    this->setMinimumSize(1000, 800);
    
    // Stil dosyasını yükle
    loadStylesheet();

    // Çift tıklama koruması için timer ayarla
    m_doubleClickTimer->setSingleShot(true);
    m_doubleClickTimer->setInterval(500); // 500ms
    connect(m_doubleClickTimer, &QTimer::timeout, [this]() {
        m_doubleClickProcessed = false;
        qDebug() << "Çift tıklama koruması sıfırlandı.";
    });

    // Ses altyapısını ayarla (Qt6 uyumlu)
    m_successSound->setAudioOutput(m_audioOutput);
    QString soundFilePath = QCoreApplication::applicationDirPath() + "/success.wav";
    qDebug() << "Ses dosyası yolu kontrol ediliyor:" << soundFilePath;
    if (!QFile::exists(soundFilePath)) {
        qWarning() << "!!! SES DOSYASI BULUNAMADI !!! Kontrol edilmesi gereken yol: " << soundFilePath;
    }
    m_successSound->setSource(QUrl::fromLocalFile(soundFilePath));
    m_audioOutput->setVolume(0.8); // 0.0 ile 1.0 arasında

    connect(m_successSound, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, const QString &errorString) {
        qDebug() << "Medya Oynatıcı Hatası:" << error << " - " << errorString;
    });
    connect(m_successSound, &QMediaPlayer::mediaStatusChanged, this, [](QMediaPlayer::MediaStatus status) {
        qDebug() << "Medya Oynatıcı Durumu:" << status;
        if (status == QMediaPlayer::InvalidMedia) {
            qDebug() << "Medya geçersiz veya bulunamadı!";
        }
    });

    qDebug() << "Veritabanı açılıyor";
    if (!dbManager.openDatabase("yoklama_sistemi.db")) {
        qDebug() << "Veritabanı açılamadı!";
        QMessageBox::critical(this, "Veritabanı Hatası", "Veritabanına bağlanılamadı. Lütfen programı yeniden başlatın.");
    } else {
        qDebug() << "Veritabanı başarıyla açıldı";
        
        // Veritabanı şemasını kontrol et
        dbManager.debugDatabaseTables();
        
        setupUI();
        setupCardReader();
    }
    qDebug() << "MainWindow yapıcısı tamamlandı";
}

MainWindow::~MainWindow()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    delete ui;
}

void MainWindow::setupUI()
{
    qDebug() << "setupUI başladı";
    
    // UI dosyasındaki stacked widget'ı kullan
    m_stackedWidget = ui->stackedWidget;
    
    // Placeholder widget'ı kaldır (Qt Designer için eklenmişti)
    if (m_stackedWidget->count() > 0) {
        QWidget* placeholder = m_stackedWidget->widget(0);
        if (placeholder && placeholder->objectName() == "placeholderWidget") {
            m_stackedWidget->removeWidget(placeholder);
            delete placeholder;
        }
    }
    
    // Login widget'ını oluştur ve ekle
    m_loginWidget = new LoginWidget(dbManager, this);
    m_stackedWidget->addWidget(m_loginWidget);
    connect(m_loginWidget, &LoginWidget::loginSuccessful, this, &MainWindow::onLoginSuccessful);
    
    // Admin ve Teacher widget'ları için placeholder'lar oluştur
    QWidget* adminPlaceholder = new QWidget();
    m_stackedWidget->addWidget(adminPlaceholder);
    
    QWidget* teacherPlaceholder = new QWidget();
    m_stackedWidget->addWidget(teacherPlaceholder);
    
    // Başlangıçta login widget'ını göster
    m_stackedWidget->setCurrentWidget(m_loginWidget);
    
    qDebug() << "setupUI tamamlandı";
}

void MainWindow::loadStylesheet()
{
    QFile file("styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        this->setStyleSheet(styleSheet);
        qDebug() << "Stil dosyası (styles.qss) başarıyla yüklendi";
    } else {
        qDebug() << "styles.qss dosyası bulunamadı, varsayılan stil kullanılıyor";
        // Varsayılan stil
    QString styleSheet = R"(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                       stop:0 #f5f7fa, stop:1 #c3cfe2);
            color: #2c3e50;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
        }

        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498db, stop:1 #2980b9);
            border: none;
            border-radius: 8px;
            color: white;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 10pt;
            min-height: 20px;
        }

        QLineEdit {
            background: white;
            border: 2px solid #bdc3c7;
            border-radius: 6px;
            padding: 8px;
            font-size: 10pt;
            }
        )";
    this->setStyleSheet(styleSheet);
    }
}

void MainWindow::onLoginSuccessful(const User& user)
{
    currentUser = user;
    
    if (user.role == "admin") {
        // Admin widget'ını güncelle ve göster
        m_adminWidget = new AdminWidget(dbManager, user, this);
        m_stackedWidget->removeWidget(m_stackedWidget->widget(1));
        m_stackedWidget->insertWidget(1, m_adminWidget);
        m_stackedWidget->setCurrentWidget(m_adminWidget);
        connect(m_adminWidget, &AdminWidget::logoutRequested, this, &MainWindow::onLogoutRequested);
        
        statusBar()->showMessage(QString("Hoşgeldiniz, %1 (Admin)").arg(user.fullName));
    } else if (user.role == "teacher") {
        // Öğretmen widget'ını güncelle ve göster
        m_teacherWidget = new TeacherWidget(dbManager, user, this);
        m_stackedWidget->removeWidget(m_stackedWidget->widget(2));
        m_stackedWidget->insertWidget(2, m_teacherWidget);
        m_stackedWidget->setCurrentWidget(m_teacherWidget);
        connect(m_teacherWidget, &TeacherWidget::logoutRequested, this, &MainWindow::onLogoutRequested);
        connect(m_teacherWidget, &TeacherWidget::attendanceStarted, this, &MainWindow::onAttendanceStarted);
        connect(m_teacherWidget, &TeacherWidget::attendanceEnded, this, &MainWindow::onAttendanceEnded);
        connect(m_teacherWidget, &TeacherWidget::cardScanRequested, this, &MainWindow::teacherCardScanRequested);
    } else if (user.role == "student") {
        // Öğrenci widget'ını oluştur ve göster
        m_studentWidget = new StudentWidget(user, this);
        if (m_stackedWidget->count() < 4) {
            m_stackedWidget->addWidget(m_studentWidget);
        } else {
            m_stackedWidget->removeWidget(m_stackedWidget->widget(3));
            m_stackedWidget->insertWidget(3, m_studentWidget);
        }
        m_stackedWidget->setCurrentWidget(m_studentWidget);
        connect(m_studentWidget, &StudentWidget::logoutRequested, this, &MainWindow::onLogoutRequested);
        statusBar()->showMessage(QString("Hoşgeldiniz, %1 (Öğrenci)").arg(user.fullName));
    }
}

void MainWindow::onLogoutRequested()
{
    // Login widget'ına geri dön
    m_stackedWidget->setCurrentWidget(m_loginWidget);
    m_loginWidget->reset();
    
    // Kart okuyucuyu durdur
    stopCardPolling();
    
    // Durumu temizle
    currentUser = User();
    statusBar()->clearMessage();
    
    qDebug() << "Kullanıcı çıkış yaptı";
}

// ===================================================================
//   KART OKUYUCU MANTIĞI
// ===================================================================

void MainWindow::setupCardReader()
{
    qDebug() << "Kart okuyucu kurulumu başlıyor...";
    
    m_serialPort->setPortName("COM10"); 
    m_serialPort->setBaudRate(QSerialPort::Baud115200); // Eski kodda 115200 kullanılıyordu
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    qDebug() << "Seri port ayarları yapıldı: COM10, 115200 baud";

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Seri port başarıyla açıldı!";
        
        connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::onSerialPortReadyRead);
        
        // Poll timer'ı ayarla ama başlatma
        m_pollTimer->setInterval(2000); // 2 saniyede bir
        connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::pollCard);
        
        qDebug() << "Seri port COM10 açıldı ve sinyaller bağlandı.";
        qDebug() << "Kart okuyucu hazır - yoklama başladığında aktif olacak.";
    } else {
        qDebug() << "Seri port açılamadı:" << m_serialPort->errorString();
        QMessageBox::critical(this, "Port Hatası", "Kart okuyucu portu (COM10) açılamadı.");
    }
}

void MainWindow::startCardPolling()
{
    if (m_serialPort->isOpen() && !m_pollTimer->isActive()) {
        m_pollTimer->start(2000); 
        qDebug() << "Kart okuyucu periyodik yoklama başlatıldı.";
    }
}

void MainWindow::stopCardPolling()
{
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
        qDebug() << "Kart okuyucu periyodik yoklama durduruldu.";
    }
}

void MainWindow::pollCard()
{
    if (!m_serialPort->isOpen()) {
        qDebug() << "Poll denemesi: Port kapalı.";
        return;
    }
    
    QByteArray pollPacket = createPollPacket();
    qDebug() << "Kart yoklama komutu gönderiliyor:" << pollPacket.toHex(' ').toUpper();
    m_serialPort->write(pollPacket);
}

void MainWindow::onSerialPortReadyRead()
{
    m_readBuffer.append(m_serialPort->readAll());
    
    if (m_readBuffer.startsWith(STX) && m_readBuffer.endsWith(ETX)) {
        qDebug() << "Tam bir paket alındı:" << m_readBuffer.toHex(' ').toUpper();
        processCardResponse(m_readBuffer);
        m_readBuffer.clear();
    } else {
        qDebug() << "Kısmi veri alındı, bekleniyor:" << m_readBuffer.toHex(' ').toUpper();
    }
}

void MainWindow::processCardResponse(const QByteArray &response)
{
    QByteArray uidTag;
    uidTag.append(static_cast<char>(0xDF));
    uidTag.append(static_cast<char>(0x0D));

    int tagIndex = response.indexOf(uidTag);
    if (tagIndex != -1) {
        int uidLengthIndex = tagIndex + uidTag.length();
        if (response.length() > uidLengthIndex) {
            int uidLength = response.at(uidLengthIndex);
            int uidStartIndex = uidLengthIndex + 1;
            
            if (response.length() >= uidStartIndex + uidLength) {
                QByteArray uidBytes = response.mid(uidStartIndex, uidLength);
                QString uidHex = QString(uidBytes.toHex(' ').toUpper());
                qDebug() << "UID Bulundu:" << uidHex;
                
                // Kart okuma modu aktif mi kontrol et
                if (m_isCardScanModeActive) {
                    // Kart UID'sini sinyal olarak gönder
                    emit cardScanned(uidHex);
                    m_isCardScanModeActive = false;
                    qDebug() << "Kart UID gönderildi:" << uidHex;
                    return;
                }
                
                // Aktif yoklama var mı kontrol et
                if (currentUser.role == "teacher") {
                    int activeSessionId = dbManager.getActiveSessionId(currentUser.id);
                    if (activeSessionId > 0) {
                        processUid(uidHex, activeSessionId);
                    } else {
                        qDebug() << "Aktif yoklama bulunamadı";
                    }
                }
            }
        }
    } else {
        qDebug() << "Gelen yanıtta UID (DF 0D) bulunamadı.";
    }
}

void MainWindow::processUid(const QString &uid, int sessionId)
{
    qDebug() << "İşlenen UID:" << uid << "Session ID:" << sessionId;

    // Eğer bu bir "öğrenci ekle" işlemiyse (sessionId < 0)
    if (sessionId < 0 && m_isCardScanModeActive) {
        int currentCourseId = m_teacherWidget->getCurrentCourseIdForEnrollment();
        if (currentCourseId <= 0) {
            QMessageBox::warning(this, "Hata", "Öğrenci eklemek için lütfen önce bir ders seçin.");
            m_isCardScanModeActive = false;
            return;
        }

        // Öğrencinin bu derse zaten kayıtlı olup olmadığını kontrol et
        if (dbManager.isStudentEnrolled(uid, currentCourseId)) {
            QMessageBox::information(this, "Zaten Kayıtlı", "Bu öğrenci seçili derse zaten kayıtlıdır.");
            m_isCardScanModeActive = false; // Tarama modunu kapat
            return; // Fonksiyondan çık
        }

        // Öğrenciyi veritabanında ara
        Student student = dbManager.getStudentByCardUID(uid);
        if (student.id > 0) {
            // Öğrenci sistemde var ama derse kayıtlı değil, derse kaydet
            showQuickEnrollDialog(student, currentCourseId);
        } else {
            // Öğrenci sistemde hiç yok, yeni öğrenci olarak ekle
            showQuickAddDialog(uid, currentCourseId);
        }
        m_isCardScanModeActive = false; // İşlem bitti, modu kapat
        return;
    }

    // Normal yoklama işlemi
    if (m_isAttendanceActive && sessionId > 0) {
        Student student = dbManager.getStudentByCardUID(uid);

        if (student.id > 0) {
            // Yoklamanın yapıldığı dersin ID'sini al
            int courseId = dbManager.getCourseIdForSession(sessionId);
            if (courseId <= 0) {
                qWarning() << "Aktif yoklama otumuru için ders bulunamadı: " << sessionId;
                QMessageBox::critical(this, "Sistem Hatası", "Yoklama oturumu için ders bilgisi alınamadı.");
                return;
            }

            // Öğrencinin bu derse kayıtlı olup olmadığını kontrol et
            if (dbManager.isStudentEnrolled(uid, courseId)) {
                // Yoklamaya kaydet
                int rowsAffected = 0;
                if (dbManager.markStudentPresent(sessionId, student.id, rowsAffected)) {
                    if (rowsAffected > 0) {
                        showWelcomeNotification(student.firstName + " " + student.lastName);
                        // Teacher widget'a yoklama listesini güncellemesi için sinyal gönder
                        if (m_teacherWidget) {
                            m_teacherWidget->updateAttendanceList();
                        }
                    } else {
                         statusBar()->showMessage(QString("%1 %2 zaten yoklamada mevcut.").arg(student.firstName, student.lastName), 3000);
                    }
                } else {
                    qWarning() << "Yoklama kaydedilemedi, öğrenci ID:" << student.id;
                }
            } else {
                // Öğrenci derse kayıtlı değilse uyar
                QMessageBox::warning(this, "Derse Kayıtlı Değil",
                    QString("Öğrenci '%1 %2' bu derse kayıtlı değildir ve yoklamaya eklenemez.")
                    .arg(student.firstName, student.lastName));
            }
        } else {
            QMessageBox::warning(this, "Bilinmeyen Kart",
                QString("Bu kart (%1) sistemde kayıtlı değil!\nÖğrenciyi önce sisteme eklemeniz gerekiyor.")
                .arg(uid));
        }
    }
}

void MainWindow::showQuickAddDialog(const QString &cardUID, int courseId)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Yeni Öğrenci Ekle");
    dialog->setModal(true);
    dialog->setFixedSize(400, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QLabel* infoLabel = new QLabel(QString("Kart UID: %1").arg(cardUID));
    infoLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(infoLabel);
    
    QLabel* studentNumberLabel = new QLabel("Öğrenci Numarası:");
    studentNumberLabel->setMinimumWidth(120);
    QLineEdit* studentNumberEdit = new QLineEdit();
    QHBoxLayout* studentNumberLayout = new QHBoxLayout();
    studentNumberLayout->addWidget(studentNumberLabel);
    studentNumberLayout->addWidget(studentNumberEdit);
    layout->addLayout(studentNumberLayout);
    
    QLabel* firstNameLabel = new QLabel("Ad:");
    firstNameLabel->setMinimumWidth(120);
    QLineEdit* firstNameEdit = new QLineEdit();
    QHBoxLayout* firstNameLayout = new QHBoxLayout();
    firstNameLayout->addWidget(firstNameLabel);
    firstNameLayout->addWidget(firstNameEdit);
    layout->addLayout(firstNameLayout);
    
    QLabel* lastNameLabel = new QLabel("Soyad:");
    lastNameLabel->setMinimumWidth(120);
    QLineEdit* lastNameEdit = new QLineEdit();
    QHBoxLayout* lastNameLayout = new QHBoxLayout();
    lastNameLayout->addWidget(lastNameLabel);
    lastNameLayout->addWidget(lastNameEdit);
    layout->addLayout(lastNameLayout);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* saveButton = new QPushButton("Kaydet ve Derse Kaydet");
    QPushButton* cancelButton = new QPushButton("İptal");
    
    connect(saveButton, &QPushButton::clicked, [this, dialog, cardUID, courseId, 
            studentNumberEdit, firstNameEdit, lastNameEdit]() {
        
        QString studentNumber = studentNumberEdit->text().trimmed();
        QString firstName = firstNameEdit->text().trimmed();
        QString lastName = lastNameEdit->text().trimmed();
        
        if (studentNumber.isEmpty() || firstName.isEmpty() || lastName.isEmpty()) {
            QMessageBox::warning(dialog, "Uyarı", "Lütfen tüm alanları doldurun.");
            return;
        }
        
        // Öğrenciyi ekle ve derse kaydet
        if (courseId > 0) {
            Student newStudent = dbManager.addNewStudentAndEnroll(
                cardUID, studentNumber, firstName, lastName, currentUser.id, courseId);
            
            if (newStudent.id > 0) {
                // Öğrenciyi yoklamaya ekle
                int rowsAffected;
                if (dbManager.markStudentPresent(m_currentAttendanceSessionId, newStudent.id, rowsAffected)) {
                    QMessageBox::information(dialog, "Başarılı", 
                        QString("Öğrenci başarıyla eklendi ve yoklamaya kaydedildi:\n%1 %2 (%3)")
                        .arg(newStudent.firstName, newStudent.lastName, newStudent.studentNumber));
                    dialog->accept();
                    startCardPolling(); // Kart okumayı tekrar başlat
                    
                    // Öğretmene yoklama güncelleme sinyali gönder
                    if (m_teacherWidget) {
                        m_teacherWidget->updateAttendanceList();
                    }
                } else {
                    QMessageBox::critical(dialog, "Hata", "Öğrenci yoklamaya eklenirken hata oluştu.");
                }
            } else {
                QMessageBox::critical(dialog, "Hata", "Öğrenci eklenirken hata oluştu.");
            }
        } else {
            QMessageBox::critical(dialog, "Hata", "Ders bilgisi alınamadı.");
        }
    });
    
    connect(cancelButton, &QPushButton::clicked, [this, dialog]() {
        dialog->reject();
        startCardPolling(); // Kart okumayı tekrar başlat
    });
    
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}

void MainWindow::showQuickEnrollDialog(const Student &student, int courseId)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Öğrenciyi Derse Kaydet");
    dialog->setModal(true);
    dialog->setFixedSize(400, 200);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QLabel* infoLabel = new QLabel(QString("Öğrenci: %1 %2 (%3)\nBu derse kayıtlı değil. Kaydetmek ister misiniz?")
                                  .arg(student.firstName, student.lastName, student.studentNumber));
    infoLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(infoLabel);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* enrollButton = new QPushButton("Derse Kaydet");
    QPushButton* cancelButton = new QPushButton("İptal");
    
    connect(enrollButton, &QPushButton::clicked, [this, dialog, student, courseId]() {
        if (dbManager.enrollStudentToCourse(student.id, courseId, currentUser.id)) {
            QMessageBox::information(dialog, "Başarılı", 
                QString("Öğrenci %1 %2 derse kaydedildi.").arg(student.firstName, student.lastName));
            dialog->accept();
            startCardPolling(); // Kart okumayı tekrar başlat
            
            // Öğretmene yoklama güncelleme sinyali gönder
            if (m_teacherWidget) {
                m_teacherWidget->updateAttendanceList();
            }
        } else {
            QMessageBox::critical(dialog, "Hata", "Öğrenci derse kaydedilirken hata oluştu.");
        }
    });
    
    connect(cancelButton, &QPushButton::clicked, [this, dialog]() {
        dialog->reject();
        startCardPolling(); // Kart okumayı tekrar başlat
    });
    
    buttonLayout->addWidget(enrollButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}

quint8 MainWindow::calculateLRC(const QByteArray &data)
{
    quint8 lrc = 0;
    for (char byte : data) {
        lrc ^= byte;
    }
    return lrc;
}

QByteArray MainWindow::createPollPacket()
{
    // Başarılı testlerdeki çalışan komut yapısı:
    // STX | LEN | PCB | INS | DATA... | LRC | ETX
    // 02  | 0A  | 00  | 3E  | DF7E0100| 96  | 03

    QByteArray data;
    data.append(static_cast<char>(0xDF));
    data.append(static_cast<char>(0x7E));
    data.append(static_cast<char>(0x01));
    data.append(static_cast<char>(0x00));

    // LRC ve ETX hariç paketi oluştur
    QByteArray packet;
    packet.append(static_cast<char>(STX));                                 // STX
    packet.append(static_cast<char>(data.length() + 6)); // LEN: PCB,INS,DATA,LRC,ETX + STX = 6
    packet.append(static_cast<char>(PCB));              // PCB
    packet.append(INS_DO);                              // INS
    packet.append(data);                                // DATA

    // LRC'yi PCB'den itibaren hesapla
    packet.append(calculateLRC(packet.mid(2)));      // LRC
    packet.append(ETX);                                 // ETX
    
    // PDF'e göre LEN, STX ve ETX hariç geri kalan her şeyin uzunluğu.
    // PCB(1) + INS(1) + DATA(4) + LRC(1) = 7
    // Bu yüzden LEN'i 7 yapalım.
    packet[1] = static_cast<char>(data.length() + 3); // PCB+INS+LRC = 3
    
    // LRC'yi de yeni uzunluğa göre yeniden hesapla
    packet[packet.size() - 2] = calculateLRC(packet.mid(2, packet.size() - 4));


    // Çalışan komut: 02 0A 00 3E DF 7E 01 00 96 03
    // Bu komutta LEN=10, yani tüm paketin uzunluğu. LRC, STX dahil geri kalan her şeyin XOR'u.
    // Bunu deneyelim.
    
    QByteArray finalPacket;
    finalPacket.append(STX); // 0x02
    quint8 totalLength = 1 + 1 + 1 + 1 + data.length() + 1 + 1; // STX+LEN+PCB+INS+DATA+LRC+ETX
    finalPacket.append(static_cast<char>(totalLength)); // LEN = 0x0A
    finalPacket.append(static_cast<char>(PCB)); // PCB = 0x00
    finalPacket.append(INS_DO); // INS = 0x3E
    finalPacket.append(data);   // DATA = DF 7E 01 00

    // LRC, STX'ten itibaren tüm baytların XOR'u
    finalPacket.append(calculateLRC(finalPacket));
    finalPacket.append(ETX);

    return finalPacket;
}

void MainWindow::teacherCardScanRequested()
{
    // Kart okuma modunu aktifleştir
    m_isCardScanModeActive = true;
    
    // Kart okuma başlat
    if (m_serialPort->isOpen() && !m_pollTimer->isActive()) {
        // Tek seferlik kart okuma
        QByteArray pollPacket = createPollPacket();
        qDebug() << "Öğrenci ekleme için kart okuma başlatıldı";
        m_serialPort->write(pollPacket);
        
        // Kart okuma modunu 10 saniye sonra otomatik kapat
        QTimer::singleShot(10000, this, [this]() {
            if (m_isCardScanModeActive) {
                m_isCardScanModeActive = false;
                qDebug() << "Kart okuma zaman aşımı";
            }
        });
    } else {
        qDebug() << "Kart okuyucu açık değil veya zaten aktif";
    }
}

void MainWindow::readSerialData()
{
    // Bu fonksiyon, onSerialPortReadyRead fonksiyonunun yerine kullanılacak
    // veya ihtiyaç duyulursa buraya kod eklenebilir
    if (m_serialPort && m_serialPort->isOpen()) {
        m_readBuffer.append(m_serialPort->readAll());
        
        // STX ve ETX arasındaki veriyi işle
        if (m_readBuffer.startsWith(STX) && m_readBuffer.endsWith(ETX)) {
            qDebug() << "Tam bir paket alındı:" << m_readBuffer.toHex(' ').toUpper();
            processCardResponse(m_readBuffer);
            m_readBuffer.clear();
        } else {
            qDebug() << "Kısmi veri alındı, bekleniyor:" << m_readBuffer.toHex(' ').toUpper();
        }
    }
}

void MainWindow::onAttendanceStarted(int sessionId)
{
    qDebug() << "Yoklama başlatıldı, session ID:" << sessionId;
    m_currentAttendanceSessionId = sessionId;
    m_isAttendanceActive = true;
    startCardPolling();
    statusBar()->showMessage(QString("Yoklama aktif. Session ID: %1").arg(sessionId), 5000);
}

void MainWindow::onAttendanceEnded()
{
    qDebug() << "Yoklama sonlandırıldı.";
    m_isAttendanceActive = false;
    stopCardPolling();
    statusBar()->showMessage("Yoklama sona erdi.", 5000);
}

void MainWindow::onCardDetected(const QString& uid)
{
    // Bu fonksiyon, kart algılandığında çağrılabilir
    // Şu anda processCardResponse içinde doğrudan işlem yapıyoruz
    // İhtiyaç duyulursa buraya kod eklenebilir
    qDebug() << "Kart algılandı, UID:" << uid;
}

void MainWindow::showWelcomeNotification(const QString &studentName)
{
    // Sesi çal
    if (m_successSound->playbackState() == QMediaPlayer::PlayingState) {
        m_successSound->setPosition(0);
    }
    m_successSound->play();

    // Görsel bildirim
    QLabel* notificationLabel = new QLabel(QString("Hoş Geldin, %1!").arg(studentName), this);
    notificationLabel->setObjectName("notificationLabel");
    notificationLabel->setAlignment(Qt::AlignCenter);
    
    // Stil ayarları
    notificationLabel->setStyleSheet(
        "QLabel#notificationLabel {"
        "  background-color: rgba(46, 204, 113, 0.9);"
        "  color: white;"
        "  font-size: 24pt;"
        "  font-weight: bold;"
        "  padding: 20px;"
        "  border-radius: 15px;"
        "}"
    );

    notificationLabel->adjustSize();
    notificationLabel->move((this->width() - notificationLabel->width()) / 2, (this->height() - notificationLabel->height()) / 2);
    notificationLabel->show();

    // 2 saniye sonra etiketi yok et
    QTimer::singleShot(2000, notificationLabel, &QLabel::deleteLater);
}
