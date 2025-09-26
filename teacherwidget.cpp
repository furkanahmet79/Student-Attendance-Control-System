#include "teacherwidget.h"
#include "./ui_teacherwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QHeaderView>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include "tablehelper.h"
#include <QDebug>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include "mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>

TeacherWidget::TeacherWidget(DatabaseManager& dbManager, const User& user, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TeacherWidget)
    , m_dbManager(dbManager)
    , m_currentUser(user)
    , m_currentSessionId(-1)
{
    qDebug() << ">>> TeacherWidget yapıcısı başladı, öğretmen:" << user.fullName << "ID:" << user.id;
    ui->setupUi(this);
    setupUI();
    loadData();
    qDebug() << ">>> TeacherWidget yapıcısı tamamlandı";
}

TeacherWidget::~TeacherWidget()
{
    delete ui;
}

void TeacherWidget::setupUI()
{
    // Başlık güncelle
    ui->titleLabel->setText(QString("Öğretmen Paneli - %1").arg(m_currentUser.fullName));
    
    // Tablo başlıklarını ayarla
    setupTableHeaders();
    
    // Sinyal bağlantılarını kur
    setupConnections();
    
    // HistoryTable için satır yüksekliğini ayarla
    ui->historyTable->verticalHeader()->setDefaultSectionSize(45);
    
    // EnrolledStudentsTable için satır yüksekliğini ayarla
    ui->enrolledStudentsTable->verticalHeader()->setDefaultSectionSize(40);
}

void TeacherWidget::setupTableHeaders()
{
    // UI dosyasında zaten tanımlanmış, sadece özellikleri ayarla
    ui->currentAttendanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->currentAttendanceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->currentAttendanceTable->setAlternatingRowColors(true);
    ui->currentAttendanceTable->setSortingEnabled(true);
    
    ui->historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->historyTable->setAlternatingRowColors(true);
    
    // Kayıtlı öğrenciler tablosu için ayarlar
    ui->enrolledStudentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->enrolledStudentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->enrolledStudentsTable->setAlternatingRowColors(true);
    ui->enrolledStudentsTable->setSortingEnabled(true);
    
    // Çift tıklama ile detay görüntüleme
    connect(ui->historyTable, &QTableWidget::cellDoubleClicked, 
            this, &TeacherWidget::onHistoryTableDoubleClicked);
}

void TeacherWidget::setupConnections()
{
    // Ders seçimi değiştiğinde buton durumunu güncelle ve öğrenci listesini yükle
    connect(ui->courseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWidget::onCourseChanged);
    
    // Geçmiş sekmesindeki ders seçimi değiştiğinde geçmişi filtrele
    connect(ui->historyCourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWidget::onHistoryCourseChanged);
    
    // Öğrenciler sekmesindeki ders seçimi değiştiğinde öğrencileri filtrele
    connect(ui->studentsCourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWidget::onStudentsCourseChanged);
    
    // Öğrenci ekleme butonu
    connect(ui->addStudentButton, &QPushButton::clicked, this, &TeacherWidget::onAddStudentClicked);
    
    // Başlık değiştiğinde buton durumunu güncelle
    connect(ui->titleEdit, &QLineEdit::textChanged,
            this, &TeacherWidget::updateStartButtonState);
    
    // Tab değiştiğinde öğrenci listesini güncelle
    connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index) {
        // Öğrenciler tabına geçildiğinde
        if (index == 2) { // 2 = studentsTab
            int courseId = ui->studentsCourseComboBox->currentData().toInt();
            if (courseId > 0) {
                loadEnrolledStudents(courseId);
            }
        }
        // Geçmiş tabına geçildiğinde
        else if (index == 1) { // 1 = historyTab
            int courseId = ui->historyCourseComboBox->currentData().toInt();
            loadAttendanceHistory(courseId);
        }
    });
    
    // Buton bağlantıları
    connect(ui->startButton, &QPushButton::clicked, this, &TeacherWidget::onStartAttendanceClicked);
    connect(ui->endButton, &QPushButton::clicked, this, &TeacherWidget::onEndAttendanceClicked);
    connect(ui->changePasswordButton, &QPushButton::clicked, this, &TeacherWidget::showChangePasswordDialog);
    connect(ui->changeEmailButton, &QPushButton::clicked, this, &TeacherWidget::showChangeEmailDialog);
    connect(ui->logoutButton, &QPushButton::clicked, this, &TeacherWidget::logoutRequested);
    
    // Başlangıçta butonları devre dışı bırak
    ui->startButton->setEnabled(false);
    ui->endButton->setEnabled(false);
}

