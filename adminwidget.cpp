#include "adminwidget.h"
#include "./ui_adminwidget.h"
#include "tablehelper.h"
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
#include <QLineEdit>
#include <QGridLayout>
#include <QComboBox>
#include <QDebug>
#include <QMenu>
#include <QAction>

AdminWidget::AdminWidget(DatabaseManager& dbManager, const User& user, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AdminWidget)
    , m_dbManager(dbManager)
    , m_currentUser(user)
{
    ui->setupUi(this);
    setupConnections();
    loadData();
}

AdminWidget::~AdminWidget()
{
    delete ui;
}

void AdminWidget::setupConnections()
{
    // Çıkış butonu
    connect(ui->logoutButton, &QPushButton::clicked, this, [this]() {
        emit logoutRequested();
    });
    
    // Öğretmen yönetimi
    connect(ui->addTeacherButton, &QPushButton::clicked, this, &AdminWidget::onAddTeacherClicked);
    connect(ui->teachersTable, &QTableWidget::customContextMenuRequested, 
            this, &AdminWidget::onTeacherTableContextMenu);
    
    // Ders yönetimi
    connect(ui->addCourseButton, &QPushButton::clicked, this, &AdminWidget::onAddCourseClicked);
    
    // Yoklama yönetimi
    connect(ui->attendanceTable, &QTableWidget::cellDoubleClicked, 
            this, &AdminWidget::onAttendanceTableDoubleClicked);
    
    // Tablo başlıklarını ayarla
    setupTableHeaders();
    
    // Hoşgeldin mesajını güncelle
    ui->welcomeLabel->setText(QString("Hoşgeldiniz, %1").arg(m_currentUser.fullName));
}

void AdminWidget::setupTableHeaders()
{
    // Yoklama tablosu başlıkları
    ui->attendanceTable->setColumnCount(5);
    ui->attendanceTable->setHorizontalHeaderLabels({
        "Yoklama Başlığı", "Öğretmen", "Tarih", "Başlangıç", "Durum"
    });
    
    // Silme istekleri tablosu başlıkları
    ui->deleteRequestsTable->setColumnCount(6);
    ui->deleteRequestsTable->setHorizontalHeaderLabels({
        "Yoklama Başlığı", "Öğretmen", "İstek Tarihi", "Sebep", "Durum", "İşlemler"
    });
    
    // Öğretmenler tablosu başlıkları
    ui->teachersTable->setColumnCount(3);
    ui->teachersTable->setHorizontalHeaderLabels({
        "Kullanıcı Adı", "Ad Soyad", "E-posta"
    });
    
    // Dersler tablosu başlıkları
    ui->coursesTable->setColumnCount(4);
    ui->coursesTable->setHorizontalHeaderLabels({
        "Ders Adı", "Ders Kodu", "Öğretmen", "Durum"
    });
    
    // Tablo özelliklerini ayarla
    ui->attendanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->attendanceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->attendanceTable->setAlternatingRowColors(true);
    
    ui->deleteRequestsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->deleteRequestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->deleteRequestsTable->setAlternatingRowColors(true);
    
    ui->teachersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->teachersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->teachersTable->setAlternatingRowColors(true);
    
    ui->coursesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->coursesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->coursesTable->setAlternatingRowColors(true);
}

void AdminWidget::loadData()
{
    loadAttendanceData();
    loadDeleteRequestsData();
    loadTeachersData();
    loadCoursesData();
}

void AdminWidget::onAddTeacherClicked()
{
    QString username = ui->teacherUsernameEdit->text().trimmed();
    QString password = ui->teacherPasswordEdit->text();
    QString fullName = ui->teacherFullNameEdit->text().trimmed();
    QString email = ui->teacherEmailEdit->text().trimmed();
    if (username.isEmpty() || password.isEmpty() || fullName.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, "Uyarı", "Lütfen tüm alanları doldurun.");
        return;
    }
    bool success = m_dbManager.addTeacher(username, password, fullName, email);
    if (success) {
        QMessageBox::information(this, "Başarılı", "Öğretmen başarıyla eklendi.");
        ui->teacherUsernameEdit->clear();
        ui->teacherPasswordEdit->clear();
        ui->teacherFullNameEdit->clear();
        ui->teacherEmailEdit->clear();
        loadTeachersData();
        loadCoursesData();
    } else {
        QMessageBox::critical(this, "Hata", "Öğretmen eklenirken bir hata oluştu.");
    }
}

