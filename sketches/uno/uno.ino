#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 13, d5 = 8, d6 = 7, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#include <SoftwareSerial.h>
const int espserial_rx = 3, espserial_tx = 2, espserial_speed = 9600;
SoftwareSerial esp_serial(espserial_rx, espserial_tx);
int  tcs34725_payload[4];
int  apds9660_payload[4];
char esp_payload[256];

#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include "Adafruit_TCS34725.h"

#define TCS34725_R_Coef 0.136
#define TCS34725_G_Coef 1.000
#define TCS34725_B_Coef -0.444
#define TCS34725_GA 1.0
#define TCS34725_DF 310.0
#define TCS34725_CT_Coef 3810.0
#define TCS34725_CT_Offset 1391.0

class tcs34725 {
private:
  struct tcs_agc {
    tcs34725Gain_t ag;
    tcs34725IntegrationTime_t at;
    uint16_t mincnt;
    uint16_t maxcnt;
  };
  
  static const tcs_agc agc_lst[];
  uint16_t agc_cur;

  void setGainTime(void);
  Adafruit_TCS34725 tcs;

public:
  tcs34725(void);

  boolean begin(void);
  void getData(void);

  boolean isAvailable, isSaturated;
  uint16_t againx, atime, atime_ms;
  uint16_t r, g, b, c;
  uint16_t ir;
  uint16_t r_comp, g_comp, b_comp, c_comp;
  uint16_t saturation, saturation75;
  float cratio, cpl, ct, lux, maxlux;
};

const tcs34725::tcs_agc tcs34725::agc_lst[] = {
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_700MS,     0, 20000 },
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_154MS,  4990, 63000 },
  { TCS34725_GAIN_16X, TCS34725_INTEGRATIONTIME_154MS, 16790, 63000 },
  { TCS34725_GAIN_4X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 63000 },
  { TCS34725_GAIN_1X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 0 }
};

tcs34725::tcs34725() : agc_cur(0), isAvailable(0), isSaturated(0) {
}

boolean tcs34725::begin(void) {
  tcs = Adafruit_TCS34725(agc_lst[agc_cur].at, agc_lst[agc_cur].ag);
  if ((isAvailable = tcs.begin()))
    setGainTime();
  return(isAvailable);
}

void tcs34725::setGainTime(void) {
  tcs.setGain(agc_lst[agc_cur].ag);
  tcs.setIntegrationTime(agc_lst[agc_cur].at);
  atime = int(agc_lst[agc_cur].at);
  atime_ms = ((256 - atime) * 2.4);
  switch(agc_lst[agc_cur].ag) {
  case TCS34725_GAIN_1X:
    againx = 1;
    break;
  case TCS34725_GAIN_4X:
    againx = 4;
    break;
  case TCS34725_GAIN_16X:
    againx = 16;
    break;
  case TCS34725_GAIN_60X:
    againx = 60;
    break;
  }
}

void tcs34725::getData(void) {
  // read the sensor and autorange if necessary
  tcs.getRawData(&r, &g, &b, &c);
  while(1) {
    if (agc_lst[agc_cur].maxcnt && c > agc_lst[agc_cur].maxcnt)
      agc_cur++;
    else if (agc_lst[agc_cur].mincnt && c < agc_lst[agc_cur].mincnt)
      agc_cur--;
    else break;

    setGainTime();
    delay((256 - atime) * 2.4 * 2); // shock absorber
    tcs.getRawData(&r, &g, &b, &c);
    break;
  }

  // DN40 calculations
  ir = (r + g + b > c) ? (r + g + b - c) / 2 : 0;
  r_comp = r - ir;
  g_comp = g - ir;
  b_comp = b - ir;
  c_comp = c - ir;
  cratio = float(ir) / float(c);

  saturation = ((256 - atime) > 63) ? 65535 : 1024 * (256 - atime);
  saturation75 = (atime_ms < 150) ? (saturation - saturation / 4) : saturation;
  isSaturated = (atime_ms < 150 && c > saturation75) ? 1 : 0;
  cpl = (atime_ms * againx) / (TCS34725_GA * TCS34725_DF);
  maxlux = 65535 / (cpl * 3);

  lux = (TCS34725_R_Coef * float(r_comp) + TCS34725_G_Coef * float(g_comp) + TCS34725_B_Coef * float(b_comp)) / cpl;
  ct = TCS34725_CT_Coef * float(b_comp) / float(r_comp) + TCS34725_CT_Offset;
}

tcs34725 rgb_sensor;

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0;
uint16_t green_light = 0;
uint16_t blue_light = 0;

void initAPDS9960() {
  Serial.println(F("------------------------------"));
  Serial.println(F("SparkFun APDS-9960 ColorSensor"));
  Serial.println(F("------------------------------"));
  
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
  if ( apds.enableLightSensor(false) ) {
    Serial.println(F("Light sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during light sensor init!"));
  }
}

