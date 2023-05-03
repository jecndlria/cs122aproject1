// #include <SoftwareSerial.h>
// SoftwareSerial esp8266(3, 4);
// #include <Adafruit_ESP8266.h>

// Humidifier Declarations
#define BUTTON_PIN 2
  bool humidifierOn = false;          
  bool humidifierButtonHeld = false;  // Prevent button holds from registering
#define WATER_PIN A1
  unsigned int lastSeenWater = 0;     // Prevents unnecessary updates
  unsigned int waterLevel = 0;
#define RELAY_PIN 6
  unsigned long lastSeenTime = 0;     // Used for timer mode
  unsigned long minuteTimer = 0;      // Used to provide live countdown of timer in minutes

// IR Declarations
#include <IRremote.h>
#define IR_PIN 4
#define IR0 0xEE110707
#define IR1 0xFB040707
#define IR2 0xFA050707
#define IR3 0xF9060707
#define IR4 0xF7080707
#define IR5 0xF6090707
#define IR6 0xF50A0707
#define IR7 0xF30C0707
#define IR8 0xF20D0707
#define IR9 0xF10E0707
#define thresholdModeButton 0xEC130707     // PRE-CH
#define timerModeButton     0xDC230707     // -
#define powerButton         0xFD020707     // Power
  IRrecv irrecv(IR_PIN);
  decode_results results;
  bool switchThresh = false;               // Flag from IR state machine to Humidifier state machine
  bool switchTimer = false;                // Flag from IR state machine to Humidifier state machine
  bool powerButtonFlag = false;            // Flag from IR state machine to Humidifier state machine   
  String humidityThresholdString = "";     // Built in IR state machine, converted to int
  int humidityThreshold = 0;
  String humidifierTimerString = "";       // Built in IR state machine, converted to int
  unsigned long humidifierTimer = 0;       // Used in translation from min -> ms
  unsigned int humidifierTimerMinutes = 0; 

// DHT Declarations
#include <EduIntro.h>
#define DHT_PIN A0
  float fahrenheit;
  float lastSeenFahrenheit;   // Prevents unnecessary updates
  int humidity; 
  int lastSeenHumidity;       // Prevents unnecessary updates
  DHT11 dht11(DHT_PIN);

// LCD Screen
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#define TFT_CS 10
#define TFT_RESET 8
#define TFT_A0 9
#define TFT_SDA 11
#define TFT_SCK 13
  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_A0, TFT_SDA, TFT_SCK, TFT_RESET);
  
#define numTasks 4

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

task tasks[numTasks];

/*
*
* DHT
*
*/

enum DHT_STATES {DHT_INIT};
int dhtTick(int state)
{
  switch(state)
  {
    case DHT_INIT:
      dht11.update();
      lastSeenFahrenheit = fahrenheit;
      lastSeenHumidity = humidity;
      fahrenheit = dht11.readFahrenheit();
      humidity = dht11.readHumidity();
      if (lastSeenFahrenheit != fahrenheit) lcdUpdateTemp();
      if (lastSeenHumidity != humidity) lcdUpdateHumidity();

      break;      
  }
  return state;  
}

/*
*
* DHT
*
*/

/* 
*
* LCD
*
*/

void initializeScreen()
{
  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);

  tft.fillRect(0, 0, 128, 16, ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.println("TEMPERATURE(F): ");

  tft.fillRect(0, 16, 128, 32, ST7735_BLACK);
  tft.setCursor(0, 16);
  tft.println("HUMIDITY: ");

  tft.fillRect(0, 32, 128, 48, ST7735_BLACK);
  tft.setCursor(0, 32);
  tft.println("WATER LEVEL: ");

  tft.fillRect(0, 48, 128, 64, ST7735_BLACK);
  tft.setCursor(0, 48);
  tft.println("STATUS: ");
  tft.setTextColor(ST7735_RED);
  tft.setCursor(40, 48);
  tft.println("OFF");
}

void lcdUpdateTemp()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(88, 0, 128-88, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(89, 0);
  tft.print(fahrenheit);
}

void lcdUpdateHumidity()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(51, 16, 128-51, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(52, 16);
  tft.print(String(humidity) + "%");
}

void lcdUpdateWaterLevel()
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(69, 32, 128-69, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(70, 32);
  tft.print(waterLevel);
  tft.setCursor(84, 32);
  tft.print("%");
}

#define MODE_THRESH 1
#define MODE_TIMER 2

void lcdUpdateHumidifier(int mode = 0)
{
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(39, 48, 128-39, 15, ST7735_BLACK); // change to fill black
  tft.setCursor(40, 48);
  if (mode == MODE_THRESH)
  {
    tft.setTextColor(ST7735_YELLOW);
    tft.println("THRESHOLD " + humidityThresholdString + "%");
  }
  else if (mode == MODE_TIMER)
  {
    tft.setTextColor(ST7735_YELLOW);
    tft.println("TIMER " + String(humidifierTimerMinutes) + " MIN");
  }
  else if (humidifierOn && mode == 0)
  {
    tft.setTextColor(ST7735_GREEN);
    tft.println("ON");    
  }
  else
  {
    tft.setTextColor(ST7735_RED);
    tft.println("OFF");
  }
}

