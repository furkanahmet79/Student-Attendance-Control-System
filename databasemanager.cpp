#include "databasemanager.h"
#include <QDebug>
#include <QDateTime>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
{
    // Singleton, constructor'ı özel
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::openDatabase(const QString &path)
{
    // Birden fazla bağlantı oluşmasını önlemek için bağlantı adı belirtiyoruz.
    m_db = QSqlDatabase::database("yoklama_connection", false);
    if (!m_db.isValid()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "yoklama_connection");
        m_db.setDatabaseName(path);
    }

    if (!m_db.open()) {
        qDebug() << "Veritabanı bağlantı hatası:" << m_db.lastError().text();
        return false;
    }
    qDebug() << "Veritabanına başarıyla bağlanıldı.";
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.removeDatabase("yoklama_connection");
    }
}

QVariant DatabaseManager::authenticateUser(const QString& username, const QString& password, User& user)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, role, fullName FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        return query.lastError().text();
    }
    
    if (query.next()) {
        user.id = query.value("id").toInt();
        user.role = query.value("role").toString();
        user.fullName = query.value("fullName").toString();
        return true;
    }
    return false;
}

QVector<Course> DatabaseManager::getCoursesForTeacher(int teacherId)
{
    QVector<Course> courses;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, course_name, course_code, teacher_id, created_by, created_at FROM courses WHERE teacher_id = :teacherId");
    query.bindValue(":teacherId", teacherId);
    if (query.exec()) {
        while (query.next()) {
            Course course;
            course.id = query.value(0).toInt();
            course.courseName = query.value(1).toString();
            course.courseCode = query.value(2).toString();
            course.teacherId = query.value(3).toInt();
            course.createdBy = query.value(4).toInt();
            course.createdAt = QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd HH:mm:ss");
            courses.append(course);
        }
    }
    return courses;
}

QVector<Student> DatabaseManager::getStudentsForCourse(int courseId)
{
    QVector<Student> students;
    QSqlQuery query(m_db);
    query.prepare("SELECT s.id, s.studentNumber, s.firstName, s.lastName, s.cardUID FROM students s "
                  "JOIN enrollments e ON s.id = e.studentId WHERE e.courseId = :courseId "
                  "ORDER BY s.lastName, s.firstName");
    query.bindValue(":courseId", courseId);
    
    if (query.exec()) {
        while(query.next()) {
            students.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString(), query.value(3).toString(), query.value(4).toString()});
        }
    }
    return students;
}

bool DatabaseManager::startAttendanceSession(int teacherId, int courseId, const QString& title)
{
    QSqlQuery query(m_db);
    
    // SQL sorgusunu debug et
    QString sqlQuery = "INSERT INTO attendance_sessions (teacher_id, course_id, title, start_time, is_active) VALUES (?, ?, ?, datetime('now'), 1)";
    qDebug() << "SQL sorgusu:" << sqlQuery;
    
    query.prepare(sqlQuery);
    query.addBindValue(teacherId);
    query.addBindValue(courseId);
    query.addBindValue(title);

    if (query.exec()) {
        qDebug() << "Yoklama oturumu başlatıldı:" << title;
        return true;
    } else {
        qDebug() << "Yoklama oturumu başlatılırken hata:" << query.lastError().text();
        qDebug() << "Parametreler: teacherId=" << teacherId << ", courseId=" << courseId << ", title=" << title;
        return false;
    }
}

bool DatabaseManager::endAttendanceSession(int sessionId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE attendance_sessions SET end_time = datetime('now'), is_active = 0 "
                  "WHERE id = ?");
    query.addBindValue(sessionId);
    
    if (query.exec()) {
        qDebug() << "Yoklama oturumu tamamlandı:" << sessionId;
        return true;
    } else {
        qDebug() << "Yoklama oturumu tamamlanırken hata:" << query.lastError().text();
        return false;
    }
}

Student DatabaseManager::getStudentByCardUID(const QString& cardUID)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, studentNumber, firstName, lastName, cardUID FROM students WHERE cardUID = :cardUID");
    query.bindValue(":cardUID", cardUID);
    if(query.exec() && query.next()) {
        return {query.value("id").toInt(), query.value("studentNumber").toString(), query.value("firstName").toString(), query.value("lastName").toString(), query.value("cardUID").toString()};
    }
    return Student(); // id = -1 olan boş öğrenci döner
}

