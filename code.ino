//yes i forgot to use git again pls forgive me i just dont like setting up repos 
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

const char* ssid = "ARRIS_59D3_5G-1";
const char* pass = "You thought lmao";

#define TFT_SCLK 9
#define TFT_MOSI 10
#define TFT_RST 8
#define TFT_DC 4
#define TFT_CS 5
#define TFT_BL 6

//reassign this when i get parts 
#define BUTTON_ADD 2
#define BUTTON_SUBTRACT 3
#define BUTTON_MODE 0
#define BUTTON_ACCEPT 1

#define BUZZER_PIN 7

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


//TODO: Switch this to enum so it actually looks good and i can log
int currentMode = 0;
int selectedMode = 0;
bool modeSelecting = false;

int alarmSubMode = 0;
int alarmHours[10];
int alarmMins[10];
int alarmCount = 0;
int tempHour = 12;
int tempMin = 0;
int deleteIdx = 0;

bool alarmRinging = false;

bool swRunning = false;
unsigned long swStartTime = 0;
unsigned long swElapsedTime = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(BUTTON_ADD, INPUT_PULLUP);
  pinMode(BUTTON_SUBTRACT, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_ACCEPT, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init(76, 284); // Our panel size (portrait)
  tft.setOffsets(82, 18); // Offsets for the weird resolution
  tft.invertDisplay(false); // Invert the colors (This display is flipped from normal)
  tft.setRotation(1); // Landscape, if it's upside down use 3!
  tft.fillScreen(ST77XX_BLACK); // clear the screen
  Serial.println("TFT Initialized!");
  tft.setCursor(0,0); // make the cursor at the top left

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
  //evb else uses ntp time to use it too
  //thank you to whoever made that bee alarm clock for this idea
  //AI was used to understand how this works because i have never used wifi before on this
  configTime(-18000, 3600, "pool.ntp.org");
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  // put your main code here, to run repeatedly:
  //according to gemini i have to make it low because apparently since my button is wired to gpio and gnd, pullup mode is what i need. this is counterintuitive. had to use ai on this sorry
  bool addPressed = digitalRead(BUTTON_ADD) == LOW;
  bool subPressed = digitalRead(BUTTON_SUBTRACT) == LOW;
  bool modePressed = digitalRead(BUTTON_MODE) == LOW;
  bool acceptPressed = digitalRead(BUTTON_ACCEPT) == LOW;

  if (alarmRinging) {
    digitalWrite(BUZZER_PIN, HIGH);
    if (acceptPressed || addPressed || subPressed) {
      alarmRinging = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  //Here was my ui idea. This is for u reviewer hope this makes code more  sensible
  //Basically, i wanted a couple modes on alarm and storage
  //So i made a mode button, this is based off my casio watch
  //If you click accept, you enter that mode and then you can edit stuff
  // It should hopfeully be a list of items, but i bet its gonna not work
  // Please gimme kit so i can fix it hack club
  if (modePressed) {
    if (!modeSelecting) {
      modeSelecting = true;
      selectedMode = currentMode;
    }
    selectedMode = (selectedMode + 1) % 3;
    delay(200);


  } else if (acceptPressed && modeSelecting) {
    currentMode = selectedMode;
    modeSelecting = false;
    alarmSubMode = 0;
    tft.fillScreen(ST77XX_BLACK);
    delay(200);


  } else if (!modeSelecting) {
    if (currentMode == 1) {
      if (alarmSubMode == 0) {
        if (addPressed && alarmCount < 10) {
          alarmSubMode = 1;
          tempHour = 12;
          tempMin = 0;
          tft.fillScreen(ST77XX_BLACK);
          delay(200);
        } else if (subPressed && alarmCount > 0) {
          alarmSubMode = 2;
          deleteIdx = 0;
          tft.fillScreen(ST77XX_BLACK);
          delay(200);
        }
      } else if (alarmSubMode == 1) {
        if (addPressed) {
          tempMin = (tempMin + 5) % 60;
          if (tempMin == 0) tempHour = (tempHour + 1) % 24;
          delay(200);
        } else if (subPressed) {
          tempMin = (tempMin - 5 + 60) % 60;
          if (tempMin == 55) tempHour = (tempHour - 1 + 24) % 24;
          delay(200);
        } else if (acceptPressed) {
          alarmHours[alarmCount] = tempHour;
          alarmMins[alarmCount] = tempMin;
          alarmCount++;
          alarmSubMode = 0;
          tft.fillScreen(ST77XX_BLACK);
          delay(200);
        }
      } else if (alarmSubMode == 2) {
        if (addPressed) {
          deleteIdx = (deleteIdx + 1) % alarmCount;
          delay(200);
        } else if (subPressed) {
          deleteIdx = (deleteIdx - 1 + alarmCount) % alarmCount;
          delay(200);
        } else if (acceptPressed) {
          for (int i = deleteIdx; i < alarmCount - 1; i++) {
            // im genuinely praying the delection works, too many entries for that tiny display
            alarmHours[i] = alarmHours[i + 1];
            alarmMins[i] = alarmMins[i + 1];
          }
          alarmCount--;
          alarmSubMode = 0;
          tft.fillScreen(ST77XX_BLACK);
          delay(200);
        }
      }
    } else if (currentMode == 2) {
      if (acceptPressed) {
        if (swRunning) {
          swRunning = false;
          swElapsedTime += (millis() - swStartTime);
        } else {
          swRunning = true;
          swStartTime = millis();
        }
        delay(200);
      } else if (addPressed) {
        swElapsedTime += 30000;
        delay(200);
      } else if (subPressed) {
        if (swElapsedTime >= 30000) swElapsedTime -= 30000;
        else swElapsedTime = 0;
        delay(200);
      }
    }
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo) && timeinfo.tm_sec == 0 && !alarmRinging) {
    for (int i = 0; i < alarmCount; i++) {
      if (alarmHours[i] == timeinfo.tm_hour && alarmMins[i] == timeinfo.tm_min) {
        alarmRinging = true;
        break;
      }
    }
  }

  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);

  if (modeSelecting) {
    tft.print("Mode: ");
    if (selectedMode == 0) tft.print("CLOCK    ");
    if (selectedMode == 1) tft.print("ALARMS   ");
    if (selectedMode == 2) tft.print("STOPWATCH");
  } else if (currentMode == 0) {
    if (getLocalTime(&timeinfo)) {
      tft.print(timeinfo.tm_hour);tft.print(":");tft.print(timeinfo.tm_min);tft.print(":");tft.print(timeinfo.tm_sec);
    }
  } else if (currentMode == 1) {
    if (alarmSubMode == 0) {
       tft.print("Alarms: ");
      tft.println(alarmCount);
      for (int i = 0; i < alarmCount; i++) {
        tft.print(i + 1);tft.print(") ");tft.print(alarmHours[i]);tft.print(":");tft.println(alarmMins[i]);
      }
    } else if (alarmSubMode == 1) {
      tft.print("Add: ");tft.print(tempHour);tft.print(":");tft.print(tempMin);
    } else if (alarmSubMode == 2) {
      tft.print("Del: ");tft.print(alarmHours[deleteIdx]);tft.print(":");tft.print(alarmMins[deleteIdx]);
    }
  } else if (currentMode == 2) {
    unsigned long total = swElapsedTime + (swRunning ? (millis() - swStartTime) : 0);
    tft.print("Del: ");tft.print(alarmHours[deleteIdx]);tft.print(":");tft.print(alarmMins[deleteIdx]);
  }                 
}            
                              
//beep boop
void buzzerBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
}        
