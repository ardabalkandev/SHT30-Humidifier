#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_SHT31.h>
#include <EEPROM.h>

// --- Pin Tanımlamaları ---
#define SET_BUTTON_PIN 2
#define MINUS_BUTTON_PIN 3
#define PLUS_BUTTON_PIN 4
#define MANUAL_BUTTON_PIN 5
#define BUZZER_PIN 6
#define RELAY_PIN 7
#define LED_PIN 8

// --- LCD Tanımlamaları ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C adresi ve boyutları (genellikle 0x27 veya 0x3F)

// --- Sensör Tanımlamaları ---
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// --- Durum Değişkenleri ---
float currentHumidity = 0.0;
float currentTemperature = 0.0;
int setHumidity = 50; // Varsayılan ayarlı nem değeri
bool isFahrenheit = false; // Varsayılan sıcaklık birimi Celsius
const int HYSTERESIS = 0.5; // Histerezis değeri
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 5000; // 2 saniye

// Röle ve LED Kontrolü
bool isRelayActive = false;
unsigned long relayActiveStartTime = 0;
const unsigned long NORMAL_RELAY_ON_DURATION = 5000; // 3 saniye
const unsigned long NORMAL_RELAY_OFF_DURATION = 10000; // 12 saniye

const unsigned long ALARM_LOW_HUMIDITY_ON_DURATION = 10000; // 5 saniye (Alarm durumunda düşük nem için)
const unsigned long ALARM_LOW_HUMIDITY_OFF_DURATION = 5000; // 5 saniye (Alarm durumunda düşük nem için)

bool isHumidityLowForRelay = false; // Röleyi tetikleyecek kadar düşük nem durumu

// Buton İşleme
unsigned long lastSetButtonPress = 0;
unsigned long lastButtonActivity = 0;
const unsigned long SETTINGS_TIMEOUT = 5000; // 5 saniye

// Buton basılı tutma için
unsigned long plusButtonDownTime = 0;   // PLUS butonuna basılma süresinin başlangıcı
unsigned long minusButtonDownTime = 0;  // MINUS butonuna basılma süresinin başlangıcı
unsigned long lastRepeatTime = 0;       // Son tekrarın gerçekleştiği zaman
const unsigned long INITIAL_REPEAT_DELAY = 800; // İlk hızlı artış/azalış başlamadan önceki gecikme (ms)
const unsigned long REPEAT_RATE = 100;        // Hızlı artış/azalışın tekrar oranı (ms)
bool plusButtonFirstPressHandled = false; // PLUS butonunun ilk basışını yakalamak için
bool minusButtonFirstPressHandled = false; // MINUS butonunun ilk basışını yakalamak için


enum SystemState {
  NORMAL_MODE,
  SET_HUMIDITY_MODE,
  SET_TEMPERATURE_UNIT_MODE,
  SET_LOW_ALARM_HUMIDITY_MODE,
  SET_HIGH_ALARM_HUMIDITY_MODE
};
SystemState currentState = NORMAL_MODE;

// Alarm Durumu
bool isAlarmActive = false;
bool isLowHumidityAlarm = false; // Nem düşüklüğünden mi, yüksekliğinden mi alarm var?
unsigned long lastAlarmToggleTime = 0;
const unsigned long ALARM_BUZZER_DURATION = 500; // 1 saniye
const unsigned long ALARM_PAUSE_DURATION = 500; // 500 ms
const int ALARM_FREQUENCY = 3000; // 2000 Hz

// Alarm Eşik Değerleri (yüzde değil, doğrudan nem değeri farkı olarak tutuluyor)
int lowAlarmThresholdOffset = 15;  // Varsayılan: ayarlı nemin 15 birim altı
int highAlarmThresholdOffset = 25; // Varsayılan: ayarlı nemin 25 birim üstü

// EEPROM Adresleri
#define EEPROM_HUMIDITY_ADDR 0
#define EEPROM_UNIT_ADDR 1
#define EEPROM_LOW_ALARM_OFFSET_ADDR 2
#define EEPROM_HIGH_ALARM_OFFSET_ADDR 3

// --- Fonksiyon Tanımlamaları ---
void readSensorData();
void displayNormalScreen();
void displaySettingHumidityScreen();
void displaySettingUnitScreen();
void displaySettingLowAlarmScreen();
void displaySettingHighAlarmScreen();
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void controlHumidifier();
void activateAlarm(bool isLow);
void deactivateAlarm();
void handleSetButton();
void adjustValue(int& value, int delta, int minVal, int maxVal, void (*displayFunc)()); // Yardımcı fonksiyon
void handleManualButton();
void runHumidifier(unsigned long onDuration, unsigned long offDuration);
void stopHumidifier();