bool DatabaseManager::isStudentEnrolled(const QString &cardUid, int courseId)
{
    if (!m_db.isOpen()) {
        qCritical() << "Veritabanı kapalı!";
        return false; 
    }

    QSqlQuery query;
    query.prepare("SELECT 1 FROM enrollments e "
                  "JOIN students s ON e.student_id = s.id "
                  "WHERE s.card_uid = :card_uid AND e.course_id = :course_id");
    query.bindValue(":card_uid", cardUid);
    query.bindValue(":course_id", courseId);

    if (!query.exec()) {
        qCritical() << "isStudentEnrolled sorgusu başarısız:" << query.lastError().text();
        return false;
    }

    return query.next();
}

bool DatabaseManager::enrollStudentToCourse(int studentId, int courseId, int teacherId)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO enrollments (courseId, studentId, enrolledAt, enrolledBy) VALUES (:courseId, :studentId, :enrolledAt, :enrolledBy)");
    query.bindValue(":courseId", courseId);
    query.bindValue(":studentId", studentId);
    query.bindValue(":enrolledAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":enrolledBy", teacherId);
    return query.exec();
}

Student DatabaseManager::addNewStudentAndEnroll(const QString& cardUID, const QString& studentNumber, const QString& firstName, const QString& lastName, int teacherId, int courseId)
{
    m_db.transaction();
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO students (cardUID, studentNumber, firstName, lastName, createdAt, createdBy) "
                  "VALUES (:cardUID, :studentNumber, :firstName, :lastName, :createdAt, :createdBy)");
    query.bindValue(":cardUID", cardUID);
    query.bindValue(":studentNumber", studentNumber);
    query.bindValue(":firstName", firstName);
    query.bindValue(":lastName", lastName);
    query.bindValue(":createdAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":createdBy", teacherId);

    if (!query.exec()) {
        qDebug() << "Yeni öğrenci ekleme hatası:" << query.lastError().text();
        m_db.rollback();
        return Student();
    }
    
    int newStudentId = query.lastInsertId().toInt();
    
    if(!enrollStudentToCourse(newStudentId, courseId, teacherId)) {
        qDebug() << "Yeni öğrenciyi derse kaydetme hatası:" << query.lastError().text();
        m_db.rollback();
        return Student();
    }

    m_db.commit();
    
    Student newStudent;
    newStudent.id = newStudentId;
    newStudent.firstName = firstName;
    newStudent.lastName = lastName;
    return newStudent;
}

bool DatabaseManager::markStudentPresent(int sessionId, int studentId, int& rowsAffected)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO attendanceRecords (sessionId, studentId, time, status) "
                        "VALUES (:sessionId, :studentId, :time, 'present')");
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":studentId", studentId);
    query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (query.exec()) {
        rowsAffected = query.numRowsAffected();
        return true;
    }
    qDebug() << "Yoklamaya ekleme hatası:" << query.lastError().text();
    return false;
}

QVector<AttendanceRecord> DatabaseManager::getAttendanceForSession(int sessionId)
{
    QVector<AttendanceRecord> records;
    QSqlQuery query(m_db);
    query.prepare("SELECT s.studentNumber, s.firstName, s.lastName, r.time FROM students s "
                  "JOIN attendanceRecords r ON s.id = r.studentId "
                  "WHERE r.sessionId = :sessionId ORDER BY r.time DESC");
    query.bindValue(":sessionId", sessionId);
    
    if (query.exec()) {
        while(query.next()) {
            QDateTime time = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
            records.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toString(), time.toString("HH:mm:ss")});
        }
    }
    return records;
}

// ===================================================================
//   ADMİN FONKSİYONLARI
// ===================================================================

QVector<Teacher> DatabaseManager::getAllTeachers()
{
    QVector<Teacher> teachers;
    QSqlQuery query(m_db);
    if (query.exec("SELECT id, fullName, username, email, role FROM users WHERE role = 'teacher' ORDER BY fullName")) {
        while (query.next()) {
            Teacher teacher;
            teacher.id = query.value(0).toInt();
            teacher.fullName = query.value(1).toString();
            teacher.username = query.value(2).toString();
            teacher.email = query.value(3).toString();
            teacher.role = query.value(4).toString();
            teachers.append(teacher);
        }
    } else {
        qDebug() << "Öğretmen listesi alınamadı:" << query.lastError().text();
    }
    return teachers;
}

