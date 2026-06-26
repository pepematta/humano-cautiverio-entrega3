/*
  El Humano en Cautiverio — Firmware Arduino (Entrega 3)
  ═══════════════════════════════════════════════════════

  PINES:
    Servo Trabajo         → D5
    Servo Ocio            → D6
    Servo Cuidado pers.   → D7
    Pulsador              → D2 (pull-up interno, a GND)
    RC522 SDA/SS          → D10
    RC522 RST             → D9
    RC522 SCK             → D13
    RC522 MOSI            → D11
    RC522 MISO            → D12
    RC522 VCC             → 3.3V  ← NO usar 5V
    RC522 GND             → GND

  PROTOCOLO SERIAL (9600 baud):
    ← Web envía:
        "A:angW,angL,angC\n"   ángulos directos en grados (0-180) con calibración ya aplicada
        "M:minW,minL,minC\n"   minutos crudos (el Arduino calcula ángulos)
    → Arduino envía:
        "-> TU DIA REAL\n"
        "-> CAZADOR-RECOLECTOR\n"
        "NFC:CL\n"
        "NFC:US\n"
        "NFC:UNKNOWN\n"
        "UID detectado: XX XX XX XX\n"   (solo en modo registro)

  TARJETAS NFC:
    1. Pon NFC_REGISTERED = false, sube, abre Monitor Serial 9600 baud
    2. Acerca cada tarjeta → copia el UID impreso
    3. Pega los bytes en NFC_UID_CHILE / NFC_UID_USA
    4. Cambia NFC_REGISTERED = true y vuelve a subir

  LIBRERÍA REQUERIDA:
    MFRC522 by GithubCommunity (Library Manager de Arduino IDE)
*/

#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// ─── PINES ──────────────────────────────────────────────────
#define PIN_SERVO_WORK    5
#define PIN_SERVO_LEISURE 6
#define PIN_SERVO_CARE    7
#define PIN_BUTTON        2
#define PIN_NFC_SS        10
#define PIN_NFC_RST       9

// ─── RANGOS ─────────────────────────────────────────────────
#define MAX_WORK    600
#define MAX_LEISURE 480
#define MAX_CARE    800

#define PRIM_WORK    240
#define PRIM_LEISURE 450
#define PRIM_CARE    690

// ─── CONFIGURACIÓN NFC ──────────────────────────────────────
#define NFC_REGISTERED false   // ← cambiar a true tras registrar

const byte NFC_UID_CHILE[4] = { 0xDE, 0xAD, 0xBE, 0xEF };  // ← reemplazar
const byte NFC_UID_USA[4]   = { 0xCA, 0xFE, 0xBA, 0xBE };  // ← reemplazar

// ─── OBJETOS ────────────────────────────────────────────────
Servo servoWork, servoLeisure, servoCare;
MFRC522 nfc(PIN_NFC_SS, PIN_NFC_RST);

// ─── ESTADO ─────────────────────────────────────────────────
bool modoPrimitivo = false;
bool buttonLastState = HIGH;

// Ángulos objetivo recibidos por serial (ya con calibración aplicada desde la web)
int angWork    = 0;
int angLeisure = 0;
int angCare    = 0;

// Posición suavizada actual de cada servo
float posWork    = 0;
float posLeisure = 0;
float posCare    = 0;

unsigned long lastNfcTime = 0;
#define NFC_DEBOUNCE_MS 1500

// ─── HELPERS ────────────────────────────────────────────────
int minutosToAngle(int min, int maxMin) {
  return constrain(map(min, 0, maxMin, 0, 180), 0, 180);
}

void suavizarServos(int tw, int tl, int tc) {
  const float k = 0.08;
  posWork    += (tw - posWork)    * k;
  posLeisure += (tl - posLeisure) * k;
  posCare    += (tc - posCare)    * k;
  servoWork.write((int)posWork);
  servoLeisure.write((int)posLeisure);
  servoCare.write((int)posCare);
}

bool uidEquals(byte *uid, byte sz, const byte *ref, byte refSz) {
  if (sz != refSz) return false;
  for (byte i = 0; i < sz; i++) if (uid[i] != ref[i]) return false;
  return true;
}

// ─── SETUP ──────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  SPI.begin();
  nfc.PCD_Init();

  servoWork.attach(PIN_SERVO_WORK);
  servoLeisure.attach(PIN_SERVO_LEISURE);
  servoCare.attach(PIN_SERVO_CARE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  servoWork.write(0);
  servoLeisure.write(0);
  servoCare.write(0);

  Serial.println("Humano en Cautiverio listo");
  if (!NFC_REGISTERED) Serial.println("Modo registro NFC activo.");
}