void setup() {
  Serial.begin(9600);

  // Pinleri ayarla
  pinMode(SET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MINUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PLUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MANUAL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Röle ve LED başlangıçta kapalı
  digitalWrite(RELAY_PIN, HIGH); // Röle normalde HIGH'da pasifse LOW yap
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  // LCD başlat
  lcd.init();
  lcd.backlight();

  // Açılış ekranı
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SHT30 Hum v4.4");
  lcd.setCursor(0, 1);
  lcd.print("by Arda Balkan");
  delay(2000);
  lcd.clear();

  // Sensörü başlat
  if (!sht31.begin(0x44)) { // SHT30 I2C adresi (0x44 veya 0x45)
    Serial.println("SHT30 bulunamadi!");
    while (1) delay(10);
  }

  // EEPROM'dan ayarları yükle
  loadSettingsFromEEPROM();

  // İlk sensör okuması
  readSensorData();
  displayNormalScreen();
}

void loop() {
  unsigned long currentTime = millis();

  // Sensör okuma aralığı kontrolü
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    readSensorData();
    lastSensorReadTime = currentTime;
    if (currentState == NORMAL_MODE) {
      displayNormalScreen();
    }
  }

  // Buton okumaları ve işleme
  if (digitalRead(SET_BUTTON_PIN) == LOW) {
    delay(50); // Debounce
    if (digitalRead(SET_BUTTON_PIN) == LOW) {
      handleSetButton();
      lastButtonActivity = currentTime;
      // Ayar moduna girince basılı tutma sayaçlarını ve flag'leri sıfırla
      plusButtonDownTime = 0;
      minusButtonDownTime = 0;
      plusButtonFirstPressHandled = false;
      minusButtonFirstPressHandled = false;
      lastRepeatTime = 0;
      while (digitalRead(SET_BUTTON_PIN) == LOW); // Buton bırakılana kadar bekle
    }
  }

  // Ayar modlarında + ve - butonları için basılı tutma kontrolü
  if (currentState == SET_HUMIDITY_MODE ||
      currentState == SET_LOW_ALARM_HUMIDITY_MODE ||
      currentState == SET_HIGH_ALARM_HUMIDITY_MODE) {
    
    // PLUS Butonu kontrolü
    if (digitalRead(PLUS_BUTTON_PIN) == LOW) {
      if (plusButtonDownTime == 0) { // Butona ilk basıldığında zamanı kaydet
        plusButtonDownTime = currentTime;
        plusButtonFirstPressHandled = false; // Yeni basış olduğu için sıfırla
      }

      // İlk basışta (1'er artış)
      if (!plusButtonFirstPressHandled && (currentTime - plusButtonDownTime < INITIAL_REPEAT_DELAY)) {
        adjustValue(
          (currentState == SET_HUMIDITY_MODE ? setHumidity : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? lowAlarmThresholdOffset : highAlarmThresholdOffset)),
          1, // Tek basışta 1 artır
          5, 95,
          (currentState == SET_HUMIDITY_MODE ? displaySettingHumidityScreen : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? displaySettingLowAlarmScreen : displaySettingHighAlarmScreen))
        );
        plusButtonFirstPressHandled = true; // İlk basış işlendi
        lastRepeatTime = currentTime; // İlk 1'lik artışı da tekrar zamanı olarak say
      }
      // Hızlı ilerleme (5'er artış)
      else if ((currentTime - plusButtonDownTime >= INITIAL_REPEAT_DELAY) && (currentTime - lastRepeatTime >= REPEAT_RATE)) {
        adjustValue(
          (currentState == SET_HUMIDITY_MODE ? setHumidity : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? lowAlarmThresholdOffset : highAlarmThresholdOffset)),
          5, // Basılı tutulursa 5 artır
          5, 95,
          (currentState == SET_HUMIDITY_MODE ? displaySettingHumidityScreen : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? displaySettingLowAlarmScreen : displaySettingHighAlarmScreen))
        );
        lastRepeatTime = currentTime;
      }
      lastButtonActivity = currentTime; // Ayar modunda herhangi bir butona basınca timeout'u sıfırla

    } else { // PLUS butonu bırakıldığında
      plusButtonDownTime = 0; // Basılı tutma zamanını sıfırla
      plusButtonFirstPressHandled = false; // İlk basış flag'ini sıfırla
    }

    // MINUS Butonu kontrolü (mantık PLUS butonu ile aynı)
    if (digitalRead(MINUS_BUTTON_PIN) == LOW) {
      if (minusButtonDownTime == 0) {
        minusButtonDownTime = currentTime;
        minusButtonFirstPressHandled = false;
      }

      // İlk basışta (1'er azalış)
      if (!minusButtonFirstPressHandled && (currentTime - minusButtonDownTime < INITIAL_REPEAT_DELAY)) {
        adjustValue(
          (currentState == SET_HUMIDITY_MODE ? setHumidity : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? lowAlarmThresholdOffset : highAlarmThresholdOffset)),
          -1, // Tek basışta 1 azalt
          5, 95,
          (currentState == SET_HUMIDITY_MODE ? displaySettingHumidityScreen : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? displaySettingLowAlarmScreen : displaySettingHighAlarmScreen))
        );
        minusButtonFirstPressHandled = true;
        lastRepeatTime = currentTime;
      }
      // Hızlı ilerleme (5'er azalış)
      else if ((currentTime - minusButtonDownTime >= INITIAL_REPEAT_DELAY) && (currentTime - lastRepeatTime >= REPEAT_RATE)) {
        adjustValue(
          (currentState == SET_HUMIDITY_MODE ? setHumidity : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? lowAlarmThresholdOffset : highAlarmThresholdOffset)),
          -5, // Basılı tutulursa 5 azalt
          5, 95,
          (currentState == SET_HUMIDITY_MODE ? displaySettingHumidityScreen : 
           (currentState == SET_LOW_ALARM_HUMIDITY_MODE ? displaySettingLowAlarmScreen : displaySettingHighAlarmScreen))
        );
        lastRepeatTime = currentTime;
      }
      lastButtonActivity = currentTime; // Ayar modunda herhangi bir butona basınca timeout'u sıfırla

    } else { // MINUS butonu bırakıldığında
      minusButtonDownTime = 0;
      minusButtonFirstPressHandled = false;
    }
  } 
  // --- Sıcaklık Birimi Ayar Modu için Özel Kontrol ---
  else if (currentState == SET_TEMPERATURE_UNIT_MODE) {
    if (digitalRead(PLUS_BUTTON_PIN) == LOW) {
      delay(50); // Debounce
      if (digitalRead(PLUS_BUTTON_PIN) == LOW) {
        isFahrenheit = !isFahrenheit; // Birimi değiştir
        displaySettingUnitScreen();   // Ekranı güncelle
        lastButtonActivity = currentTime;
        while(digitalRead(PLUS_BUTTON_PIN) == LOW); // Buton bırakılana kadar bekle
      }
    }
    if (digitalRead(MINUS_BUTTON_PIN) == LOW) {
      delay(50); // Debounce
      if (digitalRead(MINUS_BUTTON_PIN) == LOW) {
        isFahrenheit = !isFahrenheit; // Birimi değiştir
        displaySettingUnitScreen();   // Ekranı güncelle
        lastButtonActivity = currentTime;
        while(digitalRead(MINUS_BUTTON_PIN) == LOW); // Buton bırakılana kadar bekle
      }
    }
  }
  // --- Manuel Buton Kontrolü ---
  if (digitalRead(MANUAL_BUTTON_PIN) == LOW) {
    delay(50); // Debounce
    if (digitalRead(MANUAL_BUTTON_PIN) == LOW) {
      handleManualButton();
      while (digitalRead(MANUAL_BUTTON_PIN) == LOW);
    }
  }

  // Ayar modunda timeout kontrolü
  if (currentState != NORMAL_MODE && (currentTime - lastButtonActivity >= SETTINGS_TIMEOUT)) {
    saveSettingsToEEPROM();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kaydedildi!");
    delay(1500);
    currentState = NORMAL_MODE;
    displayNormalScreen();
  }

  // Alarm ve nemlendirme kontrolü
  int calculatedLowerAlarmThreshold = setHumidity - lowAlarmThresholdOffset;
  int calculatedUpperAlarmThreshold = setHumidity + highAlarmThresholdOffset;

  if (currentHumidity < calculatedLowerAlarmThreshold) {
    if (!isAlarmActive || !isLowHumidityAlarm) {
      activateAlarm(true);
    }
  } else if (currentHumidity > calculatedUpperAlarmThreshold) {
    if (!isAlarmActive || isLowHumidityAlarm) {
      activateAlarm(false);
    }
  } else {
    if (isAlarmActive) {
      deactivateAlarm();
    }
  }

  // Alarm aktifse, alarm sinyalini yönet
  if (isAlarmActive) {
    unsigned long currentAlarmTime = millis();
    if (currentAlarmTime - lastAlarmToggleTime >= (ALARM_BUZZER_DURATION + ALARM_PAUSE_DURATION)) {
      lastAlarmToggleTime = currentAlarmTime;
    }
    
    if (currentAlarmTime - lastAlarmToggleTime < ALARM_BUZZER_DURATION) {
      tone(BUZZER_PIN, ALARM_FREQUENCY);
      digitalWrite(LED_PIN, HIGH);
    } else {
      noTone(BUZZER_PIN);
      digitalWrite(LED_PIN, LOW);
    }
  }

  // Nemlendirme kontrolü
  controlHumidifier();
}

