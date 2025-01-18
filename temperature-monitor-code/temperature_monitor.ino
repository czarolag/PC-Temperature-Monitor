// basic libraries
#include <Arduino.h>
#include <ArduinoJson.h>

// for WiFi processes
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
// instead of hardcoding network info we have to connect to the
// feather network and give it the credentials using WiFiManager

#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// for the display [ST7796 Driver]

//TFT_eSPI requires pins to be setup on the User_Setup.h file in library folder
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

// local network to which the PC is connected to to send updates
const char* server = "http://192.168.1.238:5200";

// Initialize TFT object
TFT_eSPI tft = TFT_eSPI(); 


// Global variable to store the previous state
struct DisplayState {
  String cpuName;
  float maxCpuTemp;
  float maxCpuLoad;
  int memoryLoad;
  float cpuSpeed;
  float cpuPower;
} prevState;

bool stateChanged(const DisplayState& newState) {
  return newState.cpuName != prevState.cpuName ||
         newState.maxCpuTemp != prevState.maxCpuTemp ||
         newState.maxCpuLoad != prevState.maxCpuLoad ||
         newState.memoryLoad != prevState.memoryLoad ||
         newState.cpuSpeed != prevState.cpuSpeed ||
         newState.cpuPower != prevState.cpuPower;
}


void parseJSON(const String& json) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("JSON Parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  DisplayState newState;
  newState.cpuName = String(doc["CpuInfo"]["CPUName"]);
  newState.maxCpuTemp = -1;
  for (int i = 0; i < 8; i++) {
    float temp = doc["CpuInfo"]["fTemp"][i];
    if (temp > newState.maxCpuTemp) newState.maxCpuTemp = temp;
  }
  newState.maxCpuLoad = -1;
  for (int i = 0; i < 8; i++) {
    float load = doc["CpuInfo"]["uiLoad"][i];
    if (load > newState.maxCpuLoad) newState.maxCpuLoad = load;
  }
  newState.memoryLoad = doc["MemoryInfo"]["MemoryLoad"];
  newState.cpuSpeed = doc["CpuInfo"]["fCPUSpeed"];
  newState.cpuPower = doc["CpuInfo"]["fPower"][0];

  display_info(newState);
}


void setup() {
  WiFiManager wm;
  tft.begin();
  tft.setRotation(1);

  Serial.begin(115200);
  tft.fillScreen(TFT_BLACK);

  Serial.println();
  Serial.println();
  Serial.println();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.print("Connect to WiFi. czar_temp:czarberry");

  bool res;
  res = wm.autoConnect("czar_temp","czarberry"); // password protected ap

  tft.fillScreen(TFT_BLACK);

  if(!res) {
      Serial.println("Failed to connect");
      tft.setCursor(10, 10);
      tft.print("Failed to Connect!");
  } 
  else {
      Serial.println("connected...yeey :)");
      tft.setCursor(10, 10);
      tft.print("Connected. YIPEEEE");
  }
  delay(1000);
  tft.fillScreen(TFT_BLACK);
}


void loop() {
  WiFiClient client;
  HTTPClient http;

  if (client.connect("192.168.1.238", 5200)) {

    tft.fillScreen(TFT_BLACK);
    
    client.println("GET / HTTP/1.1");
    client.println("Host: 192.168.1.238");
    client.println("Connection: close");
    client.println();  // End of headers

    // Wait for server response
    Serial.println("Waiting for response...");
    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
        parseJSON(line);
        delay(500);
      }
    }
    tft.fillScreen(TFT_BLACK);
  } else {
    Serial.println("Server not reachable!");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.print("Server not reachable!");
  }
  delay(1000);
}


// Function to display system information
void display_info(const DisplayState& newState) {
  // Update only if data has changed
  if (!stateChanged(newState)) return;
  
  prevState = newState; // Save new state as previous

  // CPU Name
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("CPU: ");
  tft.print(newState.cpuName);

  // CPU Speed
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 60);
  tft.print("CPU Speed: ");
  tft.print(newState.cpuSpeed);
  tft.print(" MHz");

  // CPU Power
  tft.fillRect(10, 110, 300, 20, TFT_BLACK);
  tft.setCursor(10, 110);
  tft.print("CPU Power: ");
  tft.print(newState.cpuPower);
  tft.print(" W");



  // start meter pos
  int spacing = 15;
  int xpos = 15, ypos = 380/2- 10, radius = 70;

  // memory load meter
  xpos = ringMeter(newState.maxCpuLoad, 0, 100, xpos, ypos, radius, "CPU%", 5/*RED2GREEN*/) + spacing;

  // cpu load meter
  xpos = ringMeter(newState.memoryLoad, 0, 100, xpos, ypos, radius, "Mem%", 3/*BLUE2RED*/) + spacing;

  // cpu temp meter
  xpos = ringMeter(newState.maxCpuTemp, 0, 90, xpos, ypos, radius, "Temp (C)", 4/*GREEN2RED*/) + spacing;

}


// #########################################################################
//  Draw the meter on the screen, returns x coord of righthand side
// #########################################################################
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring

  int w = r / 3;    // Width of outer ring is 1/4 of radius
  
  int angle = 150;  // Half the sweep angle of meter (300 degrees)

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Variable to save "value" text colour from scheme and set default
  int colour = TFT_BLUE;
 
  // Draw colour blocks every inc degrees
  for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = TFT_RED; break; // Fixed colour
        case 1: colour = TFT_GREEN; break; // Fixed colour
        case 2: colour = TFT_BLUE; break; // Fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc.)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc.)
        default: colour = TFT_BLUE; break; // Fixed colour
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      //text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_BLACK);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_BLACK);
    }
  }
  // Convert value to a string
  char buf[10];
  byte len = 3; if (value > 999) len = 5;
  dtostrf(value, len, 0, buf);
  buf[len] = ' '; buf[len+1] = 0; // Add blanking space and terminator, helps to centre text too!
  // Set the text colour to default
  tft.setTextSize(1);

/*
  if (value<vmin || value>vmax) {
    drawAlert(x,y+90,50,1);
  }
  else {
    drawAlert(x,y+90,50,0);
  }
*/

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  tft.setTextColor(colour, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  // Print value, if the meter is large then use big font 8, othewise use 4
  if (r > 84) {
    tft.setTextPadding(55*3); // Allow for 3 digits each 55 pixels wide
    tft.drawString(buf, x, y, 8); // Value in middle
  }
  else {
    tft.setTextPadding(3 * 14); // Allow for 3 digits each 14 pixels wide
    tft.drawString(buf, x, y, 4); // Value in middle
  }
  tft.setTextSize(1);
  tft.setTextPadding(0);
  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (r > 84) tft.drawString(units, x, y + 60, 4); // Units display
  else tft.drawString(units, x, y + 15, 2); // Units display

  // Calculate and return right hand side x coordinate
  return x + r;
}


// #########################################################################
// Return a 16-bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16-bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


// #########################################################################
// Return a value in range -1 to +1 for a given phase angle in degrees
// #########################################################################
float sineWave(int phase) {
  return sin(phase * 0.0174532925);
}
