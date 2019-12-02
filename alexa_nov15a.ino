//include support files
#include "arduino_secrets.h"
#include "thingProperties.h"

//include MQTT and HTTP libraries
#include <PubSubClient.h>
#include <ArduinoHttpClient.h>
#include <Servo.h>

char serverAddress[] = "maker.ifttt.com";  // server address for IFTTT
int port = 443;

int speakerPin = 0;
int sensorGas = A1;
int sensorFire = A3;
int sensorValueGas = 0;
int sensorValueFire = 0;
int angle=10;
int pinServo = 1;
int open_window = 0;
boolean send_message = false;

Servo servo;

//initialize wifi client, MQTT client and HttpClient
WiFiClient client_wifi;
WiFiSSLClient client_secure;
PubSubClient client_mqtt(client_wifi);
HttpClient client = HttpClient(client_secure, serverAddress, port);

//MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {}

//connection function to MQTT
void mqttConnect(){
  //set MQTT server
  client_mqtt.setServer(mqttServer,mqttPort);
  client_mqtt.setCallback(callback);
  client_mqtt.disconnect();

  //if MQTT has never been started, it starts it
  while(!client_mqtt.connected()) {      
      Serial.println("Collego MQTT...");
        if (client_mqtt.connect(mqttID, mqttUser, mqttPass )) {
          Serial.println("MQTT connesso!");
          client_mqtt.publish("Messaggio","Benvenuto!");
          boolean rc = client_mqtt.subscribe("temp2",0);
        }else {
          Serial.print("Errore di connessione!");
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

void setup() {

  //open serial connection
  Serial.begin(9600);
  pinMode(speakerPin, OUTPUT); 
  servo.attach(pinServo);
  servo.write(angle);
  
  //wait 2 seconds
  delay(2000);
  
  //inizialize the properties of Arduino Iot Cloud
  initProperties();

  //Connect the board to the wifi and connect the Arduino Iot Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  //Initialize the debug mode of Arduino Iot Cloud
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

}

void loop() {
  //I keep the status of properties of the Arduino Iot Cloud updated
  ArduinoCloud.update();
  
//I reset the connection for mqtt if I have connection problems to avoid future timeouts
 if (client_mqtt.connected()) {
      client_mqtt.loop();
  }

  sensorValueGas = analogRead(sensorGas);
  sensorValueFire = analogRead(sensorFire);
  // Print out the value you read
  Serial.println(sensorValueGas, DEC);
  Serial.println(sensorValueFire);
  delay(1000);
  
  // If sensorValueGas is greater than 500 and the window is closed, open the window
  if(sensorValueGas > 500 && !open_window){
    Serial.println(sensorValueGas, DEC);

    //sends the POST request to IFTTT
    Serial.println("making POST request");
    String contentType = "application/json";
    String postData = "{\"key1\": \"\"}";
    client.post("/trigger/send_email_gas_sensor/with/key/**INSERT YOUR SECRET KEY**", contentType, postData);
    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    for(angle = 10; angle < 180; angle++){                                  
    servo.write(angle);               
    delay(15);                   
    }
    open_window = 1; 
    Serial.println("finestra aperta");
    Serial.println(open_window);

   //until the sensorValueGas is greater than 500, the buzzer sounds
   while(sensorValueGas > 500){
    sensorValueGas = analogRead(sensorGas);
    Serial.println(sensorValueGas, DEC);
    tone(speakerPin,1000,500);
    delay(1500);
   }
   
   Serial.println("Wait three seconds");
   delay(3000);
  }

   //if the window is open, it is closed if the sensorValueGas is less than 300
   if((sensorValueGas < 300) && (open_window == 1)){
    Serial.println(sensorValueGas, DEC);
    for(angle = 180; angle > 10; angle--){                                
      servo.write(angle);           
      delay(15);       
    } 
    open_window = 0;
    Serial.println("finestra chiusa");
    Serial.println(open_window);
  }
  
  if(sensorValueFire > 500){
    Serial.println(sensorValueFire);
    delay(500);

    //sends the POST request to IFTTT
    Serial.println("making POST request");
    String contentType = "application/json";
    String postData = "{\"key1\": \"\"}";
    client.post("/trigger/send_email_fire_sensor/with/key/**INSERT YOUR SECRET KEY**", contentType, postData);
   // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    //until the sensorValueFire is greater than 500, the buzzer sounds
    while(sensorValueFire > 500){
      sensorValueFire = analogRead(sensorFire);
      Serial.println(sensorValueFire);
      tone(speakerPin,1000,500);
      delay(1500);
    }
     
    Serial.println("Wait three seconds");
    delay(3000);
  }
  
}

void onModalitaFreeChange() {
  if(modalitaSleep != 0 || modalitaManual != 0 && modalitaFree == 1){ //the state of the free mode changes and then this function is executed, for this reason this "if" is only executed if there are 2 modes at 1
    modalitaSleep = 0;
    modalitaManual = 0;
  }
  //show the current status of the Led Light property on the serial window
  Serial.println(modalitaFree);
  Serial.println(modalitaManual);
  Serial.println(modalitaSleep);
  //I write on MQTT every time that the status of the property variable is updated
  if(modalitaFree == 1){
    delay(500);
    sendMqttMessage("topic", "free");
  }
}


void onModalitaSleepChange() {
    if(modalitaFree != 0 || modalitaManual != 0 && modalitaSleep == 1){ //the state of the sleep mode changes and then this function is executed, for this reason this "if" is only executed if there are 2 modes at 1
      modalitaFree = 0;
      modalitaManual = 0;
  }
  //show the current status of the Led Light property on the serial window
  Serial.println(modalitaFree);
  Serial.println(modalitaManual);
  Serial.println(modalitaSleep);
  //I write on MQTT every time that the status of the property variable is updated
  if(modalitaSleep == 1){
    delay(500);
    sendMqttMessage("topic", "sleep");
  }
}


void onModalitaManualChange() {
   if(modalitaSleep != 0 || modalitaFree != 0 && modalitaManual == 1){ //the state of the manual mode changes and then this function is executed, for this reason this "if" is only executed if there are 2 modes at 1
    modalitaSleep = 0;
    modalitaFree = 0;
  }
  //show the current status of the Led Light property on the serial window
  Serial.println(modalitaFree);
  Serial.println(modalitaManual);
  Serial.println(modalitaSleep);
  //I write on MQTT every time that the status of the property variable is updated
  if(modalitaManual == 1){
    delay(500);
    sendMqttMessage("topic", "manual");
  }
}