// --- Fonksiyon Uygulamaları ---

void readSensorData() {
  currentHumidity = sht31.readHumidity();
  currentTemperature = sht31.readTemperature();

  if (isFahrenheit) {
    currentTemperature = (currentTemperature * 9 / 5) + 32;
  }

  Serial.print("Nem: ");
  Serial.print(currentHumidity);
  Serial.print(" %\tSicaklik: ");
  Serial.print(currentTemperature);
  Serial.println(isFahrenheit ? " F" : " C");
}

void displayNormalScreen() {
  int displayLowerAlarmThreshold = setHumidity - lowAlarmThresholdOffset;
  int displayUpperAlarmThreshold = setHumidity + highAlarmThresholdOffset;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(currentHumidity, 1);
  lcd.print("%  ");
  lcd.setCursor(8, 0); // Sıcaklık için yeni konum
  lcd.print("T:");
  lcd.print(currentTemperature, 1);
  lcd.print(isFahrenheit ? "F" : "C");

  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(setHumidity);
  lcd.print("  A:-");
  lcd.print(lowAlarmThresholdOffset); // Burası düşük alarm ofseti olmalı
  lcd.print("/+");
  lcd.print(highAlarmThresholdOffset); // Burası yüksek alarm ofseti olmalı
}

void displaySettingHumidityScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Istenen Nem:");
  lcd.setCursor(0, 1);
  lcd.print(setHumidity);
  lcd.print("%");
}

void displaySettingUnitScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sicaklik Birimi:");
  lcd.setCursor(0, 1);
  lcd.print(isFahrenheit ? "Fahrenheit" : "Celsius");
}

void displaySettingLowAlarmScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nem Dusuk Alm:");
  lcd.setCursor(0, 1);
  lcd.print(lowAlarmThresholdOffset);
  lcd.print(" Birim Alt");
}

void displaySettingHighAlarmScreen() {
  lcd.clear(); // Hata: 'clear' yerine 'lcd.clear()' olmalı
  lcd.setCursor(0, 0);
  lcd.print("Nem Yuksek Alm:");
  lcd.setCursor(0, 1);
  lcd.print(highAlarmThresholdOffset);
  lcd.print(" Birim Ust");
}

void saveSettingsToEEPROM() {
  EEPROM.update(EEPROM_HUMIDITY_ADDR, setHumidity);
  EEPROM.update(EEPROM_UNIT_ADDR, (byte)isFahrenheit);
  EEPROM.update(EEPROM_LOW_ALARM_OFFSET_ADDR, (byte)lowAlarmThresholdOffset);
  EEPROM.update(EEPROM_HIGH_ALARM_OFFSET_ADDR, (byte)highAlarmThresholdOffset);
  Serial.println("Ayarlar EEPROM'a kaydedildi.");
}

void loadSettingsFromEEPROM() {
  setHumidity = EEPROM.read(EEPROM_HUMIDITY_ADDR);
  if (setHumidity < 5 || setHumidity > 95 || setHumidity == 255) {
    setHumidity = 50; // Varsayılan
  }
  isFahrenheit = (bool)EEPROM.read(EEPROM_UNIT_ADDR);
  
  lowAlarmThresholdOffset = EEPROM.read(EEPROM_LOW_ALARM_OFFSET_ADDR);
  if (lowAlarmThresholdOffset < 5 || lowAlarmThresholdOffset > 95 || lowAlarmThresholdOffset == 255) {
    lowAlarmThresholdOffset = 15; // Varsayılan değer
  }
  
  highAlarmThresholdOffset = EEPROM.read(EEPROM_HIGH_ALARM_OFFSET_ADDR);
  if (highAlarmThresholdOffset < 5 || highAlarmThresholdOffset > 95 || highAlarmThresholdOffset == 255) {
    highAlarmThresholdOffset = 25; // Varsayılan değer
  }

  Serial.println("Ayarlar EEPROM'dan yuklendi.");
}

