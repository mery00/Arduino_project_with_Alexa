#include <PubSubClient.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <BH1750.h>
#include <Servo.h>
#include "arduino_secrets.h"

#define PIN 10
#define LEDS 1

Servo servo;
BH1750 lightMeter;
WiFiClient esp;
PubSubClient client_mqtt(esp);

int angle=0;
int pin=8;
int ints=0;
int angle_curr=0;
int progress=0;
const char* mqttUser= "";//insert the username take from CloudMqtt
const char* mqttPass="";//insert the password take from CloudMqtt 
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char buff[4];
bool isOpenServo=false;
boolean isLightOn=false;
boolean isFreeMode=false;
boolean isManualMode=true;
boolean isSleepMode=false;
boolean isRegulize=false;
boolean buttonLed=false;
unsigned int previousLight=0;
unsigned long previousMillisFree=0;
unsigned long previousMillis = 0; //will store last time LED was updated
unsigned long interval = 1000; //interval at which to blink (milliseconds)
unsigned long intervalFree=1000;
uint16_t intensity;
uint16_t intensity_normalized;
  


void callback(char* topic, byte* payload, unsigned int length) {
// In order to republish this payload, a copy must be made
// as the orignal payload buffer will be overwritten whilst
// constructing the PUBLISH packet.
Serial.println("Incoming data : ");
Serial.println(topic);
//Serial.println(payload);
// Allocate the correct amount of memory for the payload copy
byte* p = (byte*)malloc(length);
char* resp = (char*)malloc(length+1);

// Copy the payload to the new buffer
memcpy(p,payload,length);
for(int i=0; i<=length;i++){
    if(i==length) resp[i]='\0';
    else resp[i]=(char)p[i];
  }

if(strcmp(resp,"ON")==0){
  if (isFreeMode) sendMqttMessage("log","Ehi,dude! you're in free mode! you can't do that");
  else lightON();
}
else if(strcmp(resp,"OFF")==0){
  if (isFreeMode) sendMqttMessage("log","Ehi,dude! you're in free mode! you can't do that");
  else lightOFF();

}
else if(strcmp(resp,"più")==0){ 
  if(isFreeMode) sendMqttMessage("log", "Ehi,dude! you're in free mode! you can't do that");
  else MoreLight();
  
}
else if(strcmp(resp,"meno")==0){
  if(isFreeMode) sendMqttMessage("log", "Ehi,dude! you're in free mode! you can't do that");
  else LessLight();
}
else if(strcmp(resp,"manual")==0) setupMode(resp);
else if(strcmp(resp,"sleep")==0) setupMode(resp);
else if(strcmp(resp,"free")==0) setupMode(resp);

if(strcmp(resp,"piu_servo")==0) servoMore();
else if(strcmp(resp,"meno_servo")==0) servoLess();

if(strcmp(resp,"ON_W")==0) openServo();
else if (strcmp(resp,"OFF_W")==0) closeServo();

// Free the memory
free(p);
free(resp);
}


void wifiConnect(){
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println("You're connected to the network");
  mqttConnect();
  Serial.println();
}

void mqttConnect(){
  client_mqtt.setServer("",);// //insert the server take from CloudMqtt and the port
  client_mqtt.setCallback(callback);
  client_mqtt.disconnect();

  while (!client_mqtt.connected()) {      
        Serial.println("Connecting to MQTT...");
        if (client_mqtt.connect("Arduino_client_mqtt", mqttUser, mqttPass )) {
          Serial.println("Connected to mqtt server");
          client_mqtt.publish("temp","hello world");
          boolean rc = client_mqtt.subscribe("topic",0);
        } 
        else {
          Serial.print("failed with state ");
          Serial.println(client_mqtt.state());
          delay(2000);
        }
    }
}

void sendMqttMessage(const char* title, const char* text) {
  if(client_mqtt.connected()) {
    client_mqtt.publish(title,text);
  } else {
    mqttConnect();
    client_mqtt.publish(title,text);
  }
}


void openServo(){
  if(!isOpenServo){
  servo.write(180); 
  angle_curr=180;
  progress=100;
  isOpenServo=true;
  sendMqttMessage("servo","100");
  sendMqttMessage("button_W","ON_W");
  }else{
    sendMqttMessage("log","The tent is already open!");  
  }
}

void closeServo(){
  if(isOpenServo){
  servo.write(0); 
  angle_curr=0;
  progress=0;
  isOpenServo=false;
  sendMqttMessage("servo","0");
  sendMqttMessage("button_W","OFF_W");
  }else{
    sendMqttMessage("log","The tent is already close!");  
  }
}

void servoMore(){
   if (isFreeMode){
         sendMqttMessage("log","Ehi Dude, you are in free mode you can't do that!");  
    }
   else if(angle_curr<180){
    if(angle_curr==0){
      sendMqttMessage("button_W","ON_W");
    }
    servo.write(angle_curr+18); 
    angle_curr=angle_curr+18;
    progress=progress+10;
    sendMqttMessage("servo",strcat(itoa(progress,buff,10),"%"));
   }
    else{
      sendMqttMessage("servo","100");
      sendMqttMessage("log", "max reached"); 
    }
  }