QVector<CourseWithTeacher> DatabaseManager::getAllCoursesWithTeachers()
{
    QVector<CourseWithTeacher> courses;
    QSqlQuery query("SELECT c.id, c.course_name, c.course_code, c.teacher_id, "
                    "u.fullName as teacher_name, c.created_by, c.created_at "
                    "FROM courses c "
                    "LEFT JOIN users u ON c.teacher_id = u.id "
                    "ORDER BY c.course_name");
    
        while (query.next()) {
        CourseWithTeacher course;
        course.id = query.value(0).toInt();
        course.courseName = query.value(1).toString();
        course.courseCode = query.value(2).toString();
        course.teacherId = query.value(3).toInt();
        course.teacherName = query.value(4).toString();
        course.createdBy = query.value(5).toInt();
        course.createdAt = QDateTime::fromString(query.value(6).toString(), "yyyy-MM-dd HH:mm:ss");
        courses.append(course);
    }
    
    return courses;
}

bool DatabaseManager::addTeacher(const QString& username, const QString& password, const QString& fullName, const QString& email)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, fullName, role, email, createdAt) "
                  "VALUES (:username, :password, :fullName, 'teacher', :email, :createdAt)");
    query.bindValue(":username", username);
    query.bindValue(":password", password); // Gerçek uygulamada şifrelenmeli
    query.bindValue(":fullName", fullName);
    query.bindValue(":email", email);
    query.bindValue(":createdAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if(!query.exec()) {
        qDebug() << "Öğretmen ekleme hatası:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addCourse(const QString& courseName, const QString& courseCode, int teacherId, int createdBy)
{
    QSqlQuery query;
    query.prepare("INSERT INTO courses (course_name, course_code, teacher_id, created_by, created_at) "
                  "VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)");
    query.addBindValue(courseName);
    query.addBindValue(courseCode);
    query.addBindValue(teacherId);
    query.addBindValue(createdBy);
    
    if (query.exec()) {
        qDebug() << "Ders başarıyla eklendi:" << courseName;
        return true;
    } else {
        qDebug() << "Ders eklenirken hata:" << query.lastError().text();
        return false;
    }
}

QVector<Course> DatabaseManager::getAllCourses()
{
    QVector<Course> courses;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, course_name, course_code, teacher_id, created_by, created_at "
                    "FROM courses ORDER BY course_name");
    
    if (query.exec()) {
    while (query.next()) {
        Course course;
        course.id = query.value(0).toInt();
        course.courseName = query.value(1).toString();
        course.courseCode = query.value(2).toString();
        course.teacherId = query.value(3).toInt();
        course.createdBy = query.value(4).toInt();
        course.createdAt = QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd HH:mm:ss");
        courses.append(course);
        }
    } else {
        qDebug() << "Tüm dersler alınamadı:" << query.lastError().text();
    }
    
    return courses;
}

QVector<Course> DatabaseManager::getCoursesByTeacher(int teacherId)
{
    QVector<Course> courses;
    QSqlQuery query;
    query.prepare("SELECT id, course_name, course_code, teacher_id, created_by, created_at "
                  "FROM courses WHERE teacher_id = ? ORDER BY course_name");
    query.addBindValue(teacherId);
    
    if (query.exec()) {
        while (query.next()) {
            Course course;
            course.id = query.value(0).toInt();
            course.courseName = query.value(1).toString();
            course.courseCode = query.value(2).toString();
            course.teacherId = query.value(3).toInt();
            course.createdBy = query.value(4).toInt();
            course.createdAt = QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd HH:mm:ss");
            courses.append(course);
        }
    }
    
    return courses;
}

bool DatabaseManager::deleteCourse(int courseId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM courses WHERE id = ?");
    query.addBindValue(courseId);
    
    if (query.exec()) {
        qDebug() << "Ders başarıyla silindi:" << courseId;
        return true;
    } else {
        qDebug() << "Ders silinirken hata:" << query.lastError().text();
        return false;
    }
}

