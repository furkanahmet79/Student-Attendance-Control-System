#ifndef TABLEHELPER_H
#define TABLEHELPER_H

#include <QTableWidget>
#include <QHeaderView>
#include <QApplication>
#include <QScreen>

class TableHelper
{
public:
    // Tabloyu dinamik boyutlandır
    static void setupDynamicTable(QTableWidget* table, const QStringList& headers, 
                                 bool stretchLastColumn = true, bool alternateColors = true);
    
    // Sütunları içeriğe göre boyutlandır
    static void resizeColumnsToContent(QTableWidget* table);
    
    // Tabloyu salt okunur yap
    static void makeReadOnly(QTableWidget* table);
    
    // Tabloyu temizle
    static void clearTable(QTableWidget* table);
    
    // Satır ekle
    static void addRow(QTableWidget* table, const QStringList& data, 
                      const QVariant& userData = QVariant());
    
    // Satır ekle (QVariant listesi ile)
    static void addRow(QTableWidget* table, const QVariantList& data, 
                      const QVariant& userData = QVariant());
    
    // Tabloyu stil ile güzelleştir
    static void applyModernStyle(QTableWidget* table);

private:
    static QString getTextWidth(const QString& text);
};

#endif // TABLEHELPER_H 