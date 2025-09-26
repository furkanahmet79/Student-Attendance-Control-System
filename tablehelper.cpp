#include "tablehelper.h"
#include <QFontMetrics>
#include <QScrollBar>

void TableHelper::setupDynamicTable(QTableWidget* table, const QStringList& headers, 
                                   bool stretchLastColumn, bool alternateColors)
{
    // Sütun sayısını ayarla
    table->setColumnCount(headers.count());
    table->setHorizontalHeaderLabels(headers);
    
    // Header'ı ayarla
    QHeaderView* header = table->horizontalHeader();
    header->setStretchLastSection(stretchLastColumn);
    header->setSectionsClickable(true);
    header->setSectionsMovable(false);
    
    // Alternatif renkler
    if (alternateColors) {
        table->setAlternatingRowColors(true);
    }
    
    // Seçim modu
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Salt okunur yap
    makeReadOnly(table);
    
    // Modern stil uygula
    applyModernStyle(table);
}

void TableHelper::resizeColumnsToContent(QTableWidget* table)
{
    if (!table) return;
    
    table->resizeColumnsToContents();
    
    // Minimum genişlikleri ayarla
    for (int i = 0; i < table->columnCount(); ++i) {
        int currentWidth = table->columnWidth(i);
        int minWidth = 80; // Minimum genişlik
        
        // Başlık genişliğini kontrol et
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(i);
        if (headerItem) {
            QFontMetrics fm(table->font());
            int headerWidth = fm.horizontalAdvance(headerItem->text()) + 20;
            minWidth = qMax(minWidth, headerWidth);
        }
        
        // İçerik genişliğini kontrol et
        int maxContentWidth = minWidth;
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = table->item(row, i);
            if (item) {
                QFontMetrics fm(table->font());
                int itemWidth = fm.horizontalAdvance(item->text()) + 20;
                maxContentWidth = qMax(maxContentWidth, itemWidth);
            }
        }
        
        // Maksimum genişliği sınırla
        int maxWidth = 300;
        int finalWidth = qMin(qMax(currentWidth, maxContentWidth), maxWidth);
        
        table->setColumnWidth(i, finalWidth);
    }
    
    // Tablo genişliğini ayarla
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void TableHelper::makeReadOnly(QTableWidget* table)
{
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void TableHelper::clearTable(QTableWidget* table)
{
    table->clearContents();
    table->setRowCount(0);
}

void TableHelper::addRow(QTableWidget* table, const QStringList& data, const QVariant& userData)
{
    int row = table->rowCount();
    table->insertRow(row);
    
    for (int col = 0; col < data.count() && col < table->columnCount(); ++col) {
        QTableWidgetItem* item = new QTableWidgetItem(data[col]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, col, item);
    }
    
    // User data'yı ilk sütuna ekle
    if (userData.isValid() && table->columnCount() > 0) {
        QTableWidgetItem* firstItem = table->item(row, 0);
        if (firstItem) {
            firstItem->setData(Qt::UserRole, userData);
        }
    }
}

void TableHelper::addRow(QTableWidget* table, const QVariantList& data, const QVariant& userData)
{
    int row = table->rowCount();
    table->insertRow(row);
    
    for (int col = 0; col < data.count() && col < table->columnCount(); ++col) {
        QTableWidgetItem* item = new QTableWidgetItem(data[col].toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, col, item);
    }
    
    // User data'yı ilk sütuna ekle
    if (userData.isValid() && table->columnCount() > 0) {
        QTableWidgetItem* firstItem = table->item(row, 0);
        if (firstItem) {
            firstItem->setData(Qt::UserRole, userData);
        }
    }
}

void TableHelper::applyModernStyle(QTableWidget* table)
{
    QString styleSheet = R"(
        QTableWidget {
            background: white;
            alternate-background-color: #f8f9fa;
            border: 2px solid #bdc3c7;
            border-radius: 8px;
            gridline-color: #ecf0f1;
            selection-background-color: #3498db;
            selection-color: white;
            font-size: 10pt;
            font-family: 'Segoe UI', Arial, sans-serif;
        }

        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #ecf0f1;
        }

        QTableWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498db, stop:1 #2980b9);
            color: white;
        }

        QTableWidget::item:hover {
            background: #e8f4fd;
        }

        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #34495e, stop:1 #2c3e50);
            color: white;
            padding: 12px;
            border: none;
            font-weight: bold;
            font-size: 10pt;
        }

        QHeaderView::section:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #5dade2, stop:1 #3498db);
        }

        QScrollBar:vertical {
            background: #ecf0f1;
            width: 12px;
            border-radius: 6px;
        }

        QScrollBar::handle:vertical {
            background: #bdc3c7;
            border-radius: 6px;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background: #95a5a6;
        }

        QScrollBar:horizontal {
            background: #ecf0f1;
            height: 12px;
            border-radius: 6px;
        }

        QScrollBar::handle:horizontal {
            background: #bdc3c7;
            border-radius: 6px;
            min-width: 20px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #95a5a6;
        }
    )";
    
    table->setStyleSheet(styleSheet);
} 