QVector<AttendanceSession> DatabaseManager::getTeacherAttendanceHistory(int teacherId)
{
    QVector<AttendanceSession> sessions;
    QSqlQuery query(m_db);
    
    // SQL sorgusunu debug et
    QString sqlQuery = "SELECT s.id, s.title, s.start_time, s.end_time, s.is_active, c.course_name, c.course_code, "
                      "(SELECT COUNT(*) FROM attendanceRecords WHERE sessionId = s.id) as student_count "
                      "FROM attendance_sessions s "
                      "JOIN courses c ON s.course_id = c.id "
                      "WHERE s.teacher_id = :teacherId "
                      "ORDER BY s.start_time DESC";
    
    qDebug() << "SQL sorgusu:" << sqlQuery;
    
    query.prepare(sqlQuery);
    query.bindValue(":teacherId", teacherId);
    
    if (query.exec()) {
        while (query.next()) {
            AttendanceSession session;
            session.id = query.value("id").toInt();
            session.title = query.value("title").toString();
            QDateTime utcStart = QDateTime::fromString(query.value("start_time").toString(), "yyyy-MM-dd HH:mm:ss");
            utcStart.setTimeSpec(Qt::UTC);
            session.startTime = utcStart.toLocalTime();
            QDateTime utcEnd = QDateTime::fromString(query.value("end_time").toString(), "yyyy-MM-dd HH:mm:ss");
            utcEnd.setTimeSpec(Qt::UTC);
            session.endTime = utcEnd.toLocalTime();
            session.isActive = query.value("is_active").toBool();
            session.courseName = query.value("course_name").toString();
            session.courseCode = query.value("course_code").toString();
            session.studentCount = query.value("student_count").toInt();
            sessions.append(session);
        }
    } else {
        qDebug() << "Yoklama geçmişi alınırken hata:" << query.lastError().text();
    }
    
    return sessions;
}

QVector<AttendanceSession> DatabaseManager::getAttendanceSessionsForCourse(int courseId)
{
    QVector<AttendanceSession> sessions;
    QSqlQuery query(m_db);
    
    // SQL sorgusunu debug et
    QString sqlQuery = "SELECT s.id, s.title, s.start_time, s.end_time, s.is_active, c.course_name, c.course_code, "
                      "(SELECT COUNT(*) FROM attendanceRecords WHERE sessionId = s.id) as student_count "
                      "FROM attendance_sessions s "
                      "JOIN courses c ON s.course_id = c.id "
                      "WHERE s.course_id = :courseId "
                      "ORDER BY s.start_time DESC";
    
    qDebug() << "SQL sorgusu:" << sqlQuery;
    
    query.prepare(sqlQuery);
    query.bindValue(":courseId", courseId);
    
    if (query.exec()) {
        while (query.next()) {
            AttendanceSession session;
            session.id = query.value("id").toInt();
            session.title = query.value("title").toString();
            QDateTime utcStart = QDateTime::fromString(query.value("start_time").toString(), "yyyy-MM-dd HH:mm:ss");
            utcStart.setTimeSpec(Qt::UTC);
            session.startTime = utcStart.toLocalTime();
            QDateTime utcEnd = QDateTime::fromString(query.value("end_time").toString(), "yyyy-MM-dd HH:mm:ss");
            utcEnd.setTimeSpec(Qt::UTC);
            session.endTime = utcEnd.toLocalTime();
            session.isActive = query.value("is_active").toBool();
            session.courseName = query.value("course_name").toString();
            session.courseCode = query.value("course_code").toString();
            session.studentCount = query.value("student_count").toInt();
            sessions.append(session);
        }
    } else {
        qDebug() << "Ders için yoklama geçmişi alınırken hata:" << query.lastError().text();
    }
    
    return sessions;
}

