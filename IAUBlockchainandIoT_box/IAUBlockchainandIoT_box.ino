#include <FirebaseESP8266.h>          // Database
#include <ArduinoJson.h>              // Json
#include <MFRC522.h>                  // NFC
#include <Adafruit_SSD1306.h>         // Screen
#include <Servo.h>                    // Motor

// Configure WIFI
String wifi_ssid = "IAU Blockchain";
String wifi_password = "123454321";

// Configure DATABASE
#define DATABASE_URL "DATABASE_URL"
#define DATABASE_SECRET "DATABASE_SECRET"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Configure NFC reader
constexpr uint8_t RST_PIN = D0;
constexpr uint8_t SS_PIN = D8;
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Configure OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Configure Servo motor
Servo servo;

int permissionCounter = 0;
int permissionGoal = 3;
String previousNfcs = "";
String lastNfc = "";
boolean doorOpen = false;
int loopCounter = 0;

void setup() {
  Serial.begin(9600);

  // Lock the door
  servo.attach(2); // D4
  servo.write(160);

  // Display WIFI name and password
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(1);
  centerTextH("Searching for WiFi", 0);
  centerTextH("Name:", 8);
  centerTextH(wifi_ssid, 16);
  centerTextH("Password:", 40);
  centerTextH(wifi_password, 50);
  display.display();

  // WIFI connect
  WiFi.begin(wifi_ssid, wifi_password);
  while(WiFi.status() != WL_CONNECTED) { delay(300); }

  // Display CONNECTED
  display.clearDisplay();
  display.setTextSize(2);
  centerTextH("CONNECTED", 24);
  display.display();
  delay(100);

  // Setup database
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);

  // Erase force
  Firebase.setString(fbdo, "/box/force", "null");

  // Setup NFC reader
  SPI.begin();
  mfrc522.PCD_Init();

  displayLayout("", "Waiting for NFC", "", false);
  delay(100);
}

void loop() {
  star:

  // DISCONNECTED
  if (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setTextSize(1);
    centerTextH("DISCONNECTED", 30);
    display.display();

    // RECONNECT
    while(WiFi.status() != WL_CONNECTED) { delay(300); }
    
    display.clearDisplay();
    display.setTextSize(2);
    centerTextH("CONNECTED", 24);
    display.display();
    delay(100);
    displayLayout("", "Waiting for NFC", "", false);
    delay(100);
  }

  // Check for force lock or unlock
  Firebase.getString(fbdo, "/box/force");
  if (fbdo.stringData() == "unlock") {
    display.clearDisplay();
    display.setTextSize(2);
    centerTextH("Force", 16);
    centerTextH("unlock", 40);
    display.display();
    servo.write(0);
    Firebase.setString(fbdo, "/box/force", "null");
    delay(1000);
  } else if (fbdo.stringData() == "lock") {
    display.clearDisplay();
    display.setTextSize(2);
    centerTextH("Force", 16);
    centerTextH("lock", 40);
    display.display();
    servo.write(160);
    Firebase.setString(fbdo, "/box/force", "null");
    delay(1000);
  }

  // Reset NFC reader
  if (loopCounter >= 10) {
    loopCounter = 0;
    mfrc522.PCD_Init();
  }
  loopCounter++;
  
  String nfc = getUid();

  // No NFC tag
  if (nfc == "1" || nfc == "") {
    delay(1000);
    nfc = getUid();
    if (nfc == "1") {
      lastNfc = "";
      if (doorOpen == true) {

        // Counter until lock
        for(int i=3; i>=0; i--) {
          delay(1000);
          displayLayout("", String(i), "", true);
          nfc = getUid();
          if(previousNfcs.indexOf(nfc) != -1 && nfc != "1") {
            goto star;
          }
          nfc = getUid();
          if(previousNfcs.indexOf(nfc) != -1 && nfc != "1") {
            goto star;
          }
        }
        
        permissionCounter = 0;
        previousNfcs = "";
        displayLayout("", "Waiting for NFC", "", false);
        delay(100);
      }
    } else {
      goto cont;
    }
    return;
  }
  
  cont:
  // Same NFC tag
  if (nfc == lastNfc) {
    delay(1000);
    return;
  }

  // NFC tag detected
  lastNfc = nfc;
  nfcDetected(nfc);

  delay(1000);
}