void AdminWidget::loadTeachersData()
{
    QVector<Teacher> teachers = m_dbManager.getAllTeachers();
    ui->teachersTable->setRowCount(teachers.count());
    for (int i = 0; i < teachers.count(); ++i) {
        const auto& t = teachers[i];
        QTableWidgetItem* usernameItem = new QTableWidgetItem(t.username);
        QTableWidgetItem* nameItem = new QTableWidgetItem(t.fullName);
        QTableWidgetItem* emailItem = new QTableWidgetItem(t.email);
        ui->teachersTable->setItem(i, 0, usernameItem);
        ui->teachersTable->setItem(i, 1, nameItem);
        ui->teachersTable->setItem(i, 2, emailItem);
    }
    TableHelper::resizeColumnsToContent(ui->teachersTable);
}

void AdminWidget::onAddCourseClicked()
{
    QString courseName = ui->courseNameEdit->text().trimmed();
    QString courseCode = ui->courseCodeEdit->text().trimmed();
    int teacherIndex = ui->courseTeacherCombo->currentIndex();
    
    if (courseName.isEmpty() || courseCode.isEmpty() || teacherIndex < 0) {
        QMessageBox::warning(this, "Uyarı", "Lütfen tüm alanları doldurun.");
        return;
    }
    
    int teacherId = ui->courseTeacherCombo->currentData().toInt();
    bool success = m_dbManager.addCourse(courseName, courseCode, teacherId, m_currentUser.id);
    
    if (success) {
        QMessageBox::information(this, "Başarılı", "Ders başarıyla eklendi.");
        ui->courseNameEdit->clear();
        ui->courseCodeEdit->clear();
        ui->courseTeacherCombo->setCurrentIndex(-1);
        loadCoursesData();
    } else {
        QMessageBox::critical(this, "Hata", "Ders eklenirken bir hata oluştu.");
    }
}

void AdminWidget::loadCoursesData()
{
    QVector<CourseWithTeacher> courses = m_dbManager.getAllCoursesWithTeachers();
    qDebug() << "Yüklenen ders sayısı:" << courses.count();
    
    ui->coursesTable->setRowCount(courses.count());
    
    for (int i = 0; i < courses.count(); ++i) {
        const auto& course = courses[i];
        qDebug() << "Ders:" << course.courseName << "Öğretmen:" << course.teacherName;
        QTableWidgetItem* nameItem = new QTableWidgetItem(course.courseName);
        QTableWidgetItem* codeItem = new QTableWidgetItem(course.courseCode);
        QTableWidgetItem* teacherItem = new QTableWidgetItem(course.teacherName);
        QTableWidgetItem* statusItem = new QTableWidgetItem("Aktif");
        
        ui->coursesTable->setItem(i, 0, nameItem);
        ui->coursesTable->setItem(i, 1, codeItem);
        ui->coursesTable->setItem(i, 2, teacherItem);
        ui->coursesTable->setItem(i, 3, statusItem);
    }
    
    TableHelper::resizeColumnsToContent(ui->coursesTable);
    
    // Öğretmen combo box'ını güncelle
    ui->courseTeacherCombo->clear();
    QVector<Teacher> teachers = m_dbManager.getAllTeachers();
    for (const auto& teacher : teachers) {
        ui->courseTeacherCombo->addItem(teacher.fullName, teacher.id);
    }
}