int DatabaseManager::getActiveSessionId(int teacherId)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM attendance_sessions WHERE teacher_id = ? AND is_active = 1");
    query.addBindValue(teacherId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseManager::getCourseIdForSession(int sessionId)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT course_id FROM attendance_sessions WHERE id = ?");
    query.addBindValue(sessionId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

AttendanceSessionDetail DatabaseManager::getAttendanceSessionDetails(int sessionId)
{
    qDebug() << "getAttendanceSessionDetails çağrıldı, sessionId:" << sessionId;
    
    AttendanceSessionDetail detail;
    QSqlQuery query(m_db);
    
    // Oturum bilgilerini al
    query.prepare("SELECT id, title, start_time, end_time, is_active FROM attendance_sessions WHERE id = :sessionId");
    query.bindValue(":sessionId", sessionId);
    
    if (query.exec() && query.next()) {
        detail.sessionId = query.value("id").toInt();
        detail.title = query.value("title").toString();
        QDateTime utc = QDateTime::fromString(query.value("start_time").toString(), "yyyy-MM-dd HH:mm:ss");
        utc.setTimeSpec(Qt::UTC);
        QDateTime local = utc.toLocalTime();
        detail.date = local.toString("dd.MM.yyyy");
        detail.startTime = local.toString("HH:mm");
        detail.endTime = query.value("end_time").toString();
        detail.status = query.value("is_active").toBool() ? "active" : "completed";
        
        qDebug() << "Oturum bilgileri alındı:";
        qDebug() << "- title:" << detail.title;
        qDebug() << "- date:" << detail.date;
        qDebug() << "- status:" << detail.status;
        
        // Yoklama kayıtlarını al
        detail.records = getAttendanceForSession(sessionId);
        qDebug() << "Yoklama kayıtları alındı, count:" << detail.records.count();
    } else {
        qDebug() << "Oturum bilgileri alınamadı:" << query.lastError().text();
    }
    
    return detail;
}

bool DatabaseManager::changePassword(int userId, const QString& newPassword)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET password = :password WHERE id = :userId");
    query.bindValue(":password", newPassword);
    query.bindValue(":userId", userId);
    
    if (!query.exec()) {
        qDebug() << "Şifre değiştirme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Şifre başarıyla değiştirildi, userId:" << userId;
    return true;
}

bool DatabaseManager::changeOwnPassword(int userId, const QString& currentPassword, const QString& newPassword)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT password FROM users WHERE id = :userId");
    query.bindValue(":userId", userId);
    
    if (!query.exec() || !query.next()) {
        qDebug() << "Kullanıcı bulunamadı:" << query.lastError().text();
        return false;
    }
    
    QString storedPassword = query.value("password").toString();
    if (storedPassword != currentPassword) {
        qDebug() << "Mevcut şifre yanlış";
        return false;
    }
    
    return changePassword(userId, newPassword);
}

bool DatabaseManager::changeEmail(int userId, const QString& currentEmail, const QString& newEmail)
{
    QSqlQuery query(m_db);
    
    // Önce mevcut email'i kontrol et
    query.prepare("SELECT email FROM users WHERE id = :userId");
    query.bindValue(":userId", userId);
    
    if (!query.exec() || !query.next()) {
        qDebug() << "Kullanıcı bulunamadı:" << query.lastError().text();
        return false;
    }
    
    QString storedEmail = query.value("email").toString();
    if (storedEmail != currentEmail) {
        qDebug() << "Mevcut e-posta yanlış";
        return false;
    }
    
    // Yeni email'in başka bir kullanıcı tarafından kullanılıp kullanılmadığını kontrol et
    query.prepare("SELECT id FROM users WHERE email = :email AND id != :userId");
    query.bindValue(":email", newEmail);
    query.bindValue(":userId", userId);
    
    if (query.exec() && query.next()) {
        qDebug() << "Bu e-posta adresi başka bir kullanıcı tarafından kullanılıyor";
        return false;
    }
    
    // Email'i güncelle
    query.prepare("UPDATE users SET email = :email WHERE id = :userId");
    query.bindValue(":email", newEmail);
    query.bindValue(":userId", userId);
    
    if (!query.exec()) {
        qDebug() << "E-posta değiştirme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "E-posta başarıyla değiştirildi, userId:" << userId;
    return true;
}

bool DatabaseManager::changePasswordByEmail(const QString& username, const QString& email, const QString& newPassword)
{
    QSqlQuery query(m_db);
    
    // Kullanıcı adı ve email'i doğrula
    query.prepare("SELECT id FROM users WHERE username = :username AND email = :email");
    query.bindValue(":username", username);
    query.bindValue(":email", email);
    
    if (!query.exec() || !query.next()) {
        qDebug() << "Kullanıcı adı veya e-posta bulunamadı";
        return false;
    }
    
    int userId = query.value("id").toInt();
    
    // Şifreyi değiştir
    query.prepare("UPDATE users SET password = :password WHERE id = :userId");
    query.bindValue(":password", newPassword);
    query.bindValue(":userId", userId);
    
    if (!query.exec()) {
        qDebug() << "Şifre değiştirme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Şifre e-posta ile başarıyla değiştirildi, userId:" << userId;
    return true;
}

// ===================================================================
//   YOKLAMA SİLME İSTEKLERİ
// ===================================================================

bool DatabaseManager::requestAttendanceDeletion(int sessionId, int teacherId, const QString& reason)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO attendanceDeleteRequests (sessionId, requestedBy, requestedAt, reason, status) "
                  "VALUES (:sessionId, :requestedBy, :requestedAt, :reason, 'pending')");
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":requestedBy", teacherId);
    query.bindValue(":requestedAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":reason", reason);
    
    if (!query.exec()) {
        qDebug() << "Silme isteği ekleme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Silme isteği başarıyla eklendi, sessionId:" << sessionId;
    return true;
}

QVector<AttendanceDeleteRequest> DatabaseManager::getPendingDeleteRequests()
{
    QVector<AttendanceDeleteRequest> requests;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT r.id, r.sessionId, r.requestedAt, r.reason, r.status, "
                  "s.title as sessionTitle, u.fullName as teacherName "
                  "FROM attendanceDeleteRequests r "
                  "JOIN attendance_sessions s ON r.sessionId = s.id "
                  "JOIN users u ON r.requestedBy = u.id "
                  "WHERE r.status = 'pending' "
                  "ORDER BY r.requestedAt DESC");
    
    if (query.exec()) {
        while (query.next()) {
            AttendanceDeleteRequest request;
            request.requestId = query.value("id").toInt();
            request.sessionId = query.value("sessionId").toInt();
            request.sessionTitle = query.value("sessionTitle").toString();
            request.teacherName = query.value("teacherName").toString();
            request.requestedAt = query.value("requestedAt").toString();
            request.reason = query.value("reason").toString();
            request.status = query.value("status").toString();
            requests.append(request);
        }
    } else {
        qDebug() << "Silme istekleri alınamadı:" << query.lastError().text();
    }
    
    return requests;
}

bool DatabaseManager::approveDeleteRequest(int requestId)
{
    m_db.transaction();
    
    // İsteği onayla
    QSqlQuery query(m_db);
    query.prepare("UPDATE attendanceDeleteRequests SET status = 'approved', approvedAt = :approvedAt "
                  "WHERE id = :requestId");
    query.bindValue(":approvedAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":requestId", requestId);
    
    if (!query.exec()) {
        qDebug() << "İstek onaylama hatası:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    // Session ID'yi al
    query.prepare("SELECT sessionId FROM attendanceDeleteRequests WHERE id = :requestId");
    query.bindValue(":requestId", requestId);
    
    if (!query.exec() || !query.next()) {
        qDebug() << "Session ID alınamadı:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    int sessionId = query.value("sessionId").toInt();
    
    // Yoklamayı sil
    if (!deleteAttendanceSession(sessionId)) {
        m_db.rollback();
        return false;
    }
    
    m_db.commit();
    qDebug() << "Silme isteği onaylandı ve yoklama silindi, requestId:" << requestId;
    return true;
}

bool DatabaseManager::rejectDeleteRequest(int requestId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE attendanceDeleteRequests SET status = 'rejected', approvedAt = :approvedAt "
                  "WHERE id = :requestId");
    query.bindValue(":approvedAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":requestId", requestId);
    
    if (!query.exec()) {
        qDebug() << "İstek reddetme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Silme isteği reddedildi, requestId:" << requestId;
    return true;
}

bool DatabaseManager::deleteAttendanceSession(int sessionId)
{
    m_db.transaction();
    
    // Önce yoklama kayıtlarını sil
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM attendanceRecords WHERE sessionId = :sessionId");
    query.bindValue(":sessionId", sessionId);
    
    if (!query.exec()) {
        qDebug() << "Yoklama kayıtları silinemedi:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    // Sonra yoklama oturumunu sil
    query.prepare("DELETE FROM attendance_sessions WHERE id = :sessionId");
    query.bindValue(":sessionId", sessionId);
    
    if (!query.exec()) {
        qDebug() << "Yoklama oturumu silinemedi:" << query.lastError().text();
        m_db.rollback();
        return false;
    }
    
    m_db.commit();
    qDebug() << "Yoklama oturumu ve kayıtları başarıyla silindi:" << sessionId;
    return true;
}

// ===================================================================
//   ADMİN YOKLAMA GENEL BAKIŞ
// ===================================================================

QVector<AdminAttendanceOverview> DatabaseManager::getAdminAttendanceOverview()
{
    QVector<AdminAttendanceOverview> overview;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT s.id, s.title, s.start_time, s.is_active, "
                  "c.course_name, u.fullName as teacherName, "
                  "COUNT(r.id) as studentCount "
                  "FROM attendance_sessions s "
                  "JOIN courses c ON s.course_id = c.id "
                  "JOIN users u ON s.teacher_id = u.id "
                  "LEFT JOIN attendanceRecords r ON s.id = r.sessionId "
                  "GROUP BY s.id, s.title, s.start_time, s.is_active, c.course_name, u.fullName "
                  "ORDER BY s.start_time DESC");
    
    if (query.exec()) {
        while (query.next()) {
            AdminAttendanceOverview item;
            item.sessionId = query.value("id").toInt();
            item.sessionTitle = query.value("title").toString();
            item.teacherName = query.value("teacherName").toString();
            item.courseName = query.value("course_name").toString();
            QDateTime utc = QDateTime::fromString(query.value("start_time").toString(), "yyyy-MM-dd HH:mm:ss");
            utc.setTimeSpec(Qt::UTC);
            QDateTime local = utc.toLocalTime();
            item.date = local.toString("dd.MM.yyyy");
            item.startTime = local.toString("HH:mm");
            item.status = query.value("is_active").toBool() ? "active" : "completed";
            item.studentCount = query.value("studentCount").toInt();
            overview.append(item);
        }
    } else {
        qDebug() << "Yoklama genel bakış alınamadı:" << query.lastError().text();
    }
    
    return overview;
}

int DatabaseManager::getLastInsertId() const
{
    QSqlQuery query(m_db);
    query.exec("SELECT last_insert_rowid()");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

// ===================================================================
//   DERS-ÖĞRETMEN ATAMA
// ===================================================================

bool DatabaseManager::assignCourseToTeacher(int courseId, int teacherId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE courses SET teacher_id = :teacherId WHERE id = :courseId");
    query.bindValue(":teacherId", teacherId);
    query.bindValue(":courseId", courseId);
    
    if (!query.exec()) {
        qDebug() << "Ders atama hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Ders başarıyla öğretmene atandı, courseId:" << courseId << "teacherId:" << teacherId;
    return true;
}

// ===================================================================
//   EKSİK FONKSİYONLAR
// ===================================================================

QVector<AttendanceDetail> DatabaseManager::getAttendanceDetails(int sessionId)
{
    QVector<AttendanceDetail> details;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT s.studentNumber, s.firstName, s.lastName, "
                  "CASE WHEN ar.id IS NOT NULL THEN 'present' ELSE 'absent' END as status "
                  "FROM students s "
                  "JOIN enrollments e ON s.id = e.studentId "
                  "LEFT JOIN attendanceRecords ar ON s.id = ar.studentId AND ar.sessionId = :sessionId "
                  "WHERE e.courseId = :courseId "
                  "ORDER BY s.lastName, s.firstName");
    
    // Önce bu yoklama oturumunun ders ID'sini al
    QSqlQuery courseQuery(m_db);
    courseQuery.prepare("SELECT course_id FROM attendance_sessions WHERE id = :sessionId");
    courseQuery.bindValue(":sessionId", sessionId);
    
    int courseId = -1;
    if (courseQuery.exec() && courseQuery.next()) {
        courseId = courseQuery.value(0).toInt();
    } else {
        qDebug() << "Ders ID alınamadı:" << courseQuery.lastError().text();
        return details;
    }
    
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":courseId", courseId);
    
    if (query.exec()) {
        while (query.next()) {
            AttendanceDetail detail;
            detail.studentNumber = query.value("studentNumber").toString();
            detail.firstName = query.value("firstName").toString();
            detail.lastName = query.value("lastName").toString();
            detail.status = query.value("status").toString();
            details.append(detail);
        }
    } else {
        qDebug() << "Yoklama detayları alınamadı:" << query.lastError().text();
    }
    
    return details;
}

bool DatabaseManager::changeTeacherPassword(const QString& username, const QString& newPassword)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET password = :password WHERE username = :username AND role = 'teacher'");
    query.bindValue(":password", newPassword);
    query.bindValue(":username", username);
    
    if (!query.exec()) {
        qDebug() << "Öğretmen şifre değiştirme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Öğretmen şifresi değiştirildi, username:" << username;
    return true;
}

bool DatabaseManager::removeTeacher(const QString& username)
{
    // Önce öğretmenin derslerini kontrol et
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM courses WHERE teacher_id = (SELECT id FROM users WHERE username = :username)");
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        int courseCount = query.value(0).toInt();
        if (courseCount > 0) {
            qDebug() << "Öğretmenin dersleri var, silinemez:" << username;
            return false;
        }
    }
    
    // Öğretmeni sil
    query.prepare("DELETE FROM users WHERE username = :username AND role = 'teacher'");
    query.bindValue(":username", username);
    
    if (!query.exec()) {
        qDebug() << "Öğretmen silme hatası:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Öğretmen silindi, username:" << username;
    return true;
}

void DatabaseManager::debugDatabaseTables()
{
    qDebug() << "=== VERİTABANI TABLOLARI DEBUG ===";
    
    // Tüm tabloları listele
    QSqlQuery query(m_db);
    query.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
    qDebug() << "Mevcut tablolar:";
    while (query.next()) {
        QString tableName = query.value(0).toString();
        qDebug() << "- Tablo:" << tableName;
        
        // Her tablonun sütunlarını listele
        QSqlQuery columnsQuery(m_db);
        columnsQuery.exec("PRAGMA table_info(" + tableName + ")");
        while (columnsQuery.next()) {
            qDebug() << "  - Sütun:" << columnsQuery.value(1).toString() 
                     << "Tip:" << columnsQuery.value(2).toString();
        }
    }
    
    qDebug() << "=== DEBUG TAMAMLANDI ===";
}

QVariant DatabaseManager::authenticateStudent(const QString& studentNumber, const QString& password, User& user)
{
    QSqlQuery query(m_db);
    // Şimdilik sadece öğrenci numarası ile giriş (şifre yoksa)
    query.prepare("SELECT id, firstName, lastName FROM students WHERE studentNumber = :studentNumber");
    query.bindValue(":studentNumber", studentNumber);
    if (!query.exec()) {
        return query.lastError().text();
    }
    if (query.next()) {
        user.id = query.value("id").toInt();
        user.role = "student";
        user.fullName = query.value("firstName").toString() + " " + query.value("lastName").toString();
        return true;
    }
    return false;
}

QVector<Course> DatabaseManager::getCoursesForStudent(int studentId)
{
    QVector<Course> courses;
    QSqlQuery query(m_db);
    query.prepare("SELECT c.id, c.course_name, c.course_code, c.teacher_id, c.created_by, c.created_at "
                  "FROM courses c "
                  "JOIN enrollments e ON c.id = e.courseId "
                  "WHERE e.studentId = :studentId");
    query.bindValue(":studentId", studentId);
    if (query.exec()) {
        while (query.next()) {
            Course course;
            course.id = query.value(0).toInt();
            course.courseName = query.value(1).toString();
            course.courseCode = query.value(2).toString();
            course.teacherId = query.value(3).toInt();
            course.createdBy = query.value(4).toInt();
            course.createdAt = QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd HH:mm:ss");
            courses.append(course);
        }
    }
    return courses;
}

bool DatabaseManager::changeStudentPassword(const QString& studentNumber, const QString& newPassword)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE students SET password = :password WHERE studentNumber = :studentNumber");
    query.bindValue(":password", newPassword);
    query.bindValue(":studentNumber", studentNumber);
    return query.exec() && query.numRowsAffected() > 0;
}

Student DatabaseManager::getStudentById(int studentId)
{
    Student student;
    if (!m_db.isOpen()) return student;

    QSqlQuery query;
    query.prepare("SELECT id, student_number, first_name, last_name, card_uid FROM students WHERE id = :id");
    query.bindValue(":id", studentId);

    if (query.exec() && query.next()) {
        student.id = query.value(0).toInt();
        student.studentNumber = query.value(1).toString();
        student.firstName = query.value(2).toString();
        student.lastName = query.value(3).toString();
        student.cardUID = query.value(4).toString();
    }
    return student;
}

Student DatabaseManager::getStudentByNumber(const QString& studentNumber)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id, studentNumber, firstName, lastName, cardUID FROM students WHERE studentNumber = :studentNumber");
    query.bindValue(":studentNumber", studentNumber);
    if (query.exec() && query.next()) {
        Student student;
        student.id = query.value("id").toInt();
        student.studentNumber = query.value("studentNumber").toString();
        student.firstName = query.value("firstName").toString();
        student.lastName = query.value("lastName").toString();
        student.cardUID = query.value("cardUID").toString();
        return student;
    }
    return Student(); // id = -1 olan boş öğrenci
} 