void initTCS34725() {
  Serial.println(F("-----------------------------"));
  Serial.println(F("AdaFruit TCS34725 ColorSensor"));
  Serial.println(F("-----------------------------"));
  
  rgb_sensor.begin();
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW); // @gremlins Bright light, bright light!

  Serial.println(F("TCS34725 initialization complete"));
}

void setup() {
  Serial.begin(115200);
  esp_serial.begin(espserial_speed);

  initAPDS9960();
  initTCS34725();
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Hello world!");
  
  delay(500);
}

void readAPDS9960() {
  if (  !apds.readAmbientLight(ambient_light) ||
        !apds.readRedLight(red_light) ||
        !apds.readGreenLight(green_light) ||
        !apds.readBlueLight(blue_light) ) {
    Serial.println("Error reading light values");
  } else {
    Serial.print("Ambient: ");
    Serial.print(ambient_light);
    Serial.print(" Red: ");
    Serial.print(red_light);
    Serial.print(" Green: ");
    Serial.print(green_light);
    Serial.print(" Blue: ");
    Serial.println(blue_light);

    lcd.setCursor(0, 1); lcd.print(ambient_light); lcd.print("  ");
    lcd.setCursor(4, 1); lcd.print(red_light);     lcd.print("  ");
    lcd.setCursor(8, 1); lcd.print(green_light);   lcd.print("  ");
    lcd.setCursor(12, 1); lcd.print(blue_light);   lcd.print("  ");

    apds9660_payload[0] = ambient_light;
    apds9660_payload[1] = red_light;
    apds9660_payload[2] = green_light;
    apds9660_payload[3] = blue_light;
  }
}

void readTCS34725() {
  rgb_sensor.getData();
  Serial.print(F("Gain:"));
  Serial.print(rgb_sensor.againx);
  Serial.print(F("x "));
  Serial.print(F("Time:"));
  Serial.print(rgb_sensor.atime_ms);
  Serial.print(F("ms (0x"));
  Serial.print(rgb_sensor.atime, HEX);
  Serial.println(F(")"));

  Serial.print(F("Raw R:"));
  Serial.print(rgb_sensor.r);
  Serial.print(F(" G:"));
  Serial.print(rgb_sensor.g);
  Serial.print(F(" B:"));
  Serial.print(rgb_sensor.b);
  Serial.print(F(" C:"));
  Serial.println(rgb_sensor.c);

  Serial.print(F("IR:"));
  Serial.print(rgb_sensor.ir);
  Serial.print(F(" CRATIO:"));
  Serial.print(rgb_sensor.cratio);
  Serial.print(F(" Sat:"));
  Serial.print(rgb_sensor.saturation);
  Serial.print(F(" Sat75:"));
  Serial.print(rgb_sensor.saturation75);
  Serial.print(F(" "));
  Serial.println(rgb_sensor.isSaturated ? "*SATURATED*" : "");

  Serial.print(F("CPL:"));
  Serial.print(rgb_sensor.cpl);
  Serial.print(F(" Max lux:"));
  Serial.println(rgb_sensor.maxlux);

  Serial.print(F("Compensated R:"));
  Serial.print(rgb_sensor.r_comp);
  Serial.print(F(" G:"));
  Serial.print(rgb_sensor.g_comp);
  Serial.print(F(" B:"));
  Serial.print(rgb_sensor.b_comp);
  Serial.print(F(" C:"));
  Serial.println(rgb_sensor.c_comp);

  Serial.print(F("Lux:"));
  Serial.print(rgb_sensor.lux);
  Serial.print(F(" CT:"));
  Serial.print(rgb_sensor.ct);
  Serial.println(F("K"));
  
  lcd.setCursor(0, 0); lcd.print(rgb_sensor.lux); lcd.print("  ");
  lcd.setCursor(8, 0); lcd.print(rgb_sensor.ct); lcd.print("K  ");

  tcs34725_payload[0] = (int)rgb_sensor.lux;
  tcs34725_payload[1] = ((int)(rgb_sensor.lux * 100) % 100);
  tcs34725_payload[2] = (int)rgb_sensor.ct;
  tcs34725_payload[3] = ((int)(rgb_sensor.ct  * 100) % 100);
}

void loop() {
  readAPDS9960();
  readTCS34725();
  delay(1000);
  
  sprintf(esp_payload, "%i,%i;%i,%i,%i,%i",
    tcs34725_payload[0],
    tcs34725_payload[2],
    apds9660_payload[0],
    apds9660_payload[1],
    apds9660_payload[2],
    apds9660_payload[3]);
  esp_serial.println(esp_payload);
}
