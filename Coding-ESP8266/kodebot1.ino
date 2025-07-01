#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// WiFi
const char* ssid = "mncplay505";
const char* password = "12345678";

// Telegram
const char* botToken = "7574587372:AAFcZCJxauny4Tt5VBmnXU_fE58-6cw0q3I";
const int64_t chatID = 1010801913;

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

String buffer = "";
String infoBuffer = "";
bool infoMode = false;
bool isStok = false;

unsigned long lastCheck = 0;
const unsigned long interval = 2000; // interval cek pesan Telegram

void setup() {
  Serial.begin(115200);
  client.setInsecure();

  WiFi.begin(ssid, password);
  Serial.println("Menghubungkan ke WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Tersambung");
  bot.sendMessage(String(chatID), "🤖 Bot Telegram siap menerima notifikasi dari kotak obat.", "");
}

void loop() {
  // Menerima data dari ATmega
  if (Serial.available()) {
    buffer = Serial.readStringUntil('\n');
    buffer.trim();

    if (buffer.startsWith("WAKTU_MINUM")) {
      String komp = splitString(buffer, ';', 1);
      bot.sendMessage(String(chatID), "⏰ Waktunya minum obat dari kompartemen " + komp + "!", "");
    }
    else if (buffer.startsWith("OBAT_DIAAMBIL")) {
      String sisa = splitString(buffer, ';', 1);
      bot.sendMessage(String(chatID), "✅ Obat telah diambil. Sisa stok: " + sisa, "");
    }
    else if (buffer.startsWith("REMINDER")) {
      String sisa = splitString(buffer, ';', 1);
      bot.sendMessage(String(chatID), "⚠️ Obat belum diambil dalam 2 menit! Sisa stok: " + sisa, "");
    }
    else if (buffer == "NOTIF_SETJADWAL") {
      bot.sendMessage(String(chatID), "✅ Jadwal obat berhasil disetting. Sistem siap berjalan.", "");
    }
    else if (buffer == "INFO_JADWAL") {
      infoBuffer = "📋 Info Jadwal Obat:\n";
      infoMode = true;
      isStok = false;
    }
    else if (buffer == "INFO_STOK") {
      infoBuffer = "📦 Info Stok Obat:\n";
      infoMode = true;
      isStok = true;
    }
    else if (buffer == "END" && infoMode) {
      bot.sendMessage(String(chatID), infoBuffer, "");
      infoBuffer = "";
      infoMode = false;
    }
    else if (infoMode) {
      if (isStok && buffer.startsWith("Stok Obat")) {
        char komp = buffer.charAt(10);
        String stok = buffer.substring(buffer.indexOf(":") + 2);
        infoBuffer += "Kompartemen ";
        infoBuffer += komp;
        infoBuffer += " = ";
        infoBuffer += stok;
        infoBuffer += "\n";
      } else if (!isStok) {
        if (buffer.startsWith("Stok Obat")) {
          char komp = buffer.charAt(10);
          infoBuffer += "\nKompartemen ";
          infoBuffer += komp;
          infoBuffer += " =\n";
        } else if (buffer.startsWith("-")) {
          infoBuffer += buffer + "\n";
        }
      }
    }
  }

  // Menerima perintah dari Telegram
  if (millis() - lastCheck > interval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String text = bot.messages[i].text;
        String fromID = bot.messages[i].chat_id;

        if (fromID.toInt() != chatID) continue;

        if (text == "/cek_stok") {
          Serial.println("GET_STOK");
        } else if (text == "/cek_jadwal") {
          Serial.println("GET_JADWAL");
        } else {
          bot.sendMessage(fromID, "❌ Perintah tidak dikenali.\nGunakan:\n/cek_stok\n/cek_jadwal", "");
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastCheck = millis();
  }
}

// Fungsi untuk memecah string dengan pemisah
String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return (found > index) ? data.substring(strIndex[0], strIndex[1]) : "";
}
