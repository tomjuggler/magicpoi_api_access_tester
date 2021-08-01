/*********
  Magic Poi API Tester
  See circusscientist.com for details 
  Get images from magicpoi.circusscientist.com and display them on POV poi
  *********/

#include <Arduino.h>
#include "ESP_WiFiManager.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <LittleFS.h>
#include <FastLED.h>

#define LED 2
boolean gotBin = false;

String lastVal;
String allVals[20];
ESP8266WiFiMulti WiFiMulti;

boolean gotFiles = false;
// File fsUploadFile;

//fastled setup:
int newBrightness = 22; //setting 220 for battery and so white is not too much! //20 for testing ok
#define DATA_PIN D2     //D2 for D1Mini, 2 for ESP-01
#define CLOCK_PIN D1    //D1 for D1Mini, 0 for ESP-01
int cnti = 0;
File a;
int imageToUse = 0;
// #define NUM_LEDS 37
#define NUM_LEDS 73

// Define the array of leds
CRGB leds[NUM_LEDS];

// #define NUM_PX 36
#define NUM_PX 72
//trying a really large array, since we have space now for it:
const int maxPX = 20736; //enough for 72x288 or 36x576

//lets try using a maximum number of pixels so very large array to hold any number:
uint8_t message1Data[maxPX]; //this is much larger than our image

int pxDown = NUM_PX;

int pxAcross = pxDown;     //this will change with the image
byte packetBuffer[NUM_PX]; //buffer to hold incoming packet
String bin = "/test.txt";  //test bin file

//function prototypes:
void listDir(const char *dirname);
void uploadFile(const char *filename);
void getBin(String word, int counter);
void showLittleFSImage(int imgNum);

void showLittleFSImage(int imgNum) //todo: get bin here as argument
{
  bin = "/" + String(imgNum) + ".bin";
  a = LittleFS.open(bin, "r"); 

  if (!a)
  {
    FastLED.showColor(CRGB::Blue);
    Serial.println("Code Blue - no file found!");
  }
  else
  {
    size_t size = a.size();
    if (size > maxPX)
    {
      FastLED.showColor(CRGB::Blue); //error indicator
      imageToUse++;
    }
    else
    {
      a.read(message1Data, size);
      cnti++;
      if (cnti >= pxDown)
      {
        cnti = 0;
      }
      a.close();
    }
  }

  int counter = 0;
  for (int j = 0; j < pxAcross; j++)
  {
    for (int i = 0; i < pxDown; i++)
    {
      byte X;
      X = message1Data[counter++];
      byte R1 = (X & 0xE0);
      leds[i].r = R1; //
      byte G1 = ((X << 3) & 0xE0);
      leds[i].g = G1;
      byte M1 = (X << 6);
      leds[i].b = M1;
      // Serial.print(R1); Serial.print(", "); Serial.print(G1); Serial.print(", "); Serial.println(M1);
    }
    FastLED.show();
    //Todo: check delay(1) effect on 72px poi - definitely needed for 36px
    if (NUM_LEDS < 72)
    {
      FastLED.delay(1); // for 160mhz
    }
    else
    {
      // no delay
    }
  }
  a.close();
}

//listDir from utilities at: https://microcontrollerslab.com/littlefs-introduction-install-esp8266-nodemcu-filesystem-uploader-arduino/
void listDir(const char *dirname)
{
  Serial.printf("Listing directory: %s\n", dirname);

  Dir root = LittleFS.openDir(dirname);

  while (root.next())
  {
    File file = root.openFile("r");
    Serial.print("  FILE: ");
    Serial.print(root.fileName());
    Serial.print("  SIZE: ");
    Serial.print(file.size());
    file.close();
     }
}

