#include <Arduino.h>
#include <AccelStepper.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// PIN TANIMLARI
#define LDR_TL 34
#define LDR_TR 35
#define LDR_BL 32
#define LDR_BR 33
#define DIR_PIN 18
#define STEP_PIN 19
#define ONE_WIRE_BUS 4
#define POT_PIN 25

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);
Adafruit_BME280 bme;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// MOTOR
const int STEPS_PER_REV = 200;
const float MIN_ANGLE = 0.0;
const float MAX_ANGLE = 360.0;
const float CENTER_ANGLE = 180.0;       // Orta nokta - LDR dengede burada
const float MAX_ANGLE_DEVIATION = 170.0; // Merkezden max sapma (sınıra dayanmasın)

const int LDR_DEADBAND = 70;
const int TRACK_DIRECTION = 1;
const float SMOOTHING = 0.15;            // 0=donar, 1=anlık zıplar; 0.1-0.2 ideal

const unsigned long TRACK_INTERVAL = 100;
const unsigned long PRINT_INTERVAL = 1000;
const unsigned long RISK_INTERVAL  = 2000;   // 5000 -> 2000

unsigned long lastTrackTime = 0;
unsigned long lastPrintTime = 0;
unsigned long lastRiskTime  = 0;

float targetAngle = CENTER_ANGLE;
bool bmeOk = false;

// PROJE SABITLERI
const float ALBEDO_WATER     = 0.08;
const float REDUCTION_FACTOR = 0.592;
const float MODULE_AREA_M2   = 0.965;
const float CO2_FACTOR       = 0.475;
const float PANEL_POWER_W_M2 = 36.26;
const float COOLING_BOOST    = 1.08;

long angleToSteps(float a) { return round((a / 360.0) * STEPS_PER_REV); }
float stepsToAngle(long s) { return (s * 360.0) / STEPS_PER_REV; }
float limitAngle(float a) {
  if (a < MIN_ANGLE) return MIN_ANGLE;
  if (a > MAX_ANGLE) return MAX_ANGLE;
  return a;
}

float calcPM(float T_air, float RH, float u2, float Rs, float P, float T_water) {
  float es = 0.6108 * exp((17.27 * T_air) / (T_air + 237.3));
  float delta = (4098.0 * es) / pow(T_air + 237.3, 2);
  float ea = es * (RH / 100.0);
  float gamma = 0.000665 * P;
  float wCorr = constrain(1.0 + 0.02 * (T_water - T_air), 0.7, 1.5);
  float Rns = (1.0 - ALBEDO_WATER) * Rs;
  float Rn  = max(0.0f, Rns - 2.0f);
  float num = 0.408 * delta * Rn + gamma * (900.0 / (T_air + 273.0)) * u2 * (es - ea);
  float den = delta + gamma * (1.0 + 0.34 * u2);
  float eto = (num / den) * wCorr;
  return max(0.0f, eto);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("AKILLI YUZER GES");
  Serial.println("Orantisal LDR Kontrol + FAO-56 Risk Analizi");
  Serial.println("==============================================");

  analogReadResolution(12);

  Wire.begin(21, 22);
  if (bme.begin(0x76) || bme.begin(0x77)) {
    bmeOk = true;
    Serial.println("[OK] BME280 hazir.");
  } else {
    bmeOk = false;
    Serial.println("[HATA] BME280 BULUNAMADI! Wokwi diagram'i ve libraries.txt kontrol et.");
  }

  waterSensor.begin();
  Serial.printf("[OK] DS18B20 sensor sayisi: %d\n", waterSensor.getDeviceCount());
  if (waterSensor.getDeviceCount() > 0) waterSensor.setResolution(11);

  stepper.setMaxSpeed(800);
  stepper.setAcceleration(400);
  stepper.setCurrentPosition(angleToSteps(CENTER_ANGLE));
  stepper.moveTo(angleToSteps(targetAngle));

  Serial.printf("Aci sinirlari: %.0f - %.0f derece (merkez %.0f)\n",
                MIN_ANGLE, MAX_ANGLE, CENTER_ANGLE);
  Serial.println("==============================================");
}

