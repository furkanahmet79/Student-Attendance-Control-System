#ifndef ADMINWIDGET_H
#define ADMINWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AdminWidget; }
QT_END_NAMESPACE

class AdminWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdminWidget(DatabaseManager& dbManager, const User& user, QWidget* parent = nullptr);
    ~AdminWidget();

signals:
    void logoutRequested();

private slots:
    void onAttendanceTableDoubleClicked(int row, int column);
    void approveDeleteRequest(int requestId);
    void rejectDeleteRequest(int requestId);
    void onAddTeacherClicked();
    void onAddCourseClicked();
    void onAssignCourseToTeacherClicked();
    void onTeacherTableContextMenu(const QPoint& pos);
    void changeTeacherPassword(int row);
    void removeTeacher();

private:
    void setupConnections();
    void setupTableHeaders();
    void loadData();
    void loadAttendanceData();
    void loadDeleteRequestsData();
    void showAttendanceDetails(int sessionId);
    void loadTeachersData();
    void loadCoursesData();

    Ui::AdminWidget *ui;
    DatabaseManager& m_dbManager;
    User m_currentUser;
};

#endif // ADMINWIDGET_H 