void getBin(String word, int counter)
{
  Serial.print("got word: ");
  Serial.println(word);
  String name = String(counter);
  Serial.print("got name");
  Serial.println(name);
  String requester = "http://magicpoi.circusscientist.com/api/output/" + word;
  Serial.print("requester is: ");
  Serial.println(requester); //should this be passed to the function?

  // filename = "/test.txt"; //test image - in case http not working, need to create the correct format file still in /data for this to work
  
  String filename = "/" + name + ".bin";//test test
  Serial.println("got filename: " + filename);
  
  //if file exists, skip download - todo: need to be able to turn this off for updates(one delete is implemented on server)
  if (LittleFS.exists(filename)){
    gotFiles = true;
    return;
  }
  File fsUploadFile = LittleFS.open(filename, "w"); //should create a file on littlefs??? why not?


  WiFiClient client;             //should this be passed to the function?
  HTTPClient http;               //should this be passed to the function?
  http.begin(client, requester); //should work..
  //       http.begin(client, "http://magicpoi.circusscientist.com/api/output/be231c60aae155fda80fce64dd55f47814e66ea.jpg.bin");

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      // get length of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      // create buffer for read
      uint8_t buff[128] = {0};


      // or "by hand"
      // get tcp stream
      WiFiClient *stream = &client;
      // read all data from server
      while (http.connected() && (len > 0 || len == -1))
      {
        // read up to 128 byte
        int c = stream->readBytes(buff, std::min((size_t)len, sizeof(buff)));
        // Serial.printf("readBytes: %d\n", c);
        if (!c)
        {
          Serial.println("read timeout");
        }
        // write it to Serial - and write to littleFS test file:
        if (fsUploadFile)
        {
          fsUploadFile.write(buff, c);
          Serial.write(buff, c); //test show data being saved to LittleFS
        }
        else
        {
          Serial.println("no fsUploadFile");
        }
        if (len > 0)
        {
          len -= c;
        }
      }

      Serial.print("[HTTP] connection closed or file end.\n");
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  listDir("/");
  gotFiles = true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("YOUR-SSID", "YOUR-PASSWORD");
  Serial.println("Mount LittleFS");
  if (!LittleFS.begin())
  {
    Serial.println("LittleFS mount failed");
    return;
  }
  
  //test LittleFS upload was successful:
  listDir("/");
  File f = LittleFS.open("/test.txt", "r");
  if (!f)
  {
    Serial.println("No Saved Data!");
  }
  else
  {
    digitalWrite(LED, HIGH);
    while (f.available())
    {
      String line = f.readStringUntil('\n');
      Serial.println(line);
    }
    f.close();
  }

  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS); 
  FastLED.setBrightness(newBrightness); //should be low figure here, for startup battery saving...
  //FastLED startup indicator:
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Green;
    FastLED.show();
    FastLED.delay(20);
    leds[i] = CRGB::Black;
    FastLED.delay(10);
  }
  FastLED.showColor(CRGB::Black);
}


void loop()
{
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;
    HTTPClient http; //must be declared after WiFiClient for correct destruction order, because used by http.begin(client,...)
    if (!gotFiles)
    {
      if (http.begin(client, "http://magicpoi.circusscientist.com/api/get-filenames"))
      {
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            String payload = http.getString();
            Serial.println(payload);
            // int maxIndex = 2; //test get only one
            int maxIndex = payload.length() - 3; //get all
            int index = 1;                       //start from 1, 0 is "["
            int next_index;
            String data_word;
            // int imgNum = 3; //choose where to stop, ie: limit file downloads
            int counter = 0;
            do
            {
              next_index = payload.indexOf('"', index);
              if (payload.substring(index, next_index) != "" && payload.substring(index, next_index) != ",")
              {
                data_word = payload.substring(index, next_index);
                getBin(data_word, counter); //gets the .bin file data
                counter++;
              }
              index = next_index + 1;
              // if (counter == imgNum)
              // {
              //   break; //stop downloading files at this point
              // }
              if (index == maxIndex)
              {
                gotFiles = true;
              }
            } while ((next_index != -1) && (next_index < maxIndex));
            lastVal = data_word; //todo: need an array
            Serial.print("lastVal is: ");
            Serial.println(lastVal);
            Serial.print("index is: " + index);
          }
        }
        else
        {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      }
      else
      {
        Serial.printf("[HTTP} Unable to connect\n");
      }
    }
    else
    {
      showLittleFSImage(5); //todo: variable for image number to display - goes with web server UI
    }
  }
}
