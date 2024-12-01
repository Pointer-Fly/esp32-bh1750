#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include "HAL_DLight.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// function prototypes
void Disp_Init();

// Use dedicated hardware SPI pins
M5_DLight sensor;

#define MAX_WIDTH 2500
#define MIN_WIDTH 500
#define SERVO_PIN 13

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_SERVO_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHARACTERISTIC_LIGHT_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a0"

// 定义 servo 对象
Servo my_servo;
BLECharacteristic *pCharacteristicLight;

enum MENU_ENUM
{
  MENU_SP,
  MENU_AP,
  MENU_ISO,
  MENU_MODE,
};
MENU_ENUM gCurMenu = MENU_ISO;

enum MODE_ENUM
{
  MODE_AV,
  MODE_TV,
};
MODE_ENUM gModel = MODE_AV;

static int gIsoList[] = {12, 25, 50, 64, 80, 100, 125, 160, 200, 250, 320, 400, 500, 600, 800, 1600, 3200, 6400, 12800, 25600, 51200};
static int gIsoListSize = sizeof(gIsoList) / sizeof(int);
int gCurIsoIndex = 5;
int gIso = gIsoList[gCurIsoIndex];

static float gSpList[] = {1.0 / 1, 1.0 / 2, 1.0 / 3, 1.0 / 4, 1.0 / 5, 1.0 / 6, 1.0 / 8, 1.0 / 10, 1.0 / 13, 1.0 / 15, 1.0 / 20, 1.0 / 25, 1.0 / 30, 1.0 / 40, 1.0 / 50, 1.0 / 60, 1.0 / 80, 1.0 / 100, 1.0 / 125, 1.0 / 160, 1.0 / 200, 1.0 / 250, 1.0 / 320, 1.0 / 400, 1.0 / 500, 1.0 / 640, 1.0 / 800, 1.0 / 1000, 1.0 / 1250, 1.0 / 1600, 1.0 / 2000, 1.0 / 2500, 1.0 / 3200, 1.0 / 4000, 1.0 / 5000, 1.0 / 6400, 1.0 / 8000, 1.0 / 12000};
static int gSpListSize = sizeof(gSpList) / sizeof(float);
int gSpIndex = 8;
float gSp = gSpList[gSpIndex];

static float gApertureList[] = {1, 1.2, 1.4, 1.8, 2, 2.4, 2.8, 3, 3.2, 3.5, 4, 4.5, 5, 5.6, 6.3, 7.1, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 26, 28, 32, 36, 40, 45, 52, 56, 64};
static int gApertureListSize = sizeof(gApertureList) / sizeof(float);
int gCurApertureIndex = 5;
float gAperture = gApertureList[gCurApertureIndex];
int gLux = 0;

void servoInit()
{
  // 分配硬件定时器
  ESP32PWM::allocateTimer(0);
  // 设置频率
  my_servo.setPeriodHertz(50);
  // 关联 servo 对象与 GPIO 引脚，设置脉宽范围
  my_servo.attach(SERVO_PIN, MIN_WIDTH, MAX_WIDTH);
}
void servoDown()
{
  for (int i = 180; i >= 0; i--)
  {
    my_servo.write(i);
    delay(15);
  }
}

void servoUp()
{
  for (int i = 0; i <= 180; i++)
  {
    my_servo.write(i);
    delay(15);
  }
}

class BLECallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    // 区分不同的特征值
    if (pCharacteristic->getUUID().equals(BLEUUID(CHARACTERISTIC_SERVO_UUID)))
    {
      if (value.length() > 0)
      {
        Serial.println("*********");
        if (value[0] == '0')
        {
          Serial.println("Servo down");
          servoDown();
        }
        else if (value[0] == '1')
        {
          Serial.println("Servo up");
          servoUp();
        }
        Serial.println();
        Serial.println("*********");
      }
    }
  }
};