void nfcDetected(String nfc) {
  displayLayout("Checking permission", "", "", false);
  delay(100);

  // Get all NFCs in DataBase
  Firebase.getString(fbdo, F("/members_nfc"));
  String all_nfc = fbdo.stringData();
  int nfc_index = all_nfc.indexOf(nfc);
  
  if (nfc_index != -1) {
    // Its a member
    all_nfc = all_nfc.substring(0, nfc_index - 1);
    String user_id = all_nfc.substring(all_nfc.lastIndexOf(",") + 1);
    JsonDocument doc;

    // Get member's information
    Firebase.getString(fbdo, "/members/" + user_id);
    deserializeJson(doc, fbdo.stringData());
    
    String firstName = lettersToEnglish(String(doc["name"]));
    String lastName = "";
    if (String(doc["surname"]) != "null") {
      lastName = lettersToEnglish(String(doc["surname"]));
    }
    
    String accessType = "";
    if (String(doc["box_access"]) == "normal") {
      accessType = "Normal Access";
      if(previousNfcs.indexOf(nfc) == -1) {
        permissionCounter++;
        previousNfcs = previousNfcs + "," + String(doc["nfc"]);
      }
    } else if (String(doc["box_access"]) == "super") {
      accessType = "Super Access";
      permissionCounter = permissionGoal;
      previousNfcs = previousNfcs + "," + String(doc["nfc"]);
    } else {
      accessType = "No Access";
    }
    displayLayout(firstName, lastName, accessType, true);
    delay(1000);
    if (permissionCounter < permissionGoal) {
      delay(500);
      displayLayout("", "Waiting for NFC", "", false);
      delay(100);
    }
  } else {
    // Not in DataBase
    displayLayout("", "Not a member", nfc, false);
    delay(2000);
    displayLayout("", "Waiting for NFC", "", false);
    delay(100);
  }
}

void displayLayout(String text, String text2, String text3, boolean bigName) {
  display.clearDisplay();

  display.setTextSize(bigName + 1);
  centerTextH(text, 16);
  centerTextH(text2, 35);
  display.setTextSize(1);
  centerTextH(text3, 56);
  
  if (permissionCounter >= permissionGoal) {
    display.setTextSize(1);
    centerTextH("UNLOCKED", 0);
    servo.write(0);
    doorOpen = true;
  } else {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(String(permissionCounter) + "/" + String(permissionGoal));
    display.setCursor(92, 0);
    display.println("LOCKED");
    servo.write(160);
    doorOpen = false;
  }

  display.display();
}

String getUid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "1";
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return "1";
  }
  return formatUid(mfrc522.uid.uidByte, mfrc522.uid.size);
}

String formatUid(byte *buffer, byte bufferSize) {
  String re = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) {
      re += '0';
    }
    re += String(buffer[i], HEX);
    re += ':';
  }
  re.remove(re.length() - 1);
  re.toUpperCase();
  return re;
}

String lettersToEnglish(String text) {
  text.replace("ö", "o");
  text.replace("ü", "u");
  text.replace("ş", "s");
  text.replace("ğ", "g");
  text.replace("ç", "c");

  text.replace("Ö", "O");
  text.replace("Ü", "U");
  text.replace("Ş", "S");
  text.replace("Ğ", "G");
  text.replace("Ç", "C");
  text.replace("İ", "I");

  return text;
}

void centerTextH(String text, int yyy) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, yyy);
  display.println(text);
}