void controlHumidifier() {
  if (isAlarmActive) {
    if (isLowHumidityAlarm) {
      runHumidifier(ALARM_LOW_HUMIDITY_ON_DURATION, ALARM_LOW_HUMIDITY_OFF_DURATION);
    } else {
      stopHumidifier();
    }
  } else {
    if (currentHumidity >= setHumidity) {
      stopHumidifier();
      isHumidityLowForRelay = false;
    } else if (currentHumidity <= (setHumidity - HYSTERESIS)) {
      isHumidityLowForRelay = true;
    }

    if (isHumidityLowForRelay) {
      runHumidifier(NORMAL_RELAY_ON_DURATION, NORMAL_RELAY_OFF_DURATION);
    } else {
      stopHumidifier();
    }
  }
}

void runHumidifier(unsigned long onDuration, unsigned long offDuration) {
  unsigned long currentTime = millis();

  if (!isRelayActive && (currentTime - relayActiveStartTime >= offDuration || relayActiveStartTime == 0)) {
    digitalWrite(RELAY_PIN, LOW);
    if (!isAlarmActive) digitalWrite(LED_PIN, HIGH);
    isRelayActive = true;
    relayActiveStartTime = currentTime;
    Serial.print("Nemlendirme Baslatildi (");
    Serial.print(onDuration / 1000);
    Serial.println("s)");
  } else if (isRelayActive && (currentTime - relayActiveStartTime >= onDuration)) {
    digitalWrite(RELAY_PIN, HIGH);
    if (!isAlarmActive) digitalWrite(LED_PIN, LOW);
    isRelayActive = false;
    Serial.print("Nemlendirme Durduruldu (");
    Serial.print(offDuration / 1000);
    Serial.println("s Dinlenme)");
  }
}

void stopHumidifier() {
  if (isRelayActive) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    isRelayActive = false;
    Serial.println("Nemlendirme Tamamen Durduruldu.");
  }
}

void activateAlarm(bool isLow) {
  if (!isAlarmActive) {
    lastAlarmToggleTime = millis();
    stopHumidifier();
  }
  isAlarmActive = true;
  isLowHumidityAlarm = isLow;
  Serial.print("Alarm Aktif! (Nem: ");
  Serial.print(isLow ? "Dusuk" : "Yuksek");
  Serial.println(")");
}

void deactivateAlarm() {
  if (isAlarmActive) {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    isAlarmActive = false;
    Serial.println("Alarm Deaktif.");
  }
}

void handleSetButton() {
  if (currentState == NORMAL_MODE) {
    currentState = SET_HUMIDITY_MODE;
    displaySettingHumidityScreen();
  } else if (currentState == SET_HUMIDITY_MODE) {
    currentState = SET_TEMPERATURE_UNIT_MODE;
    displaySettingUnitScreen();
  } else if (currentState == SET_TEMPERATURE_UNIT_MODE) {
    currentState = SET_LOW_ALARM_HUMIDITY_MODE;
    displaySettingLowAlarmScreen();
  } else if (currentState == SET_LOW_ALARM_HUMIDITY_MODE) {
    currentState = SET_HIGH_ALARM_HUMIDITY_MODE;
    displaySettingHighAlarmScreen();
  } else if (currentState == SET_HIGH_ALARM_HUMIDITY_MODE) {
    saveSettingsToEEPROM();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kaydedildi!");
    delay(1500);
    currentState = NORMAL_MODE;
    displayNormalScreen();
  }
}

// Değer ayarlama mantığını merkezileştiren yardımcı fonksiyon
void adjustValue(int& value, int delta, int minVal, int maxVal, void (*displayFunc)()) {
  value += delta;
  // Değerleri sınırlar içinde tut
  if (value < minVal) value = minVal;
  if (value > maxVal) value = maxVal;
  
  displayFunc(); // Ekrana yeni değeri yansıt
}


void handleManualButton() {
  if (currentState == NORMAL_MODE) {
    Serial.println("Manuel Nemlendirme Baslatildi.");
    digitalWrite(RELAY_PIN, LOW);  // Röleyi çek
    digitalWrite(LED_PIN, HIGH);   // LED'i yak
    delay(3000);                   // 3 saniye bekle
    digitalWrite(RELAY_PIN, HIGH); // Röleyi bırak
    digitalWrite(LED_PIN, LOW);    // LED'i söndür
    Serial.println("Manuel Nemlendirme Tamamlandi.");
  }
}