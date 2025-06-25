# Akıllı Nem Kontrol Sistemi (SHT31 & Arduino)

Bu proje, bir Arduino kartı kullanarak ortam nemini ve sıcaklığını izleyen, belirli nem seviyelerini otomatik olarak koruyan ve ayarlanan eşiklerin dışına çıkıldığında görsel ve işitsel uyarılar veren bir akıllı nem kontrol sistemidir. Sistem, bir nemlendiriciyi (veya nem alıcıyı) kontrol etmek için bir röle kullanır ve kullanıcı dostu bir arayüze sahiptir.

## Özellikler

* **Gerçek Zamanlı Nem ve Sıcaklık İzleme:** SHT31 sensörü ile anlık nem ve sıcaklık değerlerini okur.
* **Ayarlanabilir Hedef Nem:** Kullanıcı tarafından belirlenebilir hedef nem seviyesi (EEPROM'a kaydedilir).
* **Otomatik Nemlendirme/Nem Alma Kontrolü:** Hedef nem seviyesine göre bağlı nemlendiriciyi (veya nem alıcıyı) röle aracılığıyla otomatik olarak açar/kapatır.
    * Nem düşüklüğünde: Nemlendiriciyi belirli aralıklarla çalıştırır.
    * Nem yüksekliğinde: Nemlendiriciyi kapatır (veya nem alıcıyı çalıştırır).
* **Ayarlanabilir Alarm Eşikleri:** Hedef nemin altına veya üstüne düşen/çıkan durumlar için ayrı ayrı ayarlanabilir düşük ve yüksek nem alarmı ofsetleri (EEPROM'a kaydedilir).
* **Görsel ve İşitsel Alarm:** Ayarlanan alarm eşikleri aşıldığında LED ve buzzer ile uyarı verir.
* **Sıcaklık Birimi Seçimi:** Celsius (°C) veya Fahrenheit (°F) arasında geçiş yapabilme (EEPROM'a kaydedilir).
* **Manuel Nemlendirme:** Tek bir tuşla manuel olarak nemlendirme yapabilme.
* **Kullanıcı Arayüzü:** 16x2 I2C LCD ekran ve 4 adet buton ile kolayca ayar yapabilme ve durumu takip edebilme.
* **Ayarları Kaydetme:** Tüm ayarlar (hedef nem, sıcaklık birimi, alarm ofsetleri) Arduino'nun dahili EEPROM'una kaydedilir, böylece güç kesintisinde kaybolmazlar.
* **Basılı Tutma Fonksiyonu:** Ayar modlarında '+' ve '-' butonlarına basılı tutularak değerleri hızlıca artırma/azaltma.

## Kullanılan Donanımlar

* **Arduino Uno/Nano** (veya uyumlu bir Arduino kartı)
* **SHT31 Dijital Nem ve Sıcaklık Sensörü** (I2C uyumlu)
* **16x2 I2C LCD Ekran Modülü** (I2C uyumlu)
* **1 Kanal Röle Modülü**
* **Buzzer** (Pasif veya Aktif)
* **LED** (Durum göstergesi veya alarm için, 220 Ohm direnç ile)
* **4 Adet Push Buton** (`SET`, `MINUS`, `PLUS`, `MANUAL`)
* **Jumper Kablolar**
* **Breadboard veya Özel PCB** (Prototipleme veya kalıcı kurulum için)
* **220 Ohm Direnç** (LED için)
* **USB Kablosu** (Arduino'yu programlamak ve beslemek için)
* **5V Güç Kaynağı** (Sistem için)
* **Nemlendirici / Nem Alıcı** (Röle ile kontrol edilecek cihaz)

## Bağlantı Şeması

Aşağıdaki bağlantılar Arduino Uno/Nano pinlerine göre verilmiştir. Diğer Arduino modellerinde I2C pinleri (SDA/SCL) farklılık gösterebilir.

| Bileşen            | Pin                     | Arduino Pini         | Açıklama                                       |
| :----------------- | :---------------------- | :------------------- | :--------------------------------------------- |
| **Butonlar** |                         |                      | (Her butonun diğer ucu GND'ye bağlı)          |
| SET Butonu         |                         | Dijital 2 (INPUT_PULLUP) | Ayar modları arasında geçiş                   |
| MINUS Butonu       |                         | Dijital 3 (INPUT_PULLUP) | Değer azaltma / Birim değiştirme             |
| PLUS Butonu        |                         | Dijital 4 (INPUT_PULLUP) | Değer artırma / Birim değiştirme              |
| MANUAL Butonu      |                         | Dijital 5 (INPUT_PULLUP) | Manuel nemlendirmeyi tetikler                   |
| **Buzzer** | (+)                     | Dijital 6            | Sesli alarm                                    |
| **Röle** | IN (Trigger)            | Dijital 7            | Nemlendirici/Nem alıcı kontrolü                |
| **LED** | (+) (220 Ohm direnç ile)| Dijital 8            | Görsel durum/alarm göstergesi                  |
| **SHT31 Sensör** | SDA                     | A4 (veya özel SDA)   | I2C Veri Hattı                                 |
|                    | SCL                     | A5 (veya özel SCL)   | I2C Saat Hattı                                 |
|                    | VCC                     | 5V                   | Sensör beslemesi                               |
|                    | GND                     | GND                  | Sensör toprak                                  |
| **16x2 I2C LCD** | SDA                     | A4 (veya özel SDA)   | I2C Veri Hattı (SHT31 ile paralel)             |
|                    | SCL                     | A5 (veya özel SCL)   | I2C Saat Hattı (SHT31 ile paralel)             |
|                    | VCC                     | 5V                   | LCD beslemesi                                  |
|                    | GND                     | GND                  | LCD toprak                                     |

**Not:** Röle modülünün kendi güç bağlantıları (+ ve -) olabilir, bunları Arduino'dan veya harici 5V kaynağından beslemelisiniz. LED'i bağlarken 220 Ohm'luk seri direnç kullanmayı unutmayın.

## Kurulum Talimatları

1.  **Gerekli Kütüphaneleri Yükleyin:**
    * Arduino IDE'yi açın.
    * `Sketch > Include Library > Manage Libraries...` yolunu izleyin.
    * Aşağıdaki kütüphaneleri aratın ve yükleyin:
        * `LiquidCrystal I2C` (by Frank de Brabander)
        * `Adafruit SHT31 Library` (by Adafruit)
2.  **Devreyi Kurun:** Yukarıdaki bağlantı şemasına göre tüm donanım bileşenlerini Arduino'ya bağlayın.
3.  **Kodu Yükleyin:**
    * Bu proje dosyasını (örneğin `AkilliNemKontrol.ino`) Arduino IDE'de açın.
    * `Tools > Board` menüsünden kullandığınız Arduino kartını seçin (örn: "Arduino Uno").
    * `Tools > Port` menüsünden Arduino'nuzun bağlı olduğu seri portu seçin.
    * Kodu Arduino kartınıza yüklemek için "Upload" (Yükle) butonuna tıklayın.
4.  **Test Edin ve Kalibre Edin:**
    * Sistem başlatıldığında açılış mesajını göreceksiniz.
    * `SET` butonu ile ayar modları arasında geçiş yapın.
    * `PLUS` ve `MINUS` butonları ile değerleri ayarlayın. Basılı tutma özelliği sayesinde hızlıca ayarlama yapabilirsiniz.
    * Ayarlarınızı kaydetmek için `SET` butonuna tüm modlardan geçtikten sonra tekrar basın veya 5 saniye boyunca hiçbir butona dokunmayın (timeout).
    * Alarm ve nemlendirme fonksiyonlarını test etmek için nem seviyesini değiştirin.
    * `MANUAL` butonu ile manuel nemlendirmeyi test edin.

## Ayar Modları

`SET` butonuna her basıldığında, sistem aşağıdaki ayar modları arasında döngü yapar:

1.  **İstenen Nem Ayarı:** Ortamda tutulması istenen hedef nem seviyesi ayarlanır (örn: %50).
    * `PLUS` / `MINUS` ile değeri 5 ila 95 arasında değiştirin. (Tek basışta 1, basılı tutmada 5 birim değişir.)
2.  **Sıcaklık Birimi Ayarı:** Sıcaklık gösterim birimi Celsius (°C) veya Fahrenheit (°F) olarak ayarlanır.
    * `PLUS` / `MINUS` ile birimi değiştirin.
3.  **Nem Düşük Alarm Eşiği Ayarı:** Hedef nemin kaç birim altına düşüldüğünde alarmın tetikleneceği ayarlanır (örn: Hedef %50 ise, "15 Birim Alt" ayarı %35'te alarm verir).
    * `PLUS` / `MINUS` ile ofset değerini 5 ila 95 arasında değiştirin.
4.  **Nem Yüksek Alarm Eşiği Ayarı:** Hedef nemin kaç birim üstüne çıkıldığında alarmın tetikleneceği ayarlanır (örn: Hedef %50 ise, "25 Birim Üst" ayarı %75'te alarm verir).
    * `PLUS` / `MINUS` ile ofset değerini 5 ila 95 arasında değiştirin.
5.  **Kaydetme ve Çıkış:** Tüm ayarları EEPROM'a kaydeder ve normal moda döner. Ayrıca herhangi bir ayar modunda 5 saniye butona basılmazsa otomatik olarak kaydedip normal moda döner.

## Röle Kontrol Mantığı

Sistem, belirlenen hedef nemin altına düştüğünde (Histerezis değeri dikkate alınarak) nemlendiriciyi kontrol eder. Alarm aktif değilse, nemlendirici **3 saniye açılır, 12 saniye kapalı kalır.** Bu döngü, nem seviyesi yükselene kadar devam eder.

**Alarm Durumunda Röle Kontrolü:**

* **Düşük Nem Alarmı:** Nem çok düşük eşiğin altına düştüğünde, nemlendirici daha uzun süre, **5 saniye açılır ve 5 saniye kapalı kalır.**
* **Yüksek Nem Alarmı:** Nem çok yüksek eşiğin üzerine çıktığında, nemlendirici tamamen kapatılır.

## Katkıda Bulunma

Bu proje açık kaynaklıdır ve geliştirmeye açıktır. Her türlü geri bildirim, hata düzeltmesi veya yeni özellik önerisi memnuniyetle karşılanır. Katkıda bulunmak isterseniz lütfen bir "issue" açın veya "pull request" gönderin.

## Lisans

Bu proje MIT Lisansı altında lisanslanmıştır. Daha fazla bilgi için `LICENSE` dosyasına bakın.

---
