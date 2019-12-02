#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>


const char THING_ID[] = "**INSERT THE THING ID TAKE FROM ARDUINO CREATE**";

const char SSID[] = SECRET_SSID;    // Network SSID (name)
const char PASS[] = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

void onModalitaManualChange();
void onModalitaFreeChange();
void onModalitaSleepChange();

//Type and name of property variables
CloudLight modalitaManual;
CloudLight modalitaFree;
CloudLight modalitaSleep;

void initProperties(){

  modalitaFree = 0;
  modalitaManual = 0;
  modalitaSleep = 0;
  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(modalitaManual, READWRITE, ON_CHANGE, onModalitaManualChange);
  ArduinoCloud.addProperty(modalitaFree, READWRITE, ON_CHANGE, onModalitaFreeChange);
  ArduinoCloud.addProperty(modalitaSleep, READWRITE, ON_CHANGE, onModalitaSleepChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
