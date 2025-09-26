#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QUuid>

// Fonksiyonu dışarıdan çağrılabilir hale getir
extern "C" bool createDatabase() {
    // SQLite veritabanı bağlantısı oluştur
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("yoklama_sistemi.db");
    
    if (!db.open()) {
        qDebug() << "Veritabanı açılamadı:" << db.lastError().text();
        return false;
    }
    
    QSqlQuery query;
    
    // Tabloları oluştur
    
    // Kullanıcılar tablosu (admin ve öğretmenler)
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "username TEXT UNIQUE NOT NULL, "
                   "password TEXT NOT NULL, "
                   "fullName TEXT NOT NULL, "
                   "role TEXT NOT NULL, "
                   "email TEXT UNIQUE NOT NULL, "
                   "createdAt TEXT NOT NULL)")) {
        qDebug() << "users tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Öğrenciler tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS students ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "cardUID TEXT UNIQUE NOT NULL, "
                   "studentNumber TEXT UNIQUE NOT NULL, "
                   "firstName TEXT NOT NULL, "
                   "lastName TEXT NOT NULL, "
                   "createdAt TEXT NOT NULL, "
                   "createdBy INTEGER NOT NULL, "
                   "FOREIGN KEY (createdBy) REFERENCES users(id))")) {
        qDebug() << "students tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Eğer students tablosunda password sütunu yoksa ekle
    if (!query.exec("PRAGMA table_info(students)")) {
        qDebug() << "students tablosu bilgisi alınamadı:" << query.lastError().text();
    } else {
        bool hasPasswordColumn = false;
        while (query.next()) {
            if (query.value(1).toString() == "password") {
                hasPasswordColumn = true;
                break;
            }
        }
        if (!hasPasswordColumn) {
            if (!query.exec("ALTER TABLE students ADD COLUMN password TEXT")) {
                qDebug() << "students tablosuna password sütunu eklenemedi:" << query.lastError().text();
            } else {
                qDebug() << "students tablosuna password sütunu eklendi.";
            }
        }
    }
    
    // Dersler tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS courses ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "course_name TEXT NOT NULL,"
                   "course_code TEXT NOT NULL UNIQUE,"
                   "teacher_id INTEGER NOT NULL,"
                   "created_by INTEGER NOT NULL,"
                   "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                   "FOREIGN KEY (teacher_id) REFERENCES users(id),"
                   "FOREIGN KEY (created_by) REFERENCES users(id))")) {
        qDebug() << "courses tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Ders kayıtları tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS enrollments ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "courseId INTEGER NOT NULL, "
                   "studentId INTEGER NOT NULL, "
                   "enrolledAt TEXT NOT NULL, "
                   "enrolledBy INTEGER NOT NULL, "
                   "FOREIGN KEY (courseId) REFERENCES courses(id), "
                   "FOREIGN KEY (studentId) REFERENCES students(id), "
                   "FOREIGN KEY (enrolledBy) REFERENCES users(id), "
                   "UNIQUE(courseId, studentId))")) {
        qDebug() << "enrollments tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Yoklama oturumları tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS attendance_sessions ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "teacher_id INTEGER NOT NULL,"
                   "course_id INTEGER NOT NULL,"
                   "title TEXT NOT NULL,"
                   "start_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
                   "end_time DATETIME,"
                   "is_active BOOLEAN DEFAULT 1,"
                   "FOREIGN KEY (teacher_id) REFERENCES users(id),"
                   "FOREIGN KEY (course_id) REFERENCES courses(id))")) {
        qDebug() << "attendanceSessions tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Eğer tablo zaten varsa ve title sütunu yoksa ekle
    if (!query.exec("PRAGMA table_info(attendanceSessions)")) {
        qDebug() << "Tablo bilgisi alınamadı:" << query.lastError().text();
    } else {
        bool hasTitleColumn = false;
        while (query.next()) {
            if (query.value(1).toString() == "title") {
                hasTitleColumn = true;
                break;
            }
        }
        
        if (!hasTitleColumn) {
            if (!query.exec("ALTER TABLE attendanceSessions ADD COLUMN title TEXT NOT NULL DEFAULT 'Yoklama'")) {
                qDebug() << "title sütunu eklenemedi:" << query.lastError().text();
            } else {
                qDebug() << "title sütunu başarıyla eklendi.";
            }
        }
    }
    
    // Yoklama kayıtları tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS attendanceRecords ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "sessionId INTEGER NOT NULL, "
                   "studentId INTEGER NOT NULL, "
                   "time TEXT NOT NULL, "
                   "status TEXT NOT NULL, "
                   "FOREIGN KEY (sessionId) REFERENCES attendanceSessions(id), "
                   "FOREIGN KEY (studentId) REFERENCES students(id), "
                   "UNIQUE(sessionId, studentId))")) {
        qDebug() << "attendanceRecords tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Yoklama silme istekleri tablosu
    if (!query.exec("CREATE TABLE IF NOT EXISTS attendanceDeleteRequests ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "sessionId INTEGER NOT NULL, "
                   "requestedBy INTEGER NOT NULL, "
                   "requestedAt TEXT NOT NULL, "
                   "reason TEXT, "
                   "status TEXT NOT NULL DEFAULT 'pending', "
                   "approvedBy INTEGER, "
                   "approvedAt TEXT, "
                   "FOREIGN KEY (sessionId) REFERENCES attendanceSessions(id), "
                   "FOREIGN KEY (requestedBy) REFERENCES users(id), "
                   "FOREIGN KEY (approvedBy) REFERENCES users(id))")) {
        qDebug() << "attendanceDeleteRequests tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }
    
    // Admin kullanıcısını otomatik olarak ekle
    query.prepare("SELECT COUNT(*) FROM users WHERE username = ?");
    query.addBindValue("admin");
    if (!query.exec()) {
        qDebug() << "Admin kontrolü yapılamadı:" << query.lastError().text();
    } else {
    query.next();
    if (query.value(0).toInt() == 0) {
            // Admin kullanıcısını ekle
        query.prepare("INSERT INTO users (username, password, fullName, role, email, createdAt) "
                     "VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue("admin");
        query.addBindValue("admin123");
        query.addBindValue("Sistem Yöneticisi");
        query.addBindValue("admin");
        query.addBindValue("admin@okul.com");
        query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
        if (!query.exec()) {
            qDebug() << "Admin kullanıcısı eklenemedi:" << query.lastError().text();
        } else {
                qDebug() << "Admin kullanıcısı başarıyla eklendi (admin/admin123)";
            }
        } else {
            qDebug() << "Admin kullanıcısı zaten mevcut.";
    }
    }
    
    // Örnek öğretmenler ekle
    query.exec("INSERT OR IGNORE INTO users (username, password, fullName, role, email, createdAt) VALUES "
               "('o1', '123', 'Ahmet Yılmaz', 'teacher', 'ahmet@okul.com', '" + QDateTime::currentDateTime().toString(Qt::ISODate) + "'),"
               "('o2', '123', 'Ayşe Demir', 'teacher', 'ayse@okul.com', '" + QDateTime::currentDateTime().toString(Qt::ISODate) + "')");
    
    // Örnek dersler ekle (öğretmen ID'lerini al)
    query.exec("SELECT id FROM users WHERE role = 'teacher' ORDER BY id LIMIT 2");
    QVector<int> teacherIds;
    while (query.next()) {
        teacherIds.append(query.value(0).toInt());
    }
    
    if (teacherIds.size() >= 2) {
        query.exec(QString("INSERT OR IGNORE INTO courses (course_name, course_code, teacher_id, created_by) VALUES "
                   "('Matematik', 'MATH101', %1, 1),"
                   "('Fizik', 'PHYS101', %1, 1),"
                   "('Kimya', 'CHEM101', %2, 1),"
                   "('Biyoloji', 'BIO101', %2, 1)")
                   .arg(teacherIds[0]).arg(teacherIds[1]));
    }
    
    qDebug() << "Yoklama veritabanı ve tablolar başarıyla oluşturuldu!";
    
    // Tablo kayıt sayılarını göster
    query.exec("SELECT COUNT(*) FROM users");
    query.next();
    qDebug() << "- users:" << query.value(0).toInt() << "kayıt";
    
    query.exec("SELECT COUNT(*) FROM students");
    query.next();
    qDebug() << "- students:" << query.value(0).toInt() << "kayıt";
    
    query.exec("SELECT COUNT(*) FROM courses");
    query.next();
    qDebug() << "- courses:" << query.value(0).toInt() << "kayıt";
    
    query.exec("SELECT COUNT(*) FROM enrollments");
    query.next();
    qDebug() << "- enrollments:" << query.value(0).toInt() << "kayıt";
    
    query.exec("SELECT COUNT(*) FROM attendanceSessions");
    query.next();
    qDebug() << "- attendanceSessions:" << query.value(0).toInt() << "kayıt";
    
    query.exec("SELECT COUNT(*) FROM attendanceRecords");
    query.next();
    qDebug() << "- attendanceRecords:" << query.value(0).toInt() << "kayıt";
    
    return true;
} 