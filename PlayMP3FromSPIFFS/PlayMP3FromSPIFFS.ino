#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include "SPIFFS.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioOutputMixer.h"

// To run, set your ESP8266 build to 160MHz, and include a SPIFFS of 512KB or greater.
// Use the "Tools->ESP8266/ESP32 Sketch Data Upload" menu to write the MP3 to SPIFFS
// Then upload the sketch normally.  

// pno_cs from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *playbackFile;
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub;
AudioFileSourceID3 *id3;

int connectFileCount = 0;
int holdFileCount = 0;
int messageFileCount = 0;

enum states { CONNECTING, HOLD, MESSAGE };
enum states curState = CONNECTING;

#define bclk_pin 32
#define wclk_pin 25
#define dout_pin 27

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  (void)cbData;
//  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
//    Serial.printf("%c", a);
  }
//  Serial.printf("'\n");
//  Serial.flush();
}


void setup() {
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();

  buildFileCount();
  audioLogger = &Serial;

  out = new AudioOutputI2S();
  out->SetPinout(bclk_pin, wclk_pin, dout_pin);
  out->SetOutputModeMono(true);

  mixer = new AudioOutputMixer(32, out);
  stub = mixer->NewInput();
  stub->SetGain(1.0);

  mp3 = new AudioGeneratorMP3();
}

void buildFileCount() {
  Serial.println("Counting files in SPIFFS...");
  File file;

  File root = SPIFFS.open("/");
  while(file = root.openNextFile()){
//      Serial.print("FILE: ");
//      Serial.println(file.name());

    String str(file.name());
    if (str.startsWith("/connect/")) connectFileCount++;
    else if (str.startsWith("/hold/")) holdFileCount++;
    else if (str.startsWith("/message/")) messageFileCount++;
  }

  Serial.print("Number of connect audio files in SPIFFS: ");
  Serial.println(connectFileCount);
  Serial.print("Number of hold audio files in SPIFFS: ");
  Serial.println(holdFileCount);
  Serial.print("Number of message audio files in SPIFFS: ");
  Serial.println(messageFileCount);
}

String getFileName(String folder, int index) {
//  Serial.print("Index: ");
//  Serial.println(index);
  
  File root = SPIFFS.open("/");
  String filename;
  while(index >= 0){
      File file = root.openNextFile();
//      Serial.println(file.name());
      filename = file.name();
      if (filename.startsWith(folder)) index--;
  }

//  Serial.println(filename);
  return filename;
}

void loop() {
  if (mp3->isRunning()) { // finish playing ongoing mp3
    if (!mp3->loop()){
      mp3->stop();
      stub->stop();
      Serial.println("Playback complete.");
      if (curState == CONNECTING) curState = HOLD;
      else if (curState == HOLD) curState = MESSAGE;
      else if (curState == MESSAGE) curState = CONNECTING;
    }
  }
  else { // select a random new file to play
    String filename = getFileName("/connect/", random(connectFileCount));
    if (curState == HOLD) filename = getFileName("/hold/", random(holdFileCount));
    if (curState == MESSAGE) filename = getFileName("/message/", random(messageFileCount));
    
    Serial.print("Playing new file: " + filename + "...");
    int str_len = filename.length() + 1; 
    char char_array[str_len];
    filename.toCharArray(char_array, str_len);
    
    playbackFile = new AudioFileSourceSPIFFS(char_array);
    id3 = new AudioFileSourceID3(playbackFile);
    id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
    
    mp3->begin(id3, stub);
  }
}
