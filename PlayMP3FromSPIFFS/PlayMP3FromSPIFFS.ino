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

// To run, set your ESP8266 build to 160MHz, and include a SPIFFS of 512KB or greater.
// Use the "Tools->ESP8266/ESP32 Sketch Data Upload" menu to write the MP3 to SPIFFS
// Then upload the sketch normally.  

// pno_cs from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *playbackFile;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

int fileCount;


#define bclk_pin 32
#define wclk_pin 25
#define dout_pin 27

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}


void setup()
{
  WiFi.mode(WIFI_OFF); 
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();

  buildFileArray();
  
  Serial.printf("Sample MP3 playback begins...\n");
  audioLogger = &Serial;
  playbackFile = new AudioFileSourceSPIFFS("/pno-cs.mp3");
  id3 = new AudioFileSourceID3(playbackFile);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  out = new AudioOutputI2S();
  out->SetPinout(bclk_pin, wclk_pin, dout_pin);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
}

void buildFileArray(){
  fileCount = 0;
  Serial.println("Listing files in SPIFFS...");

  File root = SPIFFS.open("/");
  File file;
  
  while(file = root.openNextFile()){
      fileCount++;
      Serial.print("FILE: ");
      Serial.println(file.name());
  }

  Serial.print("Number of audio files in SPIFFS: ");
  Serial.println(fileCount);
}

void loop()
{
  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
  }
}