/* 
*
* LCD
*
*/

/*
*
* Water Sensor
*
*/

enum WATER_STATES {WATER_INIT};

int waterTick(int state)
{
  switch(state)
  {
    case WATER_INIT:

      lastSeenWater = waterLevel;
      waterLevel = map(analogRead(WATER_PIN), 20, 700, 0, 100);
      if (lastSeenWater != waterLevel) lcdUpdateWaterLevel();
  }
  return state;
}

/*
*
* Water Sensor
*
*/

/*
*
* Humidifier
*
*/

enum HUM_STATES {HUM_INIT, HUM_OFF, HUM_ON, HUM_THRESH, HUM_TIMER};

int humTick(int state)
{
  switch(state)
  {
    case HUM_INIT:
      break;
    case HUM_OFF:
      humidifierOn = false; 
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;        
      break;
    case HUM_ON:
      humidifierOn = true;
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;
      break;
    case HUM_THRESH:
      if (humidity >= humidityThreshold)
      {
        humidifierOn = false;
        digitalWrite(RELAY_PIN, HIGH);
      }
      else 
      {
        lastSeenTime = millis();
        humidifierOn = true;
        digitalWrite(RELAY_PIN, LOW);
      }
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;
      break;
    case HUM_TIMER:
      humidifierOn = true;
      
      if (millis() - minuteTimer > 60000) 
      {
        //Serial.println("I am here, millis value: " + String(millis()) + " minuteTimer value: " + String(minuteTimer));
        minuteTimer = millis();
        humidifierTimerMinutes--;
        lcdUpdateHumidifier(MODE_TIMER);   
      }
      
      if (digitalRead(BUTTON_PIN) == LOW) humidifierButtonHeld = false;
      break;
  }
  switch(state)
  {
    case HUM_INIT:
      state = HUM_OFF;
      break;
    case HUM_OFF:
      if ((digitalRead(BUTTON_PIN) && !humidifierButtonHeld) || powerButtonFlag)
      {
        if (powerButtonFlag) powerButtonFlag = false;
        humidifierButtonHeld = true;
        state = HUM_ON;
        digitalWrite(RELAY_PIN, HIGH);
        lcdUpdateHumidifier();
      }
      else if (switchThresh)
      {
        switchThresh = false;
        lastSeenTime = 0;
        state = HUM_THRESH;
        lcdUpdateHumidifier(MODE_THRESH);
      }
      else if (switchTimer)
      {
        switchTimer = false;
        lastSeenTime = millis();
        minuteTimer = millis();
        humidifierTimer = humidifierTimerMinutes * 60000;        
        state = HUM_TIMER;
        digitalWrite(RELAY_PIN, LOW);
        lcdUpdateHumidifier(MODE_TIMER);
      }
      break;
    case HUM_ON:
      if ((digitalRead(BUTTON_PIN) && !humidifierButtonHeld) || powerButtonFlag)
      {
        if (powerButtonFlag) powerButtonFlag = false;
        humidifierButtonHeld = true;
        state = HUM_OFF;
        digitalWrite(RELAY_PIN, LOW);
        lcdUpdateHumidifier();
      }
      else if (switchThresh)
      {
        switchThresh = false;
        lastSeenTime = 0;
        state = HUM_THRESH;
        lcdUpdateHumidifier(MODE_THRESH);
      }
      else if (switchTimer)
      {
        switchTimer = false;
        lastSeenTime = millis();
        minuteTimer = millis();
        humidifierTimer = humidifierTimerMinutes * 60000;        
        state = HUM_TIMER;
        digitalWrite(RELAY_PIN, LOW);
        lcdUpdateHumidifier(MODE_TIMER);
      }
      break;
    case HUM_THRESH:
      if ((digitalRead(BUTTON_PIN) && !humidifierButtonHeld) || powerButtonFlag)
        {
        if (powerButtonFlag) powerButtonFlag = false;
          humidifierOn = false;
          humidifierButtonHeld = true;
          state = HUM_OFF;
          digitalWrite(RELAY_PIN, HIGH);
          lcdUpdateHumidifier();
        }
        else if (switchThresh)
        {
          switchThresh = false;
          lastSeenTime = 0;
          state = HUM_THRESH;
          lcdUpdateHumidifier(MODE_THRESH);

        }
        else if (switchTimer)
        {
          switchTimer = false;
          lastSeenTime = millis();
          minuteTimer = millis();
          humidifierTimer = humidifierTimerMinutes * 60000;        
          state = HUM_TIMER;
          digitalWrite(RELAY_PIN, LOW);
          lcdUpdateHumidifier(MODE_TIMER);
        }
      break;
    case HUM_TIMER:
      if (millis() - lastSeenTime >= humidifierTimer) 
      {
        humidifierOn = false;
        humidifierButtonHeld = true;
        state = HUM_OFF;
        digitalWrite(RELAY_PIN, HIGH);
        lcdUpdateHumidifier();
      }
      else if ((digitalRead(BUTTON_PIN) && !humidifierButtonHeld) || powerButtonFlag)
      {
        if (powerButtonFlag) powerButtonFlag = false;
        humidifierOn = false;
        humidifierButtonHeld = true;
        state = HUM_OFF;
        digitalWrite(RELAY_PIN, HIGH);
        lcdUpdateHumidifier();
      }
      else if (switchThresh)
      {
        switchThresh = false;
        lastSeenTime = 0;
        state = HUM_THRESH;
        lcdUpdateHumidifier(MODE_THRESH);
      }
      else if (switchTimer)
      {
        switchTimer = false;
        lastSeenTime = millis();
        minuteTimer = millis();
        humidifierTimer = humidifierTimerMinutes * 60000;        
        state = HUM_TIMER;
        digitalWrite(RELAY_PIN, LOW);
        lcdUpdateHumidifier(MODE_TIMER);
      }
      break;
  }
  return state;
}