void BLE_Init()
{

  Serial.println("Starting BLE work!");

  BLEDevice::init("ESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristicServo = pService->createCharacteristic(
      CHARACTERISTIC_SERVO_UUID,
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristicLight = pService->createCharacteristic(
      CHARACTERISTIC_LIGHT_UUID,
      BLECharacteristic::PROPERTY_READ);

  pCharacteristicServo->setValue("0");
  pCharacteristicServo->setCallbacks(new BLECallbacks());

  pCharacteristicLight->setValue("0");
  pCharacteristicLight->setCallbacks(new BLECallbacks());

  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void switchMenu()
{
  if (gModel == MODE_AV)
  {
    gCurMenu = (gCurMenu == MENU_ISO) ? MENU_AP : (gCurMenu == MENU_AP) ? MENU_MODE
                                                                        : MENU_ISO;
  }
  else
  {
    gCurMenu = (gCurMenu == MENU_ISO) ? MENU_SP : (gCurMenu == MENU_SP) ? MENU_MODE
                                                                        : MENU_ISO;
  }
}

void adjustValue(int direction)
{
  switch (gCurMenu)
  {
  case MENU_SP:
    gSpIndex = (gSpIndex + direction + gSpListSize) % gSpListSize;
    gSp = gSpList[gSpIndex];
    break;
  case MENU_AP:
    gCurApertureIndex = (gCurApertureIndex + direction + gApertureListSize) % gApertureListSize;
    gAperture = gApertureList[gCurApertureIndex];
    break;
  case MENU_ISO:
    gCurIsoIndex = (gCurIsoIndex + direction + gIsoListSize) % gIsoListSize;
    gIso = gIsoList[gCurIsoIndex];
    break;
  case MENU_MODE:
    gModel = (gModel == MODE_AV) ? MODE_TV : MODE_AV;
    break;
  }
}

void keyScanTask(void *pvParameters)
{
  while (1)
  {
    if (digitalRead(17) == 0 || digitalRead(18) == 0)
    {
      delay(50);
      if (digitalRead(17) == 0 && digitalRead(18) == 0)
      {
        switchMenu();
        while (digitalRead(17) == 0)
        {
          delay(1);
        }
      }
      else if (digitalRead(17) == 0)
      {
        adjustValue(1);
        while (digitalRead(17) == 0)
        {
          delay(1);
        }
      }
      else if (digitalRead(18) == 0)
      {
        adjustValue(-1);
        while (digitalRead(18) == 0)
        {
          delay(1);
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // Initialize the keypad
  pinMode(18, INPUT);
  pinMode(17, INPUT);
  // sensor.begin();
  // delay(500);
  // sensor.setMode(CONTINUOUSLY_H_RESOLUTION_MODE);

  // servoInit();
  // Disp_Init();
  BLE_Init();
  // xTaskCreatePinnedToCore(keyScanTask, "keyScanTask", 4096, NULL, 1, NULL, 1);
}

void displayLx()
{
  gLux = sensor.getLUX();
  pCharacteristicLight->setValue(String(gLux).c_str());
  tft.setTextSize(2);
  tft.setCursor(5, 115);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.printf("%d Lx        \n", gLux);
}

void displayMode()
{
  int thisColor = gCurMenu == MENU_MODE ? ST77XX_GREEN : ST77XX_WHITE;
  tft.fillRect(214, 114, 26, 18, thisColor);
  tft.setTextColor(ST77XX_BLACK, thisColor);
  tft.setTextSize(2);
  tft.setCursor(216, 116);
  tft.printf("%s\n", gModel == MODE_AV ? "AV" : "TV");
}

void displaySP()
{
  int thisColor = gCurMenu == MENU_SP ? ST77XX_GREEN : ST77XX_WHITE;
  tft.setTextColor(thisColor, ST77XX_BLACK);
  tft.drawRect(0, 0, 240, 40, thisColor);
  tft.setTextSize(4);
  tft.setCursor(5, 5);
  tft.printf("1/%.0f\n", 1.0 / gSp);
}

void displayAperture()
{
  int thisColor = gCurMenu == MENU_AP ? ST77XX_GREEN : ST77XX_WHITE;
  tft.setTextColor(thisColor, ST77XX_BLACK);
  tft.drawRect(155, 55, 85, 50, thisColor);
  tft.setCursor(160, 50);
  tft.setTextSize(2);
  tft.printf("F\n");
  tft.setCursor(160, 70);
  tft.setTextSize(4);
  if (gAperture < 10)
    tft.printf("%.1f\n", gAperture);
  else
    tft.printf("%.0f\n", gAperture);
}

void displayISO()
{
  int thisColor = gCurMenu == MENU_ISO ? ST77XX_GREEN : ST77XX_WHITE;
  tft.setTextColor(thisColor, ST77XX_BLACK);
  tft.drawRect(0, 55, 150, 50, thisColor);
  tft.setCursor(5, 50);
  tft.setTextSize(2);
  tft.printf("ISO\n");
  tft.setCursor(5, 70);
  tft.setTextSize(4);
  tft.printf("%d\n", gIso);
}

void Disp_Init()
{
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
}

// 计算快门速度
void calculateShutterSpeed()
{
  float K = 12.5;
  gSp = (gAperture * gAperture * 100) / (K * gIso * gLux);
}

// 计算光圈
void calculateAperture()
{
  float K = 12.5;
  gAperture = sqrt((gIso * gLux * K * gSp) / 100);
}

void calcFunc()
{
  if (gModel == MODE_AV)
  {
    calculateShutterSpeed();
  }
  else
  {
    calculateAperture();
  }
}

void loop()
{
  // displayLx();
  // displaySP();
  // displayAperture();
  // displayISO();
  // displayMode();
  // tft.setCursor(0, 0);
  // delay(50);
}
