#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

#define I2S_DOUT      27
#define I2S_BCLK      32
#define I2S_LRC       25

#define CONNECT_FOLDER "/connect"
#define HOLD_FOLDER "/hold"
#define MESSAGE_FOLDER "/message"

#define SWITCH_PIN 35
#define GND_PIN 33

Audio audio;

int connectFileCount = 0;
int holdFileCount = 0;
int messageFileCount = 0;

int prevSwitch = 0;

enum states { CONNECTING, HOLD, MESSAGE, WAITING };
enum states curState = CONNECTING;

void setup() {
    pinMode(GND_PIN, OUTPUT); digitalWrite(GND_PIN, HIGH);
    pinMode(SWITCH_PIN, INPUT);
    prevSwitch = analogRead(SWITCH_PIN);
    
    pinMode(SD_CS, OUTPUT);      digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    Serial.begin(115200);
    SD.begin(SD_CS);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // 0...21
    
    buildFileCount();

    curState = CONNECTING;
    audio.connecttoFS(SD, getFullFilePath(CONNECT_FOLDER, random(connectFileCount)));
}

void loop() {
//  Serial.println(analogRead(SWITCH_PIN));
  int newSwitch = analogRead(SWITCH_PIN);
  if (prevSwitch < 3000 && newSwitch > 3000) { // phone was just taken off hook
    delay(1000);
    curState = CONNECTING;
    audio.connecttoFS(SD, getFullFilePath(CONNECT_FOLDER, random(connectFileCount)));
  }
  else if (newSwitch > 3000){ // phone off the hook
    audio.loop();
  }
  else { // phone on the hook
    // nothing?
  }
  prevSwitch = newSwitch;
}

void buildFileCount() {
  Serial.println("Counting files in SD card...");
  File file;

  File root = SD.open(CONNECT_FOLDER);
  while(file = root.openNextFile()){
//      Serial.print("FILE: ");
//      Serial.println(file.name());
    connectFileCount++;
  }

  root = SD.open(HOLD_FOLDER);
  while(file = root.openNextFile()){
//      Serial.print("FILE: ");
//      Serial.println(file.name());
    holdFileCount++;
  }

  root = SD.open(MESSAGE_FOLDER);
  while(file = root.openNextFile()){
//      Serial.print("FILE: ");
//      Serial.println(file.name());
    messageFileCount++;
  }

  Serial.print("Number of connect audio files in SPIFFS: ");
  Serial.println(connectFileCount);
  Serial.print("Number of hold audio files in SPIFFS: ");
  Serial.println(holdFileCount);
  Serial.print("Number of message audio files in SPIFFS: ");
  Serial.println(messageFileCount);
}

String getFullFilePath(String folder, int index) {
//  Serial.print("Index: ");
//  Serial.println(index);
  
  File root = SD.open(folder);
  String filename;
  while(index >= 0){
      File file = root.openNextFile();
      filename = file.name();
//      Serial.println(file.name());
      index--;
  }

//  Serial.println(filename);
  return filename;
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}

void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);

    // transition state
    if (curState == CONNECTING) curState = HOLD;
    else if (curState == HOLD) curState = MESSAGE;
    else if (curState == MESSAGE) curState = WAITING;

    // pick the next file
    String filename = "";
    if (curState == CONNECTING) filename = getFullFilePath(CONNECT_FOLDER, random(connectFileCount));
    if (curState == HOLD) filename = getFullFilePath(HOLD_FOLDER, random(holdFileCount));
    if (curState == MESSAGE) filename = getFullFilePath(MESSAGE_FOLDER, random(messageFileCount));

    // start playback
    if (filename != "") audio.connecttoFS(SD, filename);
}
