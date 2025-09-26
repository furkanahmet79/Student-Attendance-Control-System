#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVector>
#include <QDateTime>

// Veri transferi için basit yapılar (structs)
struct User {
    int id = -1;
    QString role;
    QString fullName;
};

struct Course {
    int id;
    QString courseName;
    QString courseCode;
    int teacherId;
    int createdBy;
    QDateTime createdAt;
};

struct Student {
    int id = -1;
    QString studentNumber;
    QString firstName;
    QString lastName;
    QString cardUID;
};

struct Teacher {
    int id;
    QString username;
    QString fullName;
    QString email;
    QString role;
};

struct CourseWithTeacher {
    int id;
    QString courseName;
    QString courseCode;
    int teacherId;
    QString teacherName;
    int createdBy;
    QDateTime createdAt;
};

struct AttendanceRecord {
    QString studentNumber;
    QString firstName;
    QString lastName;
    QString time;
};

struct AttendanceDetail {
    QString studentNumber;
    QString firstName;
    QString lastName;
    QString status;
};

struct AttendanceSession {
    int id;
    QString title;
    QDateTime startTime;
    QDateTime endTime;
    bool isActive;
    QString courseName;
    QString courseCode;
    int studentCount;
};

struct AttendanceSessionDetail {
    int sessionId;
    QString title;
    QString date;
    QString startTime;
    QString endTime;
    QString status;
    QVector<AttendanceRecord> records;
};

struct AttendanceDeleteRequest {
    int requestId;
    int sessionId;
    QString sessionTitle;
    QString teacherName;
    QString requestedAt;
    QString reason;
    QString status;
};

struct AdminAttendanceOverview {
    int sessionId;
    QString sessionTitle;
    QString teacherName;
    QString courseName;
    QString date;
    QString startTime;
    QString status;
    int studentCount;
};

class DatabaseManager
{
public:
    static DatabaseManager& instance();
    bool openDatabase(const QString& path);
    void closeDatabase();

    QVariant authenticateUser(const QString& username, const QString& password, User& user);
    QVector<Course> getCoursesForTeacher(int teacherId);
    QVector<Student> getStudentsForCourse(int courseId);
    Student getStudentByCardUID(const QString& cardUID);
    Student getStudentById(int studentId);
    bool enrollStudentToCourse(int studentId, int courseId, int teacherId);
    Student addNewStudentAndEnroll(const QString& cardUID, const QString& studentNumber, const QString& firstName, const QString& lastName, int teacherId, int courseId);
    bool markStudentPresent(int sessionId, int studentId, int& rowsAffected);
    QVector<AttendanceRecord> getAttendanceForSession(int sessionId);
    QVector<AttendanceSession> getAttendanceSessionsForCourse(int courseId);
    AttendanceSessionDetail getAttendanceSessionDetails(int sessionId);

    // Admin fonksiyonları
    QVector<Teacher> getAllTeachers();
    QVector<CourseWithTeacher> getAllCoursesWithTeachers();
    bool addTeacher(const QString& username, const QString& password, const QString& fullName, const QString& email);
    bool addCourse(const QString& courseName, const QString& courseCode, int teacherId, int createdBy);
    bool changePassword(int userId, const QString& newPassword);
    bool changeOwnPassword(int userId, const QString& currentPassword, const QString& newPassword);
    bool changeEmail(int userId, const QString& currentEmail, const QString& newEmail);
    bool changePasswordByEmail(const QString& username, const QString& email, const QString& newPassword);
    bool changeTeacherPassword(const QString& username, const QString& newPassword);
    bool removeTeacher(const QString& username);

    // Yoklama silme istekleri
    bool requestAttendanceDeletion(int sessionId, int teacherId, const QString& reason);
    QVector<AttendanceDeleteRequest> getPendingDeleteRequests();
    bool approveDeleteRequest(int requestId);
    bool rejectDeleteRequest(int requestId);
    bool deleteAttendanceSession(int sessionId);
    
    // Admin yoklama genel bakış
    QVector<AdminAttendanceOverview> getAdminAttendanceOverview();
    QVector<AttendanceDetail> getAttendanceDetails(int sessionId);

    // Ders yönetimi
    QVector<Course> getAllCourses();
    QVector<Course> getCoursesByTeacher(int teacherId);
    bool deleteCourse(int courseId);
    bool assignCourseToTeacher(int courseId, int teacherId);

    // Yoklama oturumu yönetimi
    bool startAttendanceSession(int teacherId, int courseId, const QString& title);
    bool endAttendanceSession(int sessionId);
    QVector<AttendanceSession> getTeacherAttendanceHistory(int teacherId);
    int getActiveSessionId(int teacherId);
    int getCourseIdForSession(int sessionId);
    int getLastInsertId() const;

    // Debug fonksiyonları
    void debugDatabaseTables();

    QVariant authenticateStudent(const QString& studentNumber, const QString& password, User& user);

    QVector<Course> getCoursesForStudent(int studentId);

    bool changeStudentPassword(const QString& studentNumber, const QString& newPassword);

    bool isStudentEnrolled(const QString& cardUid, int courseId);

    Student getStudentByNumber(const QString& studentNumber);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H 