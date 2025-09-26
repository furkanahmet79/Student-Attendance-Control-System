#include "studentwidget.h"
#include "ui_studentwidget.h"
#include <QMessageBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

StudentWidget::StudentWidget(const User& student, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::StudentWidget)
    , m_student(student)
    , m_dbManager(DatabaseManager::instance())
{
    ui->setupUi(this);
    setWindowTitle("Öğrenci Paneli - " + m_student.fullName);
    ui->attendanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    loadAttendanceHistory();
    connect(ui->logoutButton, &QPushButton::clicked, this, [this]() {
        emit logoutRequested();
    });
}

StudentWidget::~StudentWidget()
{
    delete ui;
}

void StudentWidget::loadAttendanceHistory()
{
    // 1. Öğrencinin kayıtlı olduğu dersleri ComboBox'a yükle
    ui->courseComboBox->clear();
    QVector<Course> courses = m_dbManager.getCoursesForStudent(m_student.id);
    for (const auto& course : courses) {
        ui->courseComboBox->addItem(course.courseName, course.id);
    }
    // Varsayılan olarak ilk dersi seçili yap
    if (!courses.isEmpty()) {
        ui->courseComboBox->setCurrentIndex(0);
        loadAttendanceForCourse(courses.first().id);
    }
    // Ders değişince tabloyu güncelle
    connect(ui->courseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        int courseId = ui->courseComboBox->currentData().toInt();
        loadAttendanceForCourse(courseId);
    });
}

void StudentWidget::loadAttendanceForCourse(int courseId)
{
    // Tabloyu temizle
    ui->attendanceTable->setRowCount(0);
    if (courseId <= 0) return;
    // 2. Seçili dersin yoklama geçmişini getir
    QVector<AttendanceSession> sessions = m_dbManager.getAttendanceSessionsForCourse(courseId);
    int row = 0;
    for (const auto& session : sessions) {
        // Bu oturumda öğrenci var mı?
        QVector<AttendanceRecord> records = m_dbManager.getAttendanceForSession(session.id);
        bool found = false;
        QString time;
        for (const auto& rec : records) {
            Student studentInfo = m_dbManager.getStudentById(m_student.id);
            if (!studentInfo.studentNumber.isEmpty() && rec.studentNumber == studentInfo.studentNumber) {
                found = true;
                time = rec.time;
                break;
            }
        }
        ui->attendanceTable->insertRow(row);
        ui->attendanceTable->setItem(row, 0, new QTableWidgetItem(session.courseName));
        ui->attendanceTable->setItem(row, 1, new QTableWidgetItem(session.startTime.toString("yyyy-MM-dd")));
        ui->attendanceTable->setItem(row, 2, new QTableWidgetItem(found ? "Var" : "Yok"));
        ui->attendanceTable->setItem(row, 3, new QTableWidgetItem(found ? time : "-"));
        row++;
    }
    ui->attendanceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
} 