void servoLess(){
    if (isFreeMode){
         sendMqttMessage("log","Ehi Dude, you are in free mode you can't do that!");  
    }
    else if (angle_curr>0){
       servo.write(angle_curr-18); 
       angle_curr=angle_curr-18;
       progress=progress-10;
       sendMqttMessage("servo",strcat(itoa(progress,buff,10),"%"));
     }
    else {
      sendMqttMessage("servo","0");
      sendMqttMessage("log", "min reached"); 
    }
    if(angle_curr==0) sendMqttMessage("button_W","OFF_W");
  }
  
void setupMode(char *resp){
    if(strcmp(resp,"manual")==0){
        Serial.print("you're in manual mode");
        isSleepMode=false;
        isFreeMode=false;
        isManualMode=true;
        isRegulize=false;
        interval=5000;
    }else if(strcmp(resp,"sleep")==0){
        Serial.print("you're in sleep mode");
        isManualMode=false;
        isFreeMode=false;
        isSleepMode=true;
        isRegulize=false;
        sendMqttMessage("servo","0");
        sendMqttMessage("button_led","OFF");
        sendMqttMessage("button_W","OFF_W");
        interval=1000;
        closeServo();
        lightOFF();
        //windowClose();
    }else if(strcmp(resp,"free")==0){
        Serial.print("you're in free mode");
        sendMqttMessage("servo","100");
        sendMqttMessage("button_W","ON_W");
        isManualMode=false;
        isSleepMode=false;
        isFreeMode=true;
        isRegulize=false;
        openServo();
    }
}

int actual=0;
void lightON(){
    actual=255;
    isLightOn=true;
    analogWrite(PIN, 255);
    sendMqttMessage("topic",itoa(255,buff,10));
    sendMqttMessage("button_led","ON");
    delay(50);
}

void lightOFF(){
   isLightOn=false;
   analogWrite(PIN, 0);
   sendMqttMessage("topic",itoa(0,buff,10));
   sendMqttMessage("button_led","OFF"); //provare a commentare questa riga
   delay(50);
}

void MoreLight(){
  if(!isLightOn) sendMqttMessage("log","the light is OFF switch ON first");
  else if (actual<=240){
    sendMqttMessage("button_led","ON");
    actual=actual+15; 
    analogWrite(PIN, actual);
    delay(50);
    sendMqttMessage("topic",itoa(actual,buff,10));
  } else 
      sendMqttMessage("log","Max Light reached");
}

void LessLight(){
  if(!isLightOn) sendMqttMessage("log","the light is OFF switch ON first");
  else if(actual>=15){
    actual=actual-15;
   analogWrite(PIN,actual);
   sendMqttMessage("topic",itoa(actual,buff,10));
  } else sendMqttMessage("log","Min Light reached");
  if (actual==0) sendMqttMessage("button_led","OFF");
}

void Regulize(){
   sendMqttMessage("topic","155");
   if(!buttonLed){
      buttonLed=true;
      sendMqttMessage("button_led","ON");
   }
   analogWrite(PIN,155);
   delay(50);
}

void controllFreeMode(){
    if(ints<30){
      lightON();
      if(!buttonLed){
        sendMqttMessage("button_led","ON");
        buttonLed=true;
      }
      isRegulize=false;
      }else if(ints>=30 && ints<=60){
        if(!isRegulize){
           Regulize();
           isRegulize=true;    
        }
      }else if (ints>60){
       lightOFF();
        if(buttonLed){
          buttonLed=false;
          sendMqttMessage("button_led","OFF");
       }
       isRegulize=false;
    }
  }


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(PIN,OUTPUT);
  //strip.begin();
  //strip.show();
  Wire.begin();
  wifiConnect();
  //mqttConnect();
  angle_curr=0;
  servo.attach(8);
  servo.write(angle);
  lightMeter.begin();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();

  if (client_mqtt.connected()) {
      client_mqtt.loop();  
  }
  
 if(isFreeMode){
  
    if(currentMillis - previousMillis > interval) {
     intensity=lightMeter.readLightLevel();
     intensity_normalized=map(intensity, 0, 3000, 0, 1000);
     if(intensity_normalized<=10){
      ints=5;
      sendMqttMessage("temp","5");
     }else if(intensity_normalized>10 && intensity_normalized<=20){
       ints=10;
       sendMqttMessage("temp","10");
     }else if(intensity_normalized>30 && intensity_normalized<=40){
       ints=20;
       sendMqttMessage("temp","20");
     }else if(intensity_normalized>40 && intensity_normalized<=60){
       ints=30;
       sendMqttMessage("temp","30");
     }else if(intensity_normalized>60 && intensity_normalized<=90){
       ints=40;
       sendMqttMessage("temp","40");
     }else if(intensity_normalized>90 && intensity_normalized<=120){
       ints=50;
       sendMqttMessage("temp","50");
     }else if(intensity_normalized>120 && intensity_normalized<=150){
       ints=60;
       sendMqttMessage("temp","60");
     }else if(intensity_normalized>150 && intensity_normalized<=200){
       ints=70;
       sendMqttMessage("temp","70");
     }else if(intensity_normalized>200 && intensity_normalized<=300){
       ints=80;
       sendMqttMessage("temp","80");
     }else if(intensity_normalized>300 && intensity_normalized<=350){
       ints=90;
       sendMqttMessage("temp","90");
     }else if(intensity_normalized>350){
       ints=100;
       sendMqttMessage("temp","100");
     }
     previousMillis = currentMillis;
     previousLight=intensity_normalized;
    }
    if (isFreeMode && (currentMillis-previousMillisFree >intervalFree)){
      controllFreeMode();
      previousMillisFree=currentMillis;
    } 
  }
  
}
