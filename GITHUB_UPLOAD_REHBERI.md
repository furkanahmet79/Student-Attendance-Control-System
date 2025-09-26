# 🚀 GitHub'a Proje Yükleme Rehberi

## Adım 1: Git Reposu Hazırlığı

### 1.1 Git İlk Kurulum (Henüz yapmadıysan)
```bash
# Git config ayarları (tek seferli)
git config --global user.name "Furka" 
git config --global user.email "furka@example.com"
```

### 1.2 Mevcut Klasörü Git Reposu Yap
```bash
# Projenin bulunduğu klasöre git
cd C:\Users\furka\Documents\ogrencıkontrol

# Git reposu başlat
git init
git add .
git commit -m "İlk commit: Öğrenci yoklama kontrol sistemi eklendi"
```

## Adım 2: GitHub'da Depo Oluşturma

### 2.1 GitHub.com'da Yeni Repo Oluştur
1. **GitHub.com**'a git ve hesabın ile giriş yap
2. Sağ üst köşeden **"+"** → **"New repository"**
3. **Repository name**: `ogrenci-yoklama-kontrol`
4. **Description**: `Qt6 tabanlı RFID kart okuyucu ile öğrenci yoklama sistemi`
5. **Public/Private**: İsteğine göre seç
6. **⚠️ ÖNEMLİ**: "Initialize with README" işaretleme (README'n zaten var)
7. **"Create repository"** tıkla

## Adım 3: Projeyi GitHub'a Yükleme

### 3.1 Remote Bağlantı Kur
```bash
# GitHub repos arkafesını kopyla (örnek)
git remote add origin https://github.com/furka/ogrenci-yoklama-kontrol.git

# Remote kontrolü
git remote -v
```

### 3.2 Projeyi Push Et
```bash
# Ana branch'e push
git push -u origin main
```

## Adım 4: .gitignore Dosyası Oluştur

###  yourself_.gitignore dosyası
```gitignore
# Qt specific
*.pro.user
*.pro.user.*
.pro.user.*
*.qbs.user*
*.moc
moc_*.cpp
moc_*.h
qrc_*.cpp
ui_*.h
*.qmlc
*.jsc
Makefile*
*build*
*qrc_*
*.pro

# Build directories
build/
debug/
release/

# Database files (yoklama_sistemi.db hariç olsa da güvenlik için)
*.db
*.sqlite
*.sqlite3

# Qt Creator specific
*.autosave

# Compiled Object files
*.o
*.obj

# Executable files
*.exe

# Debug files
*.dSYM/
*.su
*.idb
*.pdb

# Qt Meta Object Compiler temporary files
moc_*.h
moc_*.cpp
qrc_*.cpp
ui_*.h

# macOS
.DS_Store
.AppleDouble
.LSOverride

# Windows
Thumbs.db
ehthumbs.db
Desktop.ini
```

## Adım 5: ✅ Doğrulama ve Son Kontroller

### 5.1 GitHub Repo Kontrolü
- [ ] README.md dosyan görünüyor mu?
- [ ] Kod dosyaları (`*.cpp`, `*.h`) yüklendi mi?
- [ ] Proje konumlandığında tüm dosyalar açık mı?

### 5.2 Proje Dosyası Kontrol Listesi
```
✅ main.cpp
✅ mainwindow.cpp / mainwindow.h
✅ teacherwidget.cpp / teacherwidget.h / teacherwidget.ui
✅ adminwidget.cpp / adminwidget.h
✅ studentwidget.cpp / studentwidget.h 
✅ loginwidget.cpp / loginwidget.h
✅ databasemanager.cpp / databasemanager.h
✅ tablehelper.cpp / tablehelper.h
✅ create_sqlite_db.cpp
✅ styles.qss
✅ CMakeLists.txt
✅ README.md (✅ Güncel)
```

## Adım 6: 🎯 İsteğe Bağlı: Repository Ayarları

### 6.1 README Görünümünü Test Et
1. GitHub reposunun **Code** sekmesine git
2. README.md'nin sidebar'da düzgün görünüp görünmediğini kontrol et
3. **About** bölümünde proje açıklamasını ekle (bahşiş ile)

### 6.2 Topics/Hashtag Eklemeri
1. Repository'nin ana sayfasında **⚙️ Settings** → **About**
2. **Topics** ekle:
   - qt
   - qtdisabled
   - cpp
   - sqlite
   - rfid
   - attendance
   - student-management

## 🚨 Sorun Yaşarsan

### Uygun Teknikler

**Problem: "Username for 'https://github.com':" istiyor**
```bash
# GitHub username/password yerine Personal Access Token kullan
git config --global credential.helper store
```

**Problem: Remote repository bulunamıyor**
```bash
# Remote URL'yi kontrol et
git remote remove origin
git remote add origin https://github.com/furka/ogrenci-yoklama-kontrol.git
```

**Problem: Push rejected**
```bash
# Önce pull yap, sonra push
git pull origin main --allow-unrelated-histories
git push -u origin main
```

## 🎉 Tebrik!

Artık projen GitHub'da mevcut ve:
- ✅ Teknik dökümanlar tam
- ✅ README profesyonel görünüm  
- ✅ Kod örnekleri yerli yerinde
- ✅ Troubleshooting bölümü mevcut

Repon linki: `https://github.com/furka/ogrenci-yoklama-kontrol`

Star vermeyi unutma! 🌟
