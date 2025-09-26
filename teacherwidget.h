#ifndef TEACHERWIDGET_H
#define TEACHERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TeacherWidget; }
QT_END_NAMESPACE

class TeacherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TeacherWidget(DatabaseManager& dbManager, const User& user, QWidget* parent = nullptr);
    ~TeacherWidget();

    void updateAttendanceList();
    int getCurrentCourseIdForEnrollment() const;
    bool tryOpenCardReaderPort(const QString& portName);

signals:
    void logoutRequested();
    void attendanceStarted(int sessionId);
    void attendanceEnded();
    void cardScanRequested();

private slots:
    void onStartAttendanceClicked();
    void onEndAttendanceClicked();
    void onHistoryTableDoubleClicked(int row, int column);
    void showChangePasswordDialog();
    void showChangeEmailDialog();
    void updateStartButtonState();
    void onCourseChanged(int index);
    void onHistoryCourseChanged(int index);
    void onStudentsCourseChanged(int index);
    void onAddStudentClicked();

private:
    void setupUI();
    void setupTableHeaders();
    void setupConnections();
    void loadData();
    void loadCourses();
    void loadCurrentAttendanceData();
    void loadAttendanceHistory(int courseId);
    void loadEnrolledStudents(int courseId);
    void checkActiveAttendance();
    void showAttendanceDetails(int sessionId);
    void requestDeleteAttendance(int sessionId, const QString& title);
    void showAddStudentDialog(int courseId);
    
    Ui::TeacherWidget *ui;
    DatabaseManager& m_dbManager;
    User m_currentUser;
    int m_currentSessionId;
    QTableWidget* m_currentAttendanceTable;
    QTableWidget* m_historyTable;
    QTableWidget* m_enrolledStudentsTable;
    QComboBox* m_courseComboBox;
    QComboBox* m_historyCourseComboBox;
    QComboBox* m_studentsCourseComboBox;
    QLineEdit* m_titleEdit;
    QPushButton* m_startButton;
    QPushButton* m_endButton;
    QPushButton* m_addStudentButton;
    QLabel* m_courseSelectionLabel;
};

#endif // TEACHERWIDGET_H 