void TeacherWidget::loadData()
{
    loadCourses();
    
    // Aktif yoklama kontrolü
    m_currentSessionId = m_dbManager.getActiveSessionId(m_currentUser.id);
    checkActiveAttendance();
    
    // Varsayılan olarak ilk dersi seç
    if (ui->courseComboBox->count() > 0) {
        int courseId = ui->courseComboBox->itemData(0).toInt();
        loadEnrolledStudents(courseId);
    }
    
    // Yoklama geçmişini yükle
    loadAttendanceHistory(-1); // Tüm derslerin yoklama geçmişini yükle
}

void TeacherWidget::loadCourses()
{
    qDebug() << ">>> loadCourses çağrıldı, öğretmen ID:" << m_currentUser.id;
    
    // Ana ders seçimi combobox'ı
    ui->courseComboBox->clear();
    ui->courseComboBox->addItem("Ders seçiniz...", -1);
    
    // Geçmiş sekmesindeki ders seçimi combobox'ı
    ui->historyCourseComboBox->clear();
    ui->historyCourseComboBox->addItem("Tüm Dersler", -1);
    
    // Öğrenciler sekmesindeki ders seçimi combobox'ı
    ui->studentsCourseComboBox->clear();
    ui->studentsCourseComboBox->addItem("Ders seçiniz...", -1);
    
    QVector<Course> courses = m_dbManager.getCoursesByTeacher(m_currentUser.id);
    qDebug() << ">>> Veritabanından alınan ders sayısı:" << courses.count();
    
    for (const auto& course : courses) {
        ui->courseComboBox->addItem(course.courseName, course.id);
        ui->historyCourseComboBox->addItem(course.courseName, course.id);
        ui->studentsCourseComboBox->addItem(course.courseName, course.id);
        qDebug() << ">>> Ders eklendi:" << course.courseName << "ID:" << course.id;
    }
}