void AdminWidget::loadAttendanceData()
{
    // Debug bilgilerini göster
    m_dbManager.debugDatabaseTables();
    
    QVector<AdminAttendanceOverview> sessions = m_dbManager.getAdminAttendanceOverview();
    
    ui->attendanceTable->setRowCount(sessions.count());
    
    for (int i = 0; i < sessions.count(); ++i) {
        const auto& session = sessions[i];
        
        QTableWidgetItem* titleItem = new QTableWidgetItem(session.sessionTitle);
        titleItem->setData(Qt::UserRole, session.sessionId);
        ui->attendanceTable->setItem(i, 0, titleItem);
        
        QTableWidgetItem* teacherItem = new QTableWidgetItem(session.teacherName);
        ui->attendanceTable->setItem(i, 1, teacherItem);
        
        QTableWidgetItem* dateItem = new QTableWidgetItem(session.date);
        ui->attendanceTable->setItem(i, 2, dateItem);
        
        QTableWidgetItem* startItem = new QTableWidgetItem(session.startTime);
        ui->attendanceTable->setItem(i, 3, startItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            session.status == "active" ? "Aktif" : "Tamamlandı");
        ui->attendanceTable->setItem(i, 4, statusItem);
    }
    
    TableHelper::resizeColumnsToContent(ui->attendanceTable);
}

void AdminWidget::loadDeleteRequestsData()
{
    QVector<AttendanceDeleteRequest> requests = m_dbManager.getPendingDeleteRequests();
    
    ui->deleteRequestsTable->setRowCount(requests.count());
    
    for (int i = 0; i < requests.count(); ++i) {
        const auto& request = requests[i];
        
        QTableWidgetItem* titleItem = new QTableWidgetItem(request.sessionTitle);
        ui->deleteRequestsTable->setItem(i, 0, titleItem);
        
        QTableWidgetItem* teacherItem = new QTableWidgetItem(request.teacherName);
        ui->deleteRequestsTable->setItem(i, 1, teacherItem);
        
        QTableWidgetItem* dateItem = new QTableWidgetItem(request.requestedAt);
        ui->deleteRequestsTable->setItem(i, 2, dateItem);
        
        QTableWidgetItem* reasonItem = new QTableWidgetItem(request.reason);
        ui->deleteRequestsTable->setItem(i, 3, reasonItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem("Beklemede");
        ui->deleteRequestsTable->setItem(i, 4, statusItem);
        
        // Onay/Red Butonları
        QWidget* buttonWidget = new QWidget();
        QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(2, 2, 2, 2);
        
        QPushButton* approveButton = new QPushButton("Onayla");
        approveButton->setStyleSheet("QPushButton { background: #27ae60; color: white; border: none; padding: 5px; border-radius: 3px; }");
        connect(approveButton, &QPushButton::clicked, [this, request]() {
            approveDeleteRequest(request.requestId);
        });
        
        QPushButton* rejectButton = new QPushButton("Reddet");
        rejectButton->setStyleSheet("QPushButton { background: #e74c3c; color: white; border: none; padding: 5px; border-radius: 3px; }");
        connect(rejectButton, &QPushButton::clicked, [this, request]() {
            rejectDeleteRequest(request.requestId);
        });
        
        buttonLayout->addWidget(approveButton);
        buttonLayout->addWidget(rejectButton);
        buttonLayout->addStretch();
        
        ui->deleteRequestsTable->setCellWidget(i, 5, buttonWidget);
    }
    
    TableHelper::resizeColumnsToContent(ui->deleteRequestsTable);
}

void AdminWidget::onAttendanceTableDoubleClicked(int row, int column)
{
    QTableWidgetItem* item = ui->attendanceTable->item(row, 0);
    if (item) {
        int sessionId = item->data(Qt::UserRole).toInt();
        showAttendanceDetails(sessionId);
    }
}

void AdminWidget::showAttendanceDetails(int sessionId)
{
    qDebug() << "Yoklama detayları gösteriliyor, sessionId:" << sessionId;
    
    // Öğretmen panelindeki gibi sadece yoklamaya katılan öğrencileri al
    QVector<AttendanceRecord> records = m_dbManager.getAttendanceForSession(sessionId);
    
    qDebug() << "Alınan kayıt sayısı:" << records.count();
    
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Yoklama Detayları");
    dialog->setModal(true);
    dialog->setFixedSize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QTableWidget* detailTable = new QTableWidget();
    detailTable->setColumnCount(4);
    detailTable->setHorizontalHeaderLabels({"Öğrenci No", "Ad", "Soyad", "Yoklama Zamanı"});
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
        
        qDebug() << "Tablo satırı" << i << ":" << record.firstName << record.lastName << record.time;
    }
    
    TableHelper::resizeColumnsToContent(detailTable);
    layout->addWidget(detailTable);
    
    QPushButton* closeButton = new QPushButton("Kapat");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    dialog->show();
}

