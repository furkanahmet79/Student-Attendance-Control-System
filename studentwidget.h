#ifndef STUDENTWIDGET_H
#define STUDENTWIDGET_H

#include <QWidget>
#include "databasemanager.h"

namespace Ui {
class StudentWidget;
}

class StudentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StudentWidget(const User& student, QWidget* parent = nullptr);
    ~StudentWidget();

    signals:
    void logoutRequested();

private:
    Ui::StudentWidget* ui;
    User m_student;
    DatabaseManager& m_dbManager;
    void loadAttendanceHistory();
    void loadAttendanceForCourse(int courseId);
};

#endif // STUDENTWIDGET_H 