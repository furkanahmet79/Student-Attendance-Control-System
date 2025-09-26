# 📚 Öğrenci Yoklama Kontrol Sistemi

Qt6 tabanlı geliştirilmiş, RFID/NFC kart okuyucu ile entegre olarak çalışan kapsamlı öğrenci yoklama sistemidir. Sistem, öğretmenler tarafından kart ile yoklama alan öğrencilerin kayıtlarını tutar ve yönetir.

![Qt6](https://img.shields.io/badge/Qt-6.9.1-green)
![CMake](https://img.shields.io/badge/CMake-3.16+-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-red)
![SQLite](https://img.shields.io/badge/SQLite-3-lightgrey)

## 🎯 Proje Özeti

Bu proje, eğitim kurumlarında öğrenci devamı kontrolü için tasarlanmış gelişmiş bir yoklama sistemidir. RFID/NFC kart okuyucu donanımı ile entegre çalışarak, öğrencilerin fiziksel kartları okutarak yoklama almalarını sağlar.

### 🔑 Temel Özellikler

- **📱 Rol Tabanlı Erişim**: Admin, Öğretmen ve Öğrenci rolleri
- **📋 RFID Kart Entegrasyonu**: Harmonik okuyucu ile doğrudan veri okuma
- **⏰ Gerçek Zamanlı Yoklama**: Canlı yoklama kaydı ve görsel geri bildirim
- **📊 Detaylı Raporlama**: Ders bazında yoklama geçmişi ve analiz
- **🎵 Ses Bildirimleri**: Başarılı kart okumalarında ses geri bildirimi
- **🗃️ Merkezi Veritabanı**: SQLite ile güvenli veri saklama

## 🚀 Hızlı Başlangıç

### Sistem Gereksinimleri

- **İşletim Sistemi**: Windows 10/11
- **Qt Framework**: 6.9.1 veya üzeri
- **CMake**: 3.16 veya üzeri  
- **C++17** destekli derleyici (MinGW, MSVC)
- **RFID Okuyucu**: COM10 port üzerinden bağlanan, STX/ETX protokolü kullanan kart okuyucu

### Kurulum

1. **Depoyu klonlayın**
   ```bash
   git clone https://github.com/yourusername/ogrenci-yoklama-kontrol.git
   cd ogrenci-yoklama-kontrol
   ```

2. **Qt Creator'da açın**
   - Qt Creator'ı açın
   - `CMakeLists.txt` dosyasını Qt Creator'da açın
   - Projeyi derleyin

3. **Kart Okuyucu Ayarları**
   - RFID okuyucuyu COM10 portuna bağlayın
   - Kart okuyucunun protokol ayarlarını kontrol edin (STX/ETX, 115200 baud)

### İlk Çalıştırma

- Program ilk açılışta `yoklama_sistemi.db` veritabanını otomatik oluşturur
- Varsayılan admin kullanıcısı: `admin` / `admin123`
- Varsayılan öğretmen kullanıcıları: `o1`/`o2` şifre: `123`

## 🏗️ Sistem Mimarisi

### Veritabanı Yapısı

```sql
-- Temel tablolar
users              # Kullanıcılar (Admin/Öğretmen)
students           # Öğrenci bilgileri
courses            # Ders tanımları  
enrollments        # Ders kayıtları
attendance_sessions # Yoklama oturumları
attendance_records  # Yoklama kayıtları
```

### Modül Yapısı

- **🎓 TeacherWidget**: Yoklama yönetimi, kart okuma, öğrenci yönetimi
- **👨‍💼 AdminWidget**: Sistem yönetimi, raporlama, kullanıcı işlemleri  
- **👨‍🎓 StudentWidget**: Öğrenci görünümü ve yoklama geçmişi
- **🔐 LoginWidget**: Güvenli kullanıcı giriş sistemi

## 🔧 Teacher Widget Özellikleri

Teacher Widget sistem kalbidir ve şu ana işlevleri sunar:

### 📋 Yoklama İşlemleri
```cpp
// Yoklama başlatma
void onStartAttendanceClicked() // UI'dan yoklama başlatma
void onEndAttendanceClicked()   // Yoklama sonlandırma

// Aktif yoklama durumu
void checkActiveAttendance()     // Sistem açılışında aktif yoklama kontrolü
void updateAttendanceList()     // Gerçek zamanlı liste güncelleme
```

### 👨‍🎓 Öğrenci Yönetimi
```cpp
// Öğrenci ekleme ve kayıt işlemleri
void onAddStudentClicked()      // Yeni öğrenci ekleme
void showAddStudentDialog()     // Öğrenci ekleme UI
void loadEnrolledStudents()     // Derse kayıtlı öğrenci listesi
```

### 🔍 Kart Okuma Entegrasyonu
- **Kart Tarama**: Fiziksel kart okutma desteği
- **Çift Kayıt Koruması**: Sistemdeki öğrencileri tekrar ekleme koruması
- **Hızlı Kayıt**: Okutulan kartı yeni öğrenci olarak ekleme

### 📊 Geçmiş ve Raporlama
```cpp
void loadAttendanceHistory()    // Ders geçmişi yükleme
void onHistoryTableDoubleClicked() // Detaylı görüntüleme
void requestDeleteAttendance()  // Yoklama silme talebi
```

## 🔌 Kart Okuyucu Entegrasyonu

### Teknik Özellikler
- **Port**: COM10 (Windows seri port)
- **Baud Rate**: 115200
- **Protokol**: STX/ETX mesajlaşma
- **Polling**: 2 saniye aralıklarla kart taraması

### Kart Okuma Akışı
1. **Polling Başlatma**: Serial port'tan kart sorgulama verisi gönder
2. **Kart Tespiti**: STX/ETX arası paket analizi
3. **UID Çıkarma**: DF0D etiketinden kart UID'si çıkarımı
4. **Veritabanı Eşleme**: Kart UID ile öğrenci eşleştirmesi
5. **Yoklama Kaydı**: Oturuma öğrenci ekleme/çıkarma

## 📱 Kullanıcı Arayüzü

### Modern Tasarım
- **Qt6 Widget Arayüzü**: Modern, responsive tasarım
- **Renkli Durum Göstergeleri**: Gerçek zamanlı durum uyarıları  
- **Tabbed Navigation**: Geçmiş, şimdi, öğrenciler sekmeleri
- **Responsive Tables**: Dinamik sütun genişlikleri

### Ses ve Görsel Bildirimler
```cpp
void showWelcomeNotification() // Öğrenci başarılı giriş bildirimi
m_successSound->play()         // Ses çalma
notificationLabel->show()      // Kısa süre görüntüleme
```

## 📈 Temel Kullanım Adımları

### 👨‍🏫 Öğretmen Kullanımı
1. **Sisteme giriş** yapın (o1/123 veya o2/123)
2. **Ders seçin** - Ders listesinden yoklama yapılacak dersi seçin  
3. **Yoklama başlatın** - Başlık girerek yoklama oturumunu başlatın
4. **Kart okutun** - Öğrenciler kartlarını okuyucuya okuttuklarında otomatik kayıt
5. **Yoklama sonlandırın** - Yoklama işlemini bitirin

### 👨‍💼 Admin Kullanımı  
1. **Admin paneline** erişin (admin/admin123)
2. **Öğretmen ekleyin** - Yeni öğretmen hesabı oluşturun
3. **Ders oluşturun** - Sisteme yeni ders ekleyin  
4. **Raporları izleyin** - Yoklama istatistiklerini görüntüleyin

### 👨‍🎓 Öğrenci Kullanımı
1. **Öğrenci numarası ile** giriş yapın
2. **Yoklama geçmişini** görüntüleyın
3. **Ders bazında** devam durumunuzu kontrol edin

### Kart Okuyucu Bağımlılık Ayarları

```cpp
// mainwindow.cpp - 238.satır
m_serialPort->setPortName("COM10"); 
m_serialPort->setBaudRate(QSerialPort::Baud115200);
```

### Önemli Qt6 Bağımlılıkları
- `QSerialPort` - Kart okuyucu haberleşmesi  
- `QSqlDatabase` - SQLite bağlantısı
- `QMediaPlayer` - Ses bildirimleri
- `QTimer` - Kart polling sistemi

## 🐛 Troubleshooting

### Kart Okuyucu Problemi
- **COM Port Bulunamadı**: Kart okuyucu sürücülerinin yüklü olduğunu kontrol edin
- **Veri Alınamıyor**: Port ayarlarını (COM10, 115200 baud) kontrol edin
- **STX/ETX Hatası**: Okuyucunun protokol ayarlarını kontrol edin

### Veritabanı Sorunları  
- **Veritabanı açılamıyor**: Program çalışma klasöründe veritabanı yazma yetkisini kontrol edin
- **Tablo bulunamadı**: `createDatabase()` fonksiyonunu çalıştırın

### Ses Sorunuyu
- **WAV dosyası bulunamadı**: `success.wav` dosyasını executable dosyası yanına kopyalayın
- **Ses çalmıyor**: Qt Multimedia modülünün çalıştığını kontrol edin

### ⚠️ Bilinen Hatalar

**Yoklama Alırken Program Kapanması:**
- Yoklama alma işlemi sırasında program aniden kapanabilir
- Debug modunu aktifleştirerek hata detaylarını görebilirsiniz
- Kart okuyucu bağlantı problemi veya veritabanı işlem hatası olabilir

---

⭐ **Bu projeyi beğendiniz mi? Star vermeyi unutmayın!**