// ─── LOOP ───────────────────────────────────────────────────
void loop() {
  leerSerial();
  leerPulsador();
  leerNFC();

  int tw, tl, tc;
  if (modoPrimitivo) {
    tw = minutosToAngle(PRIM_WORK,    MAX_WORK);
    tl = minutosToAngle(PRIM_LEISURE, MAX_LEISURE);
    tc = minutosToAngle(PRIM_CARE,    MAX_CARE);
  } else {
    tw = angWork;
    tl = angLeisure;
    tc = angCare;
  }
  suavizarServos(tw, tl, tc);
  delay(20);
}

// ─── LEER SERIAL ────────────────────────────────────────────
void leerSerial() {
  static String buf = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { parsearLinea(buf); buf = ""; }
    else { buf += c; if (buf.length() > 60) buf = ""; }
  }
}

void parsearLinea(String s) {
  s.trim();
  if (s.startsWith("A:")) {
    // Protocolo de ángulos directos (calibración ya aplicada desde la web)
    // "A:angW,angL,angC"
    String data = s.substring(2);
    int c1 = data.indexOf(',');
    int c2 = data.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) return;
    angWork    = constrain(data.substring(0, c1).toInt(), 0, 180);
    angLeisure = constrain(data.substring(c1+1, c2).toInt(), 0, 180);
    angCare    = constrain(data.substring(c2+1).toInt(), 0, 180);

  } else if (s.startsWith("M:")) {
    // Protocolo de minutos crudos (el Arduino calcula ángulos)
    // "M:minW,minL,minC"
    String data = s.substring(2);
    int c1 = data.indexOf(',');
    int c2 = data.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) return;
    int w = data.substring(0, c1).toInt();
    int l = data.substring(c1+1, c2).toInt();
    int c = data.substring(c2+1).toInt();
    if (w >= 0 && w <= MAX_WORK)    angWork    = minutosToAngle(w, MAX_WORK);
    if (l >= 0 && l <= MAX_LEISURE) angLeisure = minutosToAngle(l, MAX_LEISURE);
    if (c >= 0 && c <= MAX_CARE)    angCare    = minutosToAngle(c, MAX_CARE);
  }
  // Formato legacy sin prefijo (compatibilidad con versión anterior)
  else {
    int c1 = s.indexOf(',');
    int c2 = s.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) return;
    int w = s.substring(0, c1).toInt();
    int l = s.substring(c1+1, c2).toInt();
    int c = s.substring(c2+1).toInt();
    if (w >= 0 && w <= MAX_WORK)    angWork    = minutosToAngle(w, MAX_WORK);
    if (l >= 0 && l <= MAX_LEISURE) angLeisure = minutosToAngle(l, MAX_LEISURE);
    if (c >= 0 && c <= MAX_CARE)    angCare    = minutosToAngle(c, MAX_CARE);
  }
}

// ─── LEER PULSADOR ──────────────────────────────────────────
void leerPulsador() {
  bool state = digitalRead(PIN_BUTTON);
  if (state == buttonLastState) return;
  delay(30);
  buttonLastState = state;
  if (state == LOW) {
    modoPrimitivo = true;
    Serial.println("-> CAZADOR-RECOLECTOR");
  } else {
    modoPrimitivo = false;
    Serial.println("-> TU DIA REAL");
  }
}

// ─── LEER NFC ───────────────────────────────────────────────
void leerNFC() {
  if (millis() - lastNfcTime < NFC_DEBOUNCE_MS) return;
  if (!nfc.PICC_IsNewCardPresent()) return;
  if (!nfc.PICC_ReadCardSerial())   return;

  lastNfcTime = millis();
  byte *uid  = nfc.uid.uidByte;
  byte  size = nfc.uid.size;

  if (!NFC_REGISTERED) {
    Serial.print("UID detectado: ");
    for (byte i = 0; i < size; i++) {
      if (uid[i] < 0x10) Serial.print("0");
      Serial.print(uid[i], HEX);
      if (i < size - 1) Serial.print(" ");
    }
    Serial.println();
  } else {
    if      (uidEquals(uid, size, NFC_UID_CHILE, 4)) Serial.println("NFC:CL");
    else if (uidEquals(uid, size, NFC_UID_USA,   4)) Serial.println("NFC:US");
    else                                              Serial.println("NFC:UNKNOWN");
  }

  nfc.PICC_HaltA();
  nfc.PCD_StopCrypto1();
}
