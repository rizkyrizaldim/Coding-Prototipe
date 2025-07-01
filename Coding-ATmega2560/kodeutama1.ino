#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

Servo servoSeal1, servoSeal12, servoSeal13;
#define SERVO_A_PIN 22
#define SERVO_B_PIN 24
#define SERVO_C_PIN 26

#define IR_SENSOR_PIN 5

RTC_DS3231 rtc;
DateTime now;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {28, 30, 32, 34};
byte colPins[COLS] = {36, 38, 40, 42};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int jumlahObat[3] = {9, 8, 8};
int jadwalObat[3][3][3];  // [kompartemen][jadwal ke-n][0=jam,1=menit,2=detik]
int stokObat[3] = {9, 8, 8};

bool settingMode = false;
int settingObat = 0, jumlahSet = 0, waktuIndex = 0, inputBuffer = 0;
int lastSecondProcessed[3][3];  // menyimpan detik terakhir diproses agar tidak dobel

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("RTC ERROR");
    while (1);
  }

  servoSeal1.attach(SERVO_A_PIN);
  servoSeal12.attach(SERVO_B_PIN);
  servoSeal13.attach(SERVO_C_PIN);
  servoSeal1.write(90); servoSeal12.write(90); servoSeal13.write(90);

  pinMode(IR_SENSOR_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Sistem Obat");
  delay(1500); lcd.clear();

  // Inisialisasi array detik agar tidak triggering dua kali
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      lastSecondProcessed[i][j] = -1;
}

void loop() {
  now = rtc.now();

  if (!settingMode) {
    lcd.setCursor(0, 0);
    lcd.print("Jam: ");
    lcd.print(now.hour() < 10 ? "0" : ""); lcd.print(now.hour()); lcd.print(":");
    lcd.print(now.minute() < 10 ? "0" : ""); lcd.print(now.minute()); lcd.print(":");
    lcd.print(now.second() < 10 ? "0" : ""); lcd.print(now.second());
    lcd.setCursor(0, 1); lcd.print("Tekan A Set Jadwal ");
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < jumlahObat[i]; j++) {
      if (now.hour() == jadwalObat[i][j][0] &&
          now.minute() == jadwalObat[i][j][1] &&
          now.second() == jadwalObat[i][j][2]) {
        if (lastSecondProcessed[i][j] != now.second()) {
          lastSecondProcessed[i][j] = now.second();
          keluarkanObat(i);
        }
      }
    }
  }

  handleKeypad(keypad.getKey());
  handleESPCommand();
  delay(200);
}

void handleKeypad(char key) {
  if (!key) return;
  if (settingMode) {
    if (key >= '0' && key <= '9') {
      inputBuffer = inputBuffer * 10 + (key - '0');
      lcd.setCursor(0, 1); lcd.print("Input: "); lcd.print(inputBuffer); lcd.print("   ");
    } else if (key == 'D') {
      if (jumlahSet == 0) {
        jumlahObat[settingObat] = inputBuffer;
        if (jumlahObat[settingObat] > 3) jumlahObat[settingObat] = 3;
        inputBuffer = 0; jumlahSet = 1; waktuIndex = 0;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Set Waktu 1 Jam:");
      } else {
        jadwalObat[settingObat][waktuIndex][jumlahSet - 1] = inputBuffer;
        inputBuffer = 0; jumlahSet++;
        if (jumlahSet > 3) {
          waktuIndex++;
          if (waktuIndex < jumlahObat[settingObat]) {
            jumlahSet = 1;
            lcd.clear(); lcd.setCursor(0, 0);
            lcd.print("Set Waktu "); lcd.print(waktuIndex + 1);
          } else {
            settingObat++;
            jumlahSet = 0; inputBuffer = 0; waktuIndex = 0;
            if (settingObat >= 3) {
              settingMode = false;
              lcd.clear(); lcd.setCursor(0, 0); lcd.print("Set Selesai!");
              delay(1500); lcd.clear();
              Serial3.println("NOTIF_SETJADWAL");  // hanya notifikasi, tidak kirim data jadwal
            } else {
              lcd.clear(); lcd.setCursor(0, 0);
              lcd.print("Jumlah Obat ");
              lcd.print((char)('A' + settingObat));
            }
          }
        } else {
          lcd.setCursor(0, 1);
          if (jumlahSet == 2) lcd.print("Menit: ");
          if (jumlahSet == 3) lcd.print("Detik: ");
        }
      }
    } else if (key == 'C') {
      inputBuffer = 0;
      lcd.setCursor(0, 1); lcd.print("Input: 0        ");
    }
  } else if (key == 'A') {
    settingMode = true; settingObat = 0; jumlahSet = 0; inputBuffer = 0;
    lcd.clear(); lcd.setCursor(0, 0); lcd.print("Jumlah Obat A:");
    lcd.setCursor(0, 1); lcd.print("Jumlah: ");
  }
}

void keluarkanObat(int index) {
  Servo* selectedServo = (index == 0) ? &servoSeal1 : (index == 1) ? &servoSeal12 : &servoSeal13;
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Ambil Obat...");

  if (index == 0 || index == 1) {
    selectedServo->write(100); delay(2000);
    selectedServo->write(84); delay(1000);
    selectedServo->write(80); delay(2000);
  } else {
    selectedServo->write(100); delay(2000);
    selectedServo->write(84); delay(1000);
    selectedServo->write(75); delay(2000);
  }
  selectedServo->write(90);
  delay(1000);

  Serial3.println("WAKTU_MINUM;" + String((char)('A' + index)));

  long startTime = millis();
  bool tanganTerdeteksi = false;
  while (millis() - startTime < 120000) {
    if (digitalRead(IR_SENSOR_PIN) == LOW) {
      tanganTerdeteksi = true;
      break;
    }
  }

  if (tanganTerdeteksi) {
    lcd.setCursor(0, 1); lcd.print("Obat Diambil");
    stokObat[index]--;
    Serial3.println("OBAT_DIAAMBIL;" + String(stokObat[index]));
  } else {
    lcd.setCursor(0, 1); lcd.print("Tidak Diambil!");
    Serial3.println("REMINDER;" + String(stokObat[index]));
  }
  delay(1500); lcd.clear();
}

void handleESPCommand() {
  if (Serial3.available()) {
    String cmd = Serial3.readStringUntil('\n');
    cmd.trim();

    if (cmd == "GET_STOK") {
      Serial3.println("INFO_STOK");
      for (int i = 0; i < 3; i++) {
        Serial3.println("Stok Obat " + String((char)('A' + i)) + ": " + stokObat[i]);
      }
      Serial3.println("END");
    } else if (cmd == "GET_JADWAL") {
      Serial3.println("INFO_JADWAL");
      for (int i = 0; i < 3; i++) {
        Serial3.println("Stok Obat " + String((char)('A' + i)) + ": " + stokObat[i]);
        for (int j = 0; j < jumlahObat[i]; j++) {
          char buf[20];
          sprintf(buf, "- %02d:%02d:%02d", jadwalObat[i][j][0], jadwalObat[i][j][1], jadwalObat[i][j][2]);
          Serial3.println(buf);
        }
      }
      Serial3.println("END");
    }
  }
}