/*
*
* IR
*
*/

// Samsung TV remote

enum IR_STATES {IR_INIT, IR_WAIT, IR_READ_THRESH, IR_READ_TIMER};

int irTick(int state)
{
  switch(state)
  {
    case IR_INIT:
      break;
    case IR_WAIT:
      break;
    case IR_READ_THRESH:
      if (irrecv.decode())
      {
        switch(irrecv.decodedIRData.decodedRawData)
        {
          case IR0:
            humidityThresholdString += "0";
            break;          
          case IR1:
            humidityThresholdString += "1";
            break;
          case IR2:
            humidityThresholdString += "2";
            break;
          case IR3:
            humidityThresholdString += "3";
            break;
          case IR4:
            humidityThresholdString += "4";
            break;
          case IR5:
            humidityThresholdString += "5";
            break;
          case IR6:
            humidityThresholdString += "6";
            break;
          case IR7:
            humidityThresholdString += "7";
            break;
          case IR8:
            humidityThresholdString += "8";
            break;
          case IR9:
            humidityThresholdString += "9";
            break;
          default:
            humidityThresholdString += "1";
            break;
        }
        humidityThreshold = humidityThresholdString.toInt();
        irrecv.resume();

      }

      break;
    case IR_READ_TIMER:
      if (irrecv.decode())
      {
        switch(irrecv.decodedIRData.decodedRawData)
        {
          case IR0:
            humidifierTimerString += "0";
            break;          
          case IR1:
            humidifierTimerString += "1";
            break;
          case IR2:
            humidifierTimerString += "2";
            break;
          case IR3:
            humidifierTimerString += "3";
            break;
          case IR4:
            humidifierTimerString += "4";
            break;
          case IR5:
            humidifierTimerString += "5";
            break;
          case IR6:
            humidifierTimerString += "6";
            break;
          case IR7:
            humidifierTimerString += "7";
            break;
          case IR8:
            humidifierTimerString += "8";
            break;
          case IR9:
            humidifierTimerString += "9";
            break;
          default:
            humidifierTimerString += "1";
            break;        
        }
        humidifierTimerMinutes = humidifierTimerString.toInt();
        irrecv.resume();
      }
      
      break;
  }

  switch(state)
  {
    case IR_INIT:
      irrecv.enableIRIn();
      state = IR_WAIT;
      break;
    case IR_WAIT:
      
      if (irrecv.decode()) 
      {
        if (irrecv.decodedIRData.decodedRawData == thresholdModeButton)
        {
          humidityThresholdString = "";
          state = IR_READ_THRESH;
        }
        else if (irrecv.decodedIRData.decodedRawData == timerModeButton)
        {
          humidifierTimerString = "";
          state = IR_READ_TIMER;
        }
        else if (irrecv.decodedIRData.decodedRawData == powerButton)
        {
          powerButtonFlag = true;
        }
      }
      irrecv.resume();
      break;
    case IR_READ_THRESH:
      if (humidityThresholdString.length() == 2) 
      {
        switchThresh = true;
        irrecv.end();
        state = IR_INIT;
      }
      break;
    case IR_READ_TIMER:
      if (humidifierTimerString.length() == 3) 
      {
        switchTimer = true;
        irrecv.end();
        state = IR_INIT;
      }
      break;
  }
  return state;
}

/*
*
* IR
*
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  initializeScreen();
  irrecv.blink13(true);

  unsigned char i = 0;
  tasks[i].state = DHT_INIT;
  tasks[i].period = 1000;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &dhtTick;
  i++;

  tasks[i].state = HUM_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &humTick;
  i++;

  tasks[i].state = WATER_INIT;
  tasks[i].period = 2000;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &waterTick;
  i++;

  tasks[i].state = IR_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &irTick;  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  unsigned char i;
  for (i = 0; i < numTasks; ++i)
  {
    if ((millis() - tasks[i].elapsedTime) >= tasks[i].period)
    {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis();
    }
  }
  
  
}