void loop() {
  // === LDR ===
  int tl = analogRead(LDR_TL);
  int tr = analogRead(LDR_TR);
  int bl = analogRead(LDR_BL);
  int br = analogRead(LDR_BR);

  int leftAvg   = (tl + bl) / 2;
  int rightAvg  = (tr + br) / 2;
  int topAvg    = (tl + tr) / 2;
  int bottomAvg = (bl + br) / 2;
  int avgLdr    = (tl + tr + bl + br) / 4;
  int horizontalError = rightAvg - leftAvg;

  unsigned long now = millis();

  // === ORANTISAL KONTROL (DIRECT MAPPING + SMOOTHING) ===
  if (now - lastTrackTime >= TRACK_INTERVAL) {
    lastTrackTime = now;

    if (abs(horizontalError) > LDR_DEADBAND) {
      // Hatayi -1.0 .. +1.0 araligina cevir
      float errorRatio = (float)horizontalError / 4095.0;
      errorRatio = constrain(errorRatio, -1.0f, 1.0f);

      // Hedef offset: max ±170° (sinira dayanmasin)
      float idealTarget = CENTER_ANGLE + (errorRatio * MAX_ANGLE_DEVIATION) * TRACK_DIRECTION;
      idealTarget = limitAngle(idealTarget);

      // Yumusak gecis (1. derece IIR filtresi)
      targetAngle = targetAngle + SMOOTHING * (idealTarget - targetAngle);
    }

    stepper.moveTo(angleToSteps(targetAngle));
  }
  stepper.run();

  // === STANDART CIKTI ===
  if (now - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = now;
    float currentAngle = stepsToAngle(stepper.currentPosition());

    Serial.println("==============================================");
    Serial.print("LDR TL: "); Serial.print(tl);
    Serial.print(" | LDR TR: "); Serial.print(tr);
    Serial.print(" | LDR BL: "); Serial.print(bl);
    Serial.print(" | LDR BR: "); Serial.println(br);

    Serial.print("Sol Ortalama: "); Serial.print(leftAvg);
    Serial.print(" | Sag Ortalama: "); Serial.print(rightAvg);
    Serial.print(" | Fark: "); Serial.println(horizontalError);

    Serial.print("Ust Ortalama: "); Serial.print(topAvg);
    Serial.print(" | Alt Ortalama: "); Serial.println(bottomAvg);

    Serial.print("Hedef Aci: "); Serial.print(targetAngle, 1);
    Serial.print(" derece | Anlik Motor Acisi: "); Serial.print(currentAngle, 1);
    Serial.println(" derece");

    if (abs(horizontalError) <= LDR_DEADBAND) {
      Serial.println("Durum: Isik dengeli, motor bekliyor.");
    } else if (horizontalError > 0) {
      Serial.println("Durum: Sag taraf daha aydinlik, panel saga donuyor.");
    } else {
      Serial.println("Durum: Sol taraf daha aydinlik, panel sola donuyor.");
    }
    Serial.println("==============================================");
  }

  // === PENMAN-MONTEITH RAPORU (2sn'de 1) ===
  if (now - lastRiskTime >= RISK_INTERVAL) {
    lastRiskTime = now;

    float T_air, RH, P;
    if (bmeOk) {
      T_air = bme.readTemperature();
      RH    = bme.readHumidity();
      P     = bme.readPressure() / 100.0;
    } else {
      // Fallback: BME yoksa pot ile hava sicakligi simule et
      T_air = 15.0 + (analogRead(POT_PIN) / 4095.0) * 25.0;  // 15-40°C
      RH    = 60.0;
      P     = 1013.0;
    }

    waterSensor.requestTemperatures();
    float T_water = waterSensor.getTempCByIndex(0);
    bool waterOk = (T_water != DEVICE_DISCONNECTED_C && T_water != -127.0);

    if (isnan(T_air) || T_air < -40) T_air = 25.0;
    if (isnan(RH) || RH < 0)         RH    = 60.0;
    if (isnan(P) || P < 800)         P     = 1013.0;
    if (!waterOk) T_water = T_air - 2.0;

    float Rs = (avgLdr / 4095.0) * 30.0;
    float u2 = 2.5;  // Sabit ortalama (pot artik ETo simulasyonu icin gerekirse degistir)
    if (!bmeOk) u2 = 2.5;  // bmeOk false ise pot T_air icin kullaniliyor

    float eto = calcPM(T_air, RH, u2, Rs, P, T_water);

    int riskLevel; const char* riskName;
    if      (eto < 3.0) { riskLevel = 0; riskName = "DUSUK";  }
    else if (eto < 5.0) { riskLevel = 1; riskName = "ORTA";   }
    else if (eto < 7.0) { riskLevel = 2; riskName = "YUKSEK"; }
    else                { riskLevel = 3; riskName = "KRITIK"; }

    float dailyM3   = (eto / 1000.0) * MODULE_AREA_M2 * REDUCTION_FACTOR;
    float annualM3  = dailyM3 * 365.0;
    float annualKwh = MODULE_AREA_M2 * PANEL_POWER_W_M2 * 8.0 * COOLING_BOOST * 365.0 / 1000.0;
    float co2Kg     = annualKwh * CO2_FACTOR;

    Serial.println("----------------------------------------------");
    Serial.print("[PENMAN-MONTEITH RISK ANALIZI - FAO-56]  BME280: ");
    Serial.println(bmeOk ? "OK" : "HATA - fallback");

    Serial.print("Su Sicakligi (DS18B20): ");
    if (waterOk) { Serial.print(T_water, 2); Serial.println(" C"); }
    else { Serial.print("BAGLANTI YOK (tahmini "); Serial.print(T_water, 1); Serial.println(" C)"); }

    Serial.print("Hava: ");   Serial.print(T_air, 1); Serial.print(" C | ");
    Serial.print("Nem: %");   Serial.print(RH, 0);    Serial.print(" | ");
    Serial.print("Basinc: "); Serial.print(P, 0);     Serial.println(" hPa");

    Serial.print("Tahmini Rs: "); Serial.print(Rs, 2); Serial.print(" MJ/m2/gun | ");
    Serial.print("Ruzgar: ");     Serial.print(u2, 1); Serial.println(" m/s");

    Serial.print("ETo: "); Serial.print(eto, 3); Serial.println(" mm/gun");

    Serial.print("RISK: "); Serial.print(riskName); Serial.print(" [");
    for (int i = 0; i < 4; i++) Serial.print(i <= riskLevel ? "#" : "-");
    Serial.println("]");

    Serial.print("Gunluk Tasarruf: "); Serial.print(dailyM3, 4); Serial.print(" m3 | Yillik: ");
    Serial.print(annualM3, 2); Serial.println(" m3");

    Serial.print("Yillik Enerji: "); Serial.print(annualKwh, 1); Serial.print(" kWh | CO2: ");
    Serial.print(co2Kg, 1); Serial.println(" kg/yil");

    Serial.print("Oneri: ");
    switch (riskLevel) {
      case 0: Serial.println("Normal operasyon."); break;
      case 1: Serial.println("Izleme sikligi artirilsin."); break;
      case 2: Serial.println("Kapsama orani artirilsin!"); break;
      case 3: Serial.println("ACIL! Maksimum verim modu."); break;
    }
    Serial.println("----------------------------------------------");
  }
}