void AdminWidget::approveDeleteRequest(int requestId)
{
    bool success = m_dbManager.approveDeleteRequest(requestId);
    if (success) {
        QMessageBox::information(this, "Başarılı", "Silme isteği onaylandı.");
        loadDeleteRequestsData();
    } else {
        QMessageBox::critical(this, "Hata", "İstek onaylanırken hata oluştu.");
    }
}

void AdminWidget::rejectDeleteRequest(int requestId)
{
    bool success = m_dbManager.rejectDeleteRequest(requestId);
    if (success) {
        QMessageBox::information(this, "Başarılı", "Silme isteği reddedildi.");
        loadDeleteRequestsData();
    } else {
        QMessageBox::critical(this, "Hata", "İstek reddedilirken hata oluştu.");
    }
}

void AdminWidget::onAssignCourseToTeacherClicked()
{
    // Bu fonksiyon şimdilik boş bırakılıyor
    QMessageBox::information(this, "Bilgi", "Bu özellik henüz geliştirilmedi.");
}

void AdminWidget::onTeacherTableContextMenu(const QPoint& pos)
{
    QMenu* contextMenu = new QMenu(this);
    
    QAction* changePasswordAction = new QAction("Şifre Değiştir", this);
    connect(changePasswordAction, &QAction::triggered, [this]() {
        int currentRow = ui->teachersTable->currentRow();
        if (currentRow >= 0) {
            changeTeacherPassword(currentRow);
        }
    });
    
    QAction* removeAction = new QAction("Öğretmeni Kaldır", this);
    connect(removeAction, &QAction::triggered, this, &AdminWidget::removeTeacher);
    
    contextMenu->addAction(changePasswordAction);
    contextMenu->addAction(removeAction);
    
    contextMenu->exec(ui->teachersTable->mapToGlobal(pos));
}

void AdminWidget::changeTeacherPassword(int row)
{
    // Seçili öğretmenin bilgilerini al
    QTableWidgetItem* usernameItem = ui->teachersTable->item(row, 0);
    QTableWidgetItem* nameItem = ui->teachersTable->item(row, 1);
    
    if (!usernameItem || !nameItem) return;
    
    QString username = usernameItem->text();
    QString teacherName = nameItem->text();
    
    bool ok;
    QString newPassword = QInputDialog::getText(this, "Şifre Değiştir",
                                               QString("Öğretmen: %1\nYeni şifre:").arg(teacherName),
                                               QLineEdit::Password, "", &ok);
    
    if (ok && !newPassword.isEmpty()) {
        bool success = m_dbManager.changeTeacherPassword(username, newPassword);
        if (success) {
            QMessageBox::information(this, "Başarılı", "Şifre başarıyla değiştirildi.");
        } else {
            QMessageBox::critical(this, "Hata", "Şifre değiştirilirken hata oluştu.");
        }
    }
}

void AdminWidget::removeTeacher()
{
    // Seçili satırı al
    int currentRow = ui->teachersTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Uyarı", "Lütfen silinecek öğretmeni seçin.");
        return;
    }
    
    QTableWidgetItem* usernameItem = ui->teachersTable->item(currentRow, 0);
    QTableWidgetItem* nameItem = ui->teachersTable->item(currentRow, 1);
    
    if (!usernameItem || !nameItem) return;
    
    QString username = usernameItem->text();
    QString teacherName = nameItem->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Onay",
        QString("Öğretmen '%1' silinecek. Emin misiniz?").arg(teacherName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        bool success = m_dbManager.removeTeacher(username);
        if (success) {
            QMessageBox::information(this, "Başarılı", "Öğretmen başarıyla silindi.");
            loadTeachersData();
            loadCoursesData();
        } else {
            QMessageBox::critical(this, "Hata", "Öğretmen silinirken hata oluştu.");
        }
    }
} 