//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time betweew 1 and 5 seconds
// 2. prints anything it recieves to Serial.print
//
//
//************************************************************
#include "painlessMesh.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <console.h>
#include <painlessMesh.h>
#include <painlessMeshSync.h>
#include <painlessScheduler.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
#define   BTN             12
#define   LED             16

//*************************
#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);
const int type = 2;

int firePin = 0;
int doorPin = 2;
int isFire = 0;
int isDoor = 0;

int reading;
float voltage;
float tempF;
int tempFint;

int upPin = 14;
int downPin = 12;
int idealTemp = 65;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//*************************

void sendMessage() ; // Prototype so PlatformIO doesn't complain

painlessMesh  mesh;
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

// Variables for my bells and whistles
bool myLight = false;
bool yourLight = false;

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& myRequest = jsonBuffer.parseObject(msg);
  myLight = int(myRequest["ledState"]);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
    Serial.printf("Changed connections %s\n",mesh.subConnectionJson().c_str());
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(BTN, INPUT);
  attachInterrupt(BTN, changeLight, RISING);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  mesh.scheduler.addTask( taskSendMessage );
  taskSendMessage.enable() ;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  pinMode(firePin,OUTPUT);
  pinMode(doorPin,OUTPUT);
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(upPin),upTemp,FALLING);
  attachInterrupt(digitalPinToInterrupt(downPin),downTemp,FALLING);
}

void changeLight(){
  yourLight = !yourLight;
  return;
}

void loop() {
  mesh.update();
  digitalWrite(LED, myLight);
  int tempF = getTemp();
    
    //Serial.println("Temp F");
    //Serial.println(tempF);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println(idealTemp);
    display.setTextSize(6);
    //display.print(" ");
    display.print(tempF);
    display.display();
}

void upTemp()
{
  //Serial.println("UpTemp called");
  idealTemp++;
}
void downTemp()
{
  idealTemp--;
}

int getTemp()
{
    reading = analogRead(A0);
    /*
    Serial.print("***");
    Serial.println(reading);
    Serial.println("mv");
    Serial.println(idealTemp);
    Serial.println("ideal Temp");
    */
    float volt = reading * 3.0;
    //float volt = reading;
    volt /= 1024.0;
    float tempC = (volt - 0.5)*100;

    int tempF = tempC * 9.0/5.0 +32.0;
    //Serial.println("Temp F");
    return tempF;
}

void sendMessage() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& myRequest = jsonBuffer.createObject();
  myRequest["requestTemp"] = idealTemp;
  myRequest["data"] = getTemp();
  myRequest["device_id"] = ESP.getChipId();
  myRequest["type"] = 2;
  myRequest["device_name"] = "Heng's twin tower";
  String request;
  myRequest.printTo(request);
  Serial.print("Sending: ");
  Serial.println(request);
  mesh.sendBroadcast( request );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}
