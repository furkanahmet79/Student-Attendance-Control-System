#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

// Veritabanı oluşturma fonksiyonu
extern "C" bool createDatabase();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Veritabanını oluştur
    if (!createDatabase()) {
        qDebug() << "Veritabanı oluşturulamadı!";
        return -1;
    }
    
    MainWindow w;
    w.show();
    return a.exec();
}