void TeacherWidget::loadAttendanceHistory(int courseId)
{
    qDebug() << ">>> loadAttendanceHistory çağrıldı, öğretmen ID:" << m_currentUser.id << "Ders ID:" << courseId;
    
    QVector<AttendanceSession> sessions;
    
    if (courseId > 0) {
        // Belirli bir ders için yoklama geçmişini al
        sessions = m_dbManager.getAttendanceSessionsForCourse(courseId);
    } else {
        // Tüm dersler için yoklama geçmişini al
        sessions = m_dbManager.getTeacherAttendanceHistory(m_currentUser.id);
    }
    
    ui->historyTable->setRowCount(sessions.count());
    
    for (int i = 0; i < sessions.count(); ++i) {
        const auto& session = sessions[i];
        
        QTableWidgetItem* titleItem = new QTableWidgetItem(session.title);
        titleItem->setData(Qt::UserRole, session.id);
        ui->historyTable->setItem(i, 0, titleItem);
        
        QTableWidgetItem* courseItem = new QTableWidgetItem(session.courseName);
        ui->historyTable->setItem(i, 1, courseItem);
        
        QTableWidgetItem* dateItem = new QTableWidgetItem(session.startTime.toString("dd.MM.yyyy HH:mm"));
        ui->historyTable->setItem(i, 2, dateItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(session.isActive ? "Aktif" : "Tamamlandı");
        ui->historyTable->setItem(i, 3, statusItem);
        
        QTableWidgetItem* countItem = new QTableWidgetItem(QString::number(session.studentCount));
        ui->historyTable->setItem(i, 4, countItem);
        
        // İşlemler butonu - sadece tamamlanmış yoklamalar için
        if (!session.isActive) {
            QWidget* buttonWidget = new QWidget();
            buttonWidget->setStyleSheet("QWidget { background: transparent; }");
            QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
            buttonLayout->setContentsMargins(2, 2, 2, 2);
            buttonLayout->setSpacing(0);
            
            QPushButton* deleteButton = new QPushButton("Silme İsteği");
            deleteButton->setStyleSheet("QPushButton { background: #e74c3c; color: white; border: none; padding: 4px 10px; border-radius: 8px; font-size: 10pt; font-weight: bold; } QPushButton:hover { background: #ec7063; }");
            deleteButton->setToolTip("Bu yoklama için silme isteği gönder");
            connect(deleteButton, &QPushButton::clicked, [this, session]() {
                requestDeleteAttendance(session.id, session.title);
            });
            
            buttonLayout->addWidget(deleteButton);
            buttonLayout->addStretch();
            buttonLayout->setContentsMargins(4, 4, 4, 4);
            buttonLayout->setSpacing(6);
            
            ui->historyTable->setCellWidget(i, 5, buttonWidget);
        } else {
            // Aktif yoklamalar için boş widget
            QWidget* emptyWidget = new QWidget();
            emptyWidget->setStyleSheet("QWidget { background: transparent; }");
            ui->historyTable->setCellWidget(i, 5, emptyWidget);
        }
    }
    
    // Tabloyu esnek yap
    ui->historyTable->resizeColumnsToContents();
    ui->historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TeacherWidget::checkActiveAttendance()
{
    m_currentSessionId = m_dbManager.getActiveSessionId(m_currentUser.id);
    
    if (m_currentSessionId > 0) {
        // Aktif yoklamanın ders ID'sini al ve o dersi seç
        int courseId = m_dbManager.getCourseIdForSession(m_currentSessionId);
        if (courseId > 0) {
            // ComboBox'ta bu dersi seç
            for (int i = 0; i < ui->courseComboBox->count(); ++i) {
                if (ui->courseComboBox->itemData(i).toInt() == courseId) {
                    ui->courseComboBox->setCurrentIndex(i);
                    break;
                }
            }
            
            // Kayıtlı öğrencileri yükle
            loadEnrolledStudents(courseId);
        }
        
        // Başlık etiketini güncelle
        ui->titleLabel->setText(QString("Öğretmen Paneli - %1 [AKTİF YOKLAMA]").arg(m_currentUser.fullName));
        ui->titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #27ae60; margin: 10px; padding: 10px;");
        
        ui->endButton->setEnabled(true);
        ui->startButton->setEnabled(false);
        loadCurrentAttendanceData();
    } else {
        // Başlık etiketini normal hale getir
        ui->titleLabel->setText(QString("Öğretmen Paneli - %1").arg(m_currentUser.fullName));
        ui->titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2c3e50; margin: 10px; padding: 10px;");
        
        ui->endButton->setEnabled(false);
        updateStartButtonState();
    }
}

void TeacherWidget::updateStartButtonState()
{
    bool hasValidCourse = ui->courseComboBox->currentData().toInt() > 0;
    bool hasValidTitle = !ui->titleEdit->text().trimmed().isEmpty();
    bool noActiveAttendance = m_currentSessionId == -1;
    
    ui->startButton->setEnabled(hasValidCourse && hasValidTitle && noActiveAttendance);
}

void TeacherWidget::onStartAttendanceClicked()
{
    int courseId = ui->courseComboBox->currentData().toInt();
    QString title = ui->titleEdit->text().trimmed();
    
    if (courseId <= 0 || title.isEmpty()) {
        QMessageBox::warning(this, "Uyarı", "Lütfen ders seçin ve yoklama başlığı girin.");
        return;
    }
    
    bool success = m_dbManager.startAttendanceSession(m_currentUser.id, courseId, title);
    
    if (success) {
        m_currentSessionId = m_dbManager.getLastInsertId();
        
        // Başlık etiketini güncelle
        ui->titleLabel->setText(QString("Öğretmen Paneli - %1 [AKTİF YOKLAMA]").arg(m_currentUser.fullName));
        ui->titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #27ae60; margin: 10px; padding: 10px;");
        
        ui->startButton->setEnabled(false);
        ui->endButton->setEnabled(true);
        
        // Yoklama sekmesine geç
        ui->tabWidget->setCurrentIndex(0);
        
        // Mevcut yoklama tablosunu temizle
        ui->currentAttendanceTable->setRowCount(0);
        
        // Kayıtlı öğrencilerin durumunu güncelle
        loadEnrolledStudents(courseId);
        
        emit attendanceStarted(m_currentSessionId);
        
        QMessageBox::information(this, "Başarılı", "Yoklama başlatıldı!");
    } else {
        QMessageBox::critical(this, "Hata", "Yoklama başlatılamadı!");
    }
}

void TeacherWidget::onEndAttendanceClicked()
{
    if (m_currentSessionId <= 0) {
        QMessageBox::warning(this, "Uyarı", "Aktif yoklama bulunmuyor.");
        return;
    }
    
    bool success = m_dbManager.endAttendanceSession(m_currentSessionId);
    
    if (success) {
        // Başlık etiketini normal hale getir
        ui->titleLabel->setText(QString("Öğretmen Paneli - %1").arg(m_currentUser.fullName));
        ui->titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold; color: #2c3e50; margin: 10px; padding: 10px;");
        
        ui->startButton->setEnabled(true);
        ui->endButton->setEnabled(false);
        
        // Mevcut ders ID'sini al
        int courseId = ui->courseComboBox->currentData().toInt();
        
        m_currentSessionId = -1;
        emit attendanceEnded();
        
        QMessageBox::information(this, "Başarılı", "Yoklama sonlandırıldı!");
        loadAttendanceHistory(-1); // Geçmişi güncelle
        
        // Kayıtlı öğrencilerin durumunu güncelle
        if (courseId > 0) {
            loadEnrolledStudents(courseId);
        }
    } else {
        QMessageBox::critical(this, "Hata", "Yoklama sonlandırılamadı!");
    }
}

void TeacherWidget::updateAttendanceList()
{
    if (m_currentSessionId > 0) {
        loadCurrentAttendanceData();
        
        // Aktif yoklama varsa ve bir ders seçiliyse, kayıtlı öğrencilerin durumunu da güncelle
        int courseId = ui->courseComboBox->currentData().toInt();
        if (courseId > 0) {
            loadEnrolledStudents(courseId);
        }
    }
}

void TeacherWidget::loadCurrentAttendanceData()
{
    if (!ui->currentAttendanceTable || m_currentSessionId <= 0) return;
    
    QVector<AttendanceRecord> records = m_dbManager.getAttendanceForSession(m_currentSessionId);
    ui->currentAttendanceTable->setRowCount(records.count());
    
    for (int i = 0; i < records.count(); ++i) {
        const auto& record = records[i];
        QTableWidgetItem* studentNumberItem = new QTableWidgetItem(record.studentNumber);
        QTableWidgetItem* firstNameItem = new QTableWidgetItem(record.firstName);
        QTableWidgetItem* lastNameItem = new QTableWidgetItem(record.lastName);
        QTableWidgetItem* timeItem = new QTableWidgetItem(record.time);
        
        ui->currentAttendanceTable->setItem(i, 0, studentNumberItem);
        ui->currentAttendanceTable->setItem(i, 1, firstNameItem);
        ui->currentAttendanceTable->setItem(i, 2, lastNameItem);
        ui->currentAttendanceTable->setItem(i, 3, timeItem);
    }
    
    TableHelper::resizeColumnsToContent(ui->currentAttendanceTable);
}

void TeacherWidget::onHistoryTableDoubleClicked(int row, int column)
{
    QTableWidgetItem* item = ui->historyTable->item(row, 0);
    if (item) {
        int sessionId = item->data(Qt::UserRole).toInt();
        showAttendanceDetails(sessionId);
    }
}

void TeacherWidget::showAttendanceDetails(int sessionId)
{
    QVector<AttendanceRecord> records = m_dbManager.getAttendanceForSession(sessionId);
    
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Yoklama Detayları");
    dialog->setModal(true);
    dialog->setFixedSize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QTableWidget* detailTable = new QTableWidget();
    detailTable->setColumnCount(4);
    detailTable->setHorizontalHeaderLabels({"Öğrenci No", "Ad", "Soyad", "Geliş Saati"});
    detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailTable->setAlternatingRowColors(true);
    
    detailTable->setRowCount(records.count());
    for (int i = 0; i < records.count(); ++i) {
        const auto& record = records[i];
        detailTable->setItem(i, 0, new QTableWidgetItem(record.studentNumber));
        detailTable->setItem(i, 1, new QTableWidgetItem(record.firstName));
        detailTable->setItem(i, 2, new QTableWidgetItem(record.lastName));
        detailTable->setItem(i, 3, new QTableWidgetItem(record.time));
    }
    
    TableHelper::resizeColumnsToContent(detailTable);
    layout->addWidget(detailTable);
    
    QPushButton* closeButton = new QPushButton("Kapat");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    dialog->show();
}

void TeacherWidget::requestDeleteAttendance(int sessionId, const QString& title)
{
    bool ok;
    QString reason = QInputDialog::getText(this, "Silme İsteği", 
                                          QString("Yoklama: %1\nSilme sebebi:").arg(title),
                                          QLineEdit::Normal, "", &ok);
    
    if (ok && !reason.isEmpty()) {
        bool success = m_dbManager.requestAttendanceDeletion(sessionId, m_currentUser.id, reason);
        
        if (success) {
            QMessageBox::information(this, "Başarılı", "Silme isteği gönderildi. Admin onayı bekleniyor.");
        } else {
            QMessageBox::critical(this, "Hata", "Silme isteği gönderilemedi.");
        }
    }
}

void TeacherWidget::showChangePasswordDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Şifre Değiştir");
    dialog->setModal(true);
    dialog->setFixedSize(400, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // Kullanıcı adı
    QHBoxLayout* usernameLayout = new QHBoxLayout();
    QLabel* usernameLabel = new QLabel("Kullanıcı Adı:");
    usernameLabel->setMinimumWidth(100);
    QLineEdit* usernameEdit = new QLineEdit();
    usernameEdit->setText(m_currentUser.fullName.split(" ").first().toLower());
    usernameEdit->setPlaceholderText("Kullanıcı adınızı girin");
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(usernameEdit);
    layout->addLayout(usernameLayout);
    
    // Mevcut şifre
    QHBoxLayout* currentPasswordLayout = new QHBoxLayout();
    QLabel* currentPasswordLabel = new QLabel("Mevcut Şifre:");
    currentPasswordLabel->setMinimumWidth(100);
    QLineEdit* currentPasswordEdit = new QLineEdit();
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    currentPasswordEdit->setPlaceholderText("Mevcut şifrenizi girin");
    currentPasswordLayout->addWidget(currentPasswordLabel);
    currentPasswordLayout->addWidget(currentPasswordEdit);
    layout->addLayout(currentPasswordLayout);
    
    // Yeni şifre
    QHBoxLayout* newPasswordLayout = new QHBoxLayout();
    QLabel* newPasswordLabel = new QLabel("Yeni Şifre:");
    newPasswordLabel->setMinimumWidth(100);
    QLineEdit* newPasswordEdit = new QLineEdit();
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setPlaceholderText("Yeni şifrenizi girin");
    newPasswordLayout->addWidget(newPasswordLabel);
    newPasswordLayout->addWidget(newPasswordEdit);
    layout->addLayout(newPasswordLayout);
    
    // Şifre tekrar
    QHBoxLayout* confirmPasswordLayout = new QHBoxLayout();
    QLabel* confirmPasswordLabel = new QLabel("Şifre Tekrar:");
    confirmPasswordLabel->setMinimumWidth(100);
    QLineEdit* confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText("Yeni şifrenizi tekrar girin");
    confirmPasswordLayout->addWidget(confirmPasswordLabel);
    confirmPasswordLayout->addWidget(confirmPasswordEdit);
    layout->addLayout(confirmPasswordLayout);
    
    // Butonlar
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* changeButton = new QPushButton("Şifreyi Değiştir");
    QPushButton* cancelButton = new QPushButton("İptal");
    
    connect(changeButton, &QPushButton::clicked, [this, dialog, usernameEdit, currentPasswordEdit, newPasswordEdit, confirmPasswordEdit]() {
        QString username = usernameEdit->text().trimmed();
        QString currentPassword = currentPasswordEdit->text();
        QString newPassword = newPasswordEdit->text();
        QString confirmPassword = confirmPasswordEdit->text();
        
        if (username.isEmpty() || currentPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
            QMessageBox::warning(dialog, "Uyarı", "Lütfen tüm alanları doldurun.");
            return;
        }
        
        if (newPassword != confirmPassword) {
            QMessageBox::warning(dialog, "Uyarı", "Yeni şifreler eşleşmiyor.");
            return;
        }
        
        if (newPassword.length() < 6) {
            QMessageBox::warning(dialog, "Uyarı", "Şifre en az 6 karakter olmalıdır.");
            return;
        }
        
        // Önce kullanıcıyı doğrula
        User user;
        QVariant authResult = m_dbManager.authenticateUser(username, currentPassword, user);
        
        if (authResult.toBool()) {
            // Şifreyi değiştir
            bool success = m_dbManager.changePassword(user.id, newPassword);
            
            if (success) {
                QMessageBox::information(dialog, "Başarılı", "Şifreniz başarıyla değiştirildi.");
                dialog->accept();
            } else {
                QMessageBox::critical(dialog, "Hata", "Şifre değiştirilemedi.");
            }
        } else {
            QMessageBox::critical(dialog, "Hata", "Kullanıcı adı veya mevcut şifre hatalı.");
        }
    });
    
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    buttonLayout->addWidget(changeButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}

void TeacherWidget::showChangeEmailDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("E-posta Değiştir");
    dialog->setModal(true);
    dialog->setFixedSize(400, 200);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // Mevcut email
    QHBoxLayout* currentEmailLayout = new QHBoxLayout();
    QLabel* currentEmailLabel = new QLabel("Mevcut E-posta:");
    currentEmailLabel->setMinimumWidth(100);
    QLineEdit* currentEmailEdit = new QLineEdit();
    currentEmailEdit->setPlaceholderText("Mevcut e-posta adresinizi girin");
    currentEmailLayout->addWidget(currentEmailLabel);
    currentEmailLayout->addWidget(currentEmailEdit);
    layout->addLayout(currentEmailLayout);
    
    // Yeni email
    QHBoxLayout* newEmailLayout = new QHBoxLayout();
    QLabel* newEmailLabel = new QLabel("Yeni E-posta:");
    newEmailLabel->setMinimumWidth(100);
    QLineEdit* newEmailEdit = new QLineEdit();
    newEmailEdit->setPlaceholderText("Yeni e-posta adresinizi girin");
    newEmailLayout->addWidget(newEmailLabel);
    newEmailLayout->addWidget(newEmailEdit);
    layout->addLayout(newEmailLayout);
    
    // Butonlar
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* changeButton = new QPushButton("E-postayı Değiştir");
    QPushButton* cancelButton = new QPushButton("İptal");
    
    connect(changeButton, &QPushButton::clicked, [this, dialog, currentEmailEdit, newEmailEdit]() {
        QString currentEmail = currentEmailEdit->text().trimmed();
        QString newEmail = newEmailEdit->text().trimmed();
        
        if (currentEmail.isEmpty() || newEmail.isEmpty()) {
            QMessageBox::warning(dialog, "Uyarı", "Lütfen tüm alanları doldurun.");
            return;
        }
        
        if (!newEmail.contains("@") || !newEmail.contains(".")) {
            QMessageBox::warning(dialog, "Uyarı", "Geçerli bir e-posta adresi girin.");
            return;
        }
        
        // E-posta değiştirme fonksiyonunu DatabaseManager'a ekleyelim
        bool success = m_dbManager.changeEmail(m_currentUser.id, currentEmail, newEmail);
        
        if (success) {
            QMessageBox::information(dialog, "Başarılı", "E-posta adresiniz başarıyla değiştirildi.");
            dialog->accept();
        } else {
            QMessageBox::critical(dialog, "Hata", "E-posta değiştirilemedi. Mevcut e-posta adresinizi kontrol edin.");
        }
    });
    
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    buttonLayout->addWidget(changeButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}

void TeacherWidget::onCourseChanged(int index)
{
    updateStartButtonState();
    
    int courseId = ui->courseComboBox->currentData().toInt();
    if (courseId > 0) {
        loadEnrolledStudents(courseId);
    } else {
        // Ders seçilmediğinde tabloyu temizle
        ui->enrolledStudentsTable->setRowCount(0);
    }
}

void TeacherWidget::loadEnrolledStudents(int courseId)
{
    if (courseId <= 0) return;
    
    qDebug() << ">>> loadEnrolledStudents çağrıldı, ders ID:" << courseId;
    
    QVector<Student> students = m_dbManager.getStudentsForCourse(courseId);
    ui->enrolledStudentsTable->setRowCount(students.count());
    
    // Eğer aktif yoklama varsa, yoklamaya katılan öğrencileri al
    QVector<AttendanceRecord> attendanceRecords;
    if (m_currentSessionId > 0) {
        attendanceRecords = m_dbManager.getAttendanceForSession(m_currentSessionId);
        qDebug() << ">>> Aktif yoklama ID:" << m_currentSessionId << "Kayıt sayısı:" << attendanceRecords.count();
    } else {
        qDebug() << ">>> Aktif yoklama yok";
    }
    
    for (int i = 0; i < students.count(); ++i) {
        const auto& student = students[i];
        
        QTableWidgetItem* studentNumberItem = new QTableWidgetItem(student.studentNumber);
        ui->enrolledStudentsTable->setItem(i, 0, studentNumberItem);
        
        QTableWidgetItem* firstNameItem = new QTableWidgetItem(student.firstName);
        ui->enrolledStudentsTable->setItem(i, 1, firstNameItem);
        
        QTableWidgetItem* lastNameItem = new QTableWidgetItem(student.lastName);
        ui->enrolledStudentsTable->setItem(i, 2, lastNameItem);
        
        // Durum sütunu
        QTableWidgetItem* statusItem = new QTableWidgetItem();
        
        if (m_currentSessionId > 0) {
            // Öğrencinin yoklamada olup olmadığını kontrol et
            bool isPresent = false;
            QString attendanceTime;
            
            for (const auto& record : attendanceRecords) {
                if (record.studentNumber == student.studentNumber) {
                    isPresent = true;
                    attendanceTime = record.time;
                    break;
                }
            }
            
            if (isPresent) {
                statusItem->setText("Var (" + attendanceTime + ")");
                statusItem->setForeground(QBrush(QColor("#27ae60"))); // Yeşil renk
            } else {
                statusItem->setText("Yok");
                statusItem->setForeground(QBrush(QColor("#e74c3c"))); // Kırmızı renk
            }
        } else {
            statusItem->setText("-");
            statusItem->setForeground(QBrush(QColor("#7f8c8d"))); // Gri renk
        }
        
        ui->enrolledStudentsTable->setItem(i, 3, statusItem);
    }
    
    TableHelper::resizeColumnsToContent(ui->enrolledStudentsTable);
}

void TeacherWidget::onHistoryCourseChanged(int index)
{
    int courseId = ui->historyCourseComboBox->currentData().toInt();
    loadAttendanceHistory(courseId);
}

void TeacherWidget::onStudentsCourseChanged(int index)
{
    int courseId = ui->studentsCourseComboBox->currentData().toInt();
    if (courseId > 0) {
        loadEnrolledStudents(courseId);
        ui->addStudentButton->setEnabled(true);
    } else {
        ui->enrolledStudentsTable->setRowCount(0);
        ui->addStudentButton->setEnabled(false);
    }
}

void TeacherWidget::onAddStudentClicked()
{
    int courseId = ui->studentsCourseComboBox->currentData().toInt();
    if (courseId > 0) {
        showAddStudentDialog(courseId);
    } else {
        QMessageBox::warning(this, "Uyarı", "Lütfen önce bir ders seçin.");
    }
}

void TeacherWidget::showAddStudentDialog(int courseId)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Öğrenci Ekle");
    dialog->setModal(true);
    dialog->setFixedSize(500, 350);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // Öğrenci bilgileri
    QGroupBox* studentInfoGroup = new QGroupBox("Öğrenci Bilgileri");
    QGridLayout* infoLayout = new QGridLayout(studentInfoGroup);
    
    QLabel* cardLabel = new QLabel("Kart UID:");
    QLineEdit* cardEdit = new QLineEdit();
    cardEdit->setPlaceholderText("Kart okutun veya UID girin");
    infoLayout->addWidget(cardLabel, 0, 0);
    infoLayout->addWidget(cardEdit, 0, 1);
    
    QLabel* studentNumberLabel = new QLabel("Öğrenci No:");
    QLineEdit* studentNumberEdit = new QLineEdit();
    studentNumberEdit->setPlaceholderText("Öğrenci numarası girin");
    infoLayout->addWidget(studentNumberLabel, 1, 0);
    infoLayout->addWidget(studentNumberEdit, 1, 1);
    
    QLabel* firstNameLabel = new QLabel("Ad:");
    QLineEdit* firstNameEdit = new QLineEdit();
    firstNameEdit->setPlaceholderText("Öğrencinin adını girin");
    infoLayout->addWidget(firstNameLabel, 2, 0);
    infoLayout->addWidget(firstNameEdit, 2, 1);
    
    QLabel* lastNameLabel = new QLabel("Soyad:");
    QLineEdit* lastNameEdit = new QLineEdit();
    lastNameEdit->setPlaceholderText("Öğrencinin soyadını girin");
    infoLayout->addWidget(lastNameLabel, 3, 0);
    infoLayout->addWidget(lastNameEdit, 3, 1);
    
    layout->addWidget(studentInfoGroup);
    
    // Butonlar
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* saveButton = new QPushButton("Kaydet");
    saveButton->setStyleSheet("QPushButton { background: #27ae60; color: white; border: none; padding: 8px 16px; border-radius: 8px; font-weight: bold; }");
    
    QPushButton* cancelButton = new QPushButton("İptal");
    cancelButton->setStyleSheet("QPushButton { background: #e74c3c; color: white; border: none; padding: 8px 16px; border-radius: 8px; font-weight: bold; }");
    
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    // Kart okutma butonu
    QPushButton* scanCardButton = new QPushButton("Kart Okut");
    scanCardButton->setStyleSheet("QPushButton { background: #3498db; color: white; border: none; padding: 8px 16px; border-radius: 8px; font-weight: bold; }");
    layout->addWidget(scanCardButton);
    
    // Kart okuma durumu etiketi
    QLabel* scanStatusLabel = new QLabel("Kart bekleniyor...");
    scanStatusLabel->setAlignment(Qt::AlignCenter);
    scanStatusLabel->setStyleSheet("color: #7f8c8d; font-style: italic;");
    scanStatusLabel->setVisible(false);
    layout->addWidget(scanStatusLabel);
    
    // Sinyal bağlantıları
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    // Kart okutma butonuna tıklandığında
    connect(scanCardButton, &QPushButton::clicked, [this, scanCardButton, scanStatusLabel, cardEdit, dialog]() {
        // Kart okuyucuya bağlanmayı dene
        bool portAvailable = tryOpenCardReaderPort("COM10");
        if (!portAvailable) {
            QMessageBox::warning(dialog, "Uyarı", "Kart okuyucu COM10 portuna bağlı değil!\nKart UID'sini elle girebilirsiniz.");
            scanStatusLabel->setText("Kart okuyucu bağlı değil, UID'yi elle girin.");
            scanStatusLabel->setStyleSheet("color: #e67e22; font-weight: bold;");
            scanStatusLabel->setVisible(true);
            return;
        }
        // Kart okuma modunu aktifleştir
        emit cardScanRequested();
        // Buton durumunu güncelle
        scanCardButton->setEnabled(false);
        scanCardButton->setText("Kart Bekleniyor...");
        scanStatusLabel->setText("Lütfen kartı okutucuya yaklaştırın...");
        scanStatusLabel->setStyleSheet("color: #3498db; font-weight: bold;");
        scanStatusLabel->setVisible(true);
        // 10 saniye sonra timeout
        QTimer::singleShot(10000, [scanCardButton, scanStatusLabel]() {
            if (!scanCardButton->isEnabled()) {
                scanCardButton->setEnabled(true);
                scanCardButton->setText("Kart Okut");
                scanStatusLabel->setText("Zaman aşımı! Tekrar deneyin veya UID'yi elle girin.");
                scanStatusLabel->setStyleSheet("color: #e74c3c;");
            }
        });
    });
    
    // Ana pencereden gelen kart UID'sini dinle
    MainWindow* mainWindow = qobject_cast<MainWindow*>(window());
    if (mainWindow) {
        QMetaObject::Connection connection = connect(mainWindow, &MainWindow::cardScanned, 
            [this, cardEdit, scanCardButton, scanStatusLabel, connection](const QString& uid) {
                // Kart UID'sini al ve alana yerleştir
                cardEdit->setText(uid);
                
                // Buton durumunu güncelle
                scanCardButton->setEnabled(true);
                scanCardButton->setText("Kart Okut");
                scanStatusLabel->setText("Kart başarıyla okundu!");
                scanStatusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
                
                // Bağlantıyı kaldır
                disconnect(connection);
            });
    }
    
    connect(saveButton, &QPushButton::clicked, [this, dialog, cardEdit, studentNumberEdit, firstNameEdit, lastNameEdit, courseId]() {
        QString cardUID = cardEdit->text().trimmed();
        QString studentNumber = studentNumberEdit->text().trimmed();
        QString firstName = firstNameEdit->text().trimmed();
        QString lastName = lastNameEdit->text().trimmed();
        
        // Öğrenci numarası ile kontrol
        Student existingStudentByNumber = m_dbManager.getStudentByNumber(studentNumber);
        if (existingStudentByNumber.id > 0) {
            QMessageBox::warning(dialog, "Uyarı", "Bu öğrenci numarası ile zaten bir öğrenci kayıtlı!");
            return;
        }
        // Kart UID ile kontrol (boş olabilir)
        if (!cardUID.isEmpty()) {
        Student existingStudent = m_dbManager.getStudentByCardUID(cardUID);
        if (existingStudent.id > 0) {
            QString info = QString("Bu öğrenci zaten kayıtlı!\n\nAd: %1\nSoyad: %2\nNumara: %3\n\nBu öğrenciyi bu derse kaydetmek ister misiniz?")
                .arg(existingStudent.firstName)
                .arg(existingStudent.lastName)
                .arg(existingStudent.studentNumber);
            auto reply = QMessageBox::question(dialog, "Öğrenci Zaten Kayıtlı", info, QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                bool enrollSuccess = m_dbManager.enrollStudentToCourse(existingStudent.id, courseId, m_currentUser.id);
                if (enrollSuccess) {
                    QMessageBox::information(dialog, "Başarılı", "Öğrenci bu derse başarıyla kaydedildi.");
                    loadEnrolledStudents(courseId);
                    dialog->accept();
                } else {
                    QMessageBox::critical(dialog, "Hata", "Öğrenci bu derse kaydedilemedi veya zaten kayıtlı.");
                }
            }
            return;
        }
        }
        // Diğer alanlar zorunlu
        if (studentNumber.isEmpty() || firstName.isEmpty() || lastName.isEmpty()) {
            QMessageBox::warning(dialog, "Uyarı", "Lütfen tüm alanları doldurun.");
            return;
        }
        Student student = m_dbManager.addNewStudentAndEnroll(cardUID, studentNumber, firstName, lastName, m_currentUser.id, courseId);
        if (student.id > 0) {
            QMessageBox::information(dialog, "Başarılı", "Öğrenci başarıyla eklendi ve derse kaydedildi.");
            loadEnrolledStudents(courseId);
            dialog->accept();
        } else {
            QMessageBox::critical(dialog, "Hata", "Öğrenci eklenirken bir hata oluştu.");
        }
    });
    
    dialog->show();
}

int TeacherWidget::getCurrentCourseIdForEnrollment() const
{
    // "Öğrenciler" sekmesindeki combobox'tan geçerli ders ID'sini al
    return ui->studentsCourseComboBox->currentData().toInt();
}

bool TeacherWidget::tryOpenCardReaderPort(const QString& portName)
{
    QSerialPort serial;
    serial.setPortName(portName);
    bool ok = serial.open(QIODevice::ReadWrite);
    if (ok)
        serial.close();
    return ok;
}