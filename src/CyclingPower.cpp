#include "CyclingPower.h"
#include "NimBLEManager.h"
#include "Utilities.h"
#include "config/configNimBLE.h"
// ------------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate debug messages for this class
#define DEBUG
// Include these debug utility macros in all cases!
#include "config/configDebug.h"

#ifdef DEBUG
//  Restrict activating one or more of the following DEBUG directives --> process intensive 
//  The overhead can lead to spurious side effects and a loss of quality of service handling!!
//#define DEBUG_CP_MEASUREMENT  // If defined allows for parsing and decoding the Cycling Power Measurement Data
//#define DEBUG_CP_DECODE
#endif

/* Cycling Power Service ---------------------------------------------------------------
 * CP Service: 0x1818  
 * CP Characteristic: 0x2A63 (Measurement)    Mandatory
 * CP Characteristic: 0x2A65 (Feature)        Mandatory
 * CP Characteristic: 0x2A5D (Location)       Optional
*/
#define UUID16_SVC_CYCLING_POWER                              NimBLEUUID((uint16_t)0x1818)
#define UUID16_CHR_CYCLING_POWER_MEASUREMENT                  NimBLEUUID((uint16_t)0x2A63)
#define UUID16_CHR_CYCLING_POWER_FEATURE                      NimBLEUUID((uint16_t)0x2A65)
#define UUID16_CHR_SENSOR_LOCATION                            NimBLEUUID((uint16_t)0x2A5D) // shared with CSC

#ifdef DEBUG
const uint8_t client_CP_Feature_Len = 20; // Num. of Feature elements
const char* client_CP_Feature_Str[client_CP_Feature_Len] = { 
      "Pedal power balance supported",
      "Accumulated torque supported",
      "Wheel revolution data supported",
      "Crank revolution data supported",
      "Extreme magnitudes supported",
      "Extreme angles supported",
      "Top/bottom dead angle supported",
      "Accumulated energy supported",
      "Offset compensation indicator supported",
      "Offset compensation supported",
      "Cycling power measurement characteristic content masking supported",
      "Multiple sensor locations supported",
      "Crank length adj. supported",
      "Chain length adj. supported",
      "Chain weight adj. supported",
      "Span length adj. supported",
      "Sensor measurement context",
      "Instantaineous measurement direction supported",
      "Factory calibrated date supported",
      "Enhanced offset compensation supported" };

const uint8_t client_CP_Sensor_Location_Str_Len = 17;       
const char* client_CP_Sensor_Location_Str[client_CP_Sensor_Location_Str_Len] = { "Other", "Top of shoe", "In shoe", "Hip", 
    "Front wheel", "Left crank", "Right crank", "Left pedal", "Right pedal", "Front hub", 
    "Rear dropout", "Chainstay", "Rear wheel", "Rear hub", "Chest", "Spider", "Chain ring"};
#endif

/*  We only define onSubscribe !!!
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);  
    void onNotify(NimBLECharacteristic* pCharacteristic);    
*/
class server_CP_Measurement_Chr_callback final : public NimBLECharacteristicCallbacks {
private:
    CPS* cpsInstance;  // LOCAL Pointer to the CPS class instance
public:
    // Constructor to accept a pointer to the CPS class instance
    server_CP_Measurement_Chr_callback(CPS* instance) : cpsInstance(instance) {}
protected:
    inline void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        cpsInstance->serverCPMeasurementOnSubscribe(pCharacteristic, connInfo, subValue);
    }
};

// Initialize the static member
CPS* CPS::instance = nullptr;

void CPS::serverCPMeasurementOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) {
        std::string str = std::string(pCharacteristic->getUUID());
        if(subValue == 0) {
          LOG("Central Unsubscribed: [%s] CPS Measurement", str.c_str());
          isServerCPMeasurementNotifyEnabled = false;
        } else { 
          LOG("Central Subscribed: [%s] CPS Measurement", str.c_str());
          isServerCPMeasurementNotifyEnabled = true;
        }
}

void CPS::server_CP_Measurement_Chr_Notify(uint8_t* pData, size_t length) {
  if(isServerCPMeasurementNotifyEnabled) {
    server_CP_Measurement_Chr->setValue(pData, length);
    server_CP_Measurement_Chr->notify();
  }
}

void CPS::client_CP_Measurement_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                                              uint8_t* pData, size_t length, bool isNotify) {
  // Client CP Measurement data is transferred to the Server
  if(NimBLEManager::getInstance()->serverIsConnected) {
    server_CP_Measurement_Chr_Notify(pData, length);
  }

#ifdef DEBUG_CP_MEASUREMENT
  uint8_t buffer[length]= {}; 
  // Transfer first the contents of data to buffer (array of chars)
  memcpy(buffer, pData, length);
  std::string hexString = UTILS::getHexString(buffer, static_cast<uint8_t>(length));
  LOG(" -> Client Rec'd Raw CP Measurement Data: [%d] [%s]", length, hexString.c_str());
  uint8_t offset = 0;
  // Get flags field
  uint16_t flags = 0;
  memcpy(&flags, &buffer[offset], 2); // Transfer buffer fields to variable
  offset += 2;  // UINT16
  // Get Instantaneous Power values UINT16
  uint16_t PowerValue = 0;
  memcpy(&PowerValue, &buffer[offset], 2); // Transfer buffer fields to variable
  offset += 2;  // UINT16
  LOG("Instantaneous Power: %4d", PowerValue);
  // Get the other CP measurement values
  if ((flags & 1) != 0) {
    //  Power Balance Present
    LOG("Pedal Power Balance!");
  }
  if ((flags & 2) != 0) {
    // Accumulated Torque
    LOG("Accumulated Torque!");
  }
  // etcetera...
#endif
} // End cpmc_notify_callback

void CPS::Static_CP_Measurement_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                                            uint8_t* pData, size_t length, bool isNotify) {
    if (CPS::instance) {
        CPS::instance->client_CP_Measurement_Notify_Callback(pBLERemoteCharacteristic, pData, length, isNotify);
    }
}

// Constructor
CPS::CPS() { }

// Destructor
CPS::~CPS() { }

CPS* CPS::getInstance() {
    if (CPS::instance == nullptr) {
        CPS::instance = new CPS();
    }
    return CPS::instance;
}

void CPS::server_setupCPS(NimBLEServer* pServer) {
    server_CyclingPower_Service = pServer->createService(UUID16_SVC_CYCLING_POWER);
    server_CP_Measurement_Chr = server_CyclingPower_Service->createCharacteristic(UUID16_CHR_CYCLING_POWER_MEASUREMENT, 
                                                                            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    server_CP_Measurement_Chr->setCallbacks(new server_CP_Measurement_Chr_callback(this));

    server_CP_Feature_Chr = server_CyclingPower_Service->createCharacteristic(UUID16_CHR_CYCLING_POWER_FEATURE, 
                                                                            NIMBLE_PROPERTY::READ);
    // Set server CP Feature Flags field                                                                       
    server_CP_Feature_Chr->setValue(client_CP_Feature_Flags);  // Set default                                                                       
    server_CP_Location_Chr = server_CyclingPower_Service->createCharacteristic(UUID16_CHR_SENSOR_LOCATION, 
                                                                            NIMBLE_PROPERTY::READ);
    // Set server_CP_Location for sensor
    server_CP_Location_Chr->setValue(&client_CP_Location_Value, 1); // Set default 
    server_CyclingPower_Service->start();   
}

bool CPS::client_CyclingPower_Connect(NimBLEClient* pClient) {
    // Obtain a reference to the remote CP service.
    pRemote_CyclingPower_Service = pClient->getService(UUID16_SVC_CYCLING_POWER);
    if (pRemote_CyclingPower_Service == nullptr) {
      LOG("Mandatory Cycling Power Service: Not Found!");
      return false;
    }
    LOG("Client_CyclingPower_Service: Found!");
    pRemote_CP_Measurement_Chr = pRemote_CyclingPower_Service->getCharacteristic(UUID16_CHR_CYCLING_POWER_MEASUREMENT);
    if (pRemote_CP_Measurement_Chr == nullptr) {
      LOG("Mandatory client_CP_Measurement_Chr: Not Found!");
      return false;
    } 
    LOG("Client_CP_Measurement_Chr: Found!");  
    if(!pRemote_CP_Measurement_Chr->canNotify()) {
      LOG("Mandatory Client_CP_Measurement_Chr: Cannot Notify!");
      return false;
    } 
    pRemote_CP_Feature_Chr = pRemote_CyclingPower_Service->getCharacteristic(UUID16_CHR_CYCLING_POWER_FEATURE);
    if (pRemote_CP_Feature_Chr == nullptr) {
      LOG("Mandatory Client_CP_Feature_Chr: Not Found!");
      return false;     
    }
    LOG("Client_CP_Feature_Chr: Found!");
    // Read the value of the characteristic.
    if(pRemote_CP_Feature_Chr->canRead()) {
       // Read 32-bit client_CP_Feature_Chr value
      client_CP_Feature_Flags = pRemote_CP_Feature_Chr->readValue<uint32_t>();
      if(server_CP_Feature_Chr) {
        server_CP_Feature_Chr->setValue(client_CP_Feature_Flags); // Transfer/Update the value to the server side
      }
#ifdef DEBUG_CP_DECODE
      const uint8_t CPFC_FIXED_DATALEN = 4;
      uint8_t cpfcData[CPFC_FIXED_DATALEN] = {static_cast<uint8_t>(client_CP_Feature_Flags & 0xff), \
                                              static_cast<uint8_t>(client_CP_Feature_Flags >> 8), \
                                              static_cast<uint8_t>(client_CP_Feature_Flags >> 16), \
                                              static_cast<uint8_t>(client_CP_Feature_Flags >> 24)};
      std::string hexString = UTILS::getHexString(cpfcData, CPFC_FIXED_DATALEN);                                    
      LOG(" -> Client Reads Raw CP Feature bytes: [4] [%s]", hexString.c_str());
      for (int i = 0; i < client_CP_Feature_Len; i++) {
        if ( client_CP_Feature_Flags & (1 << i) )
          LOG("  %s", client_CP_Feature_Str[i]);
      }
#endif
      } // canRead Feature
    pRemote_CP_Location_Chr = pRemote_CyclingPower_Service->getCharacteristic(UUID16_CHR_SENSOR_LOCATION);
    if (pRemote_CP_Location_Chr == nullptr) {
      LOG("Client_CP_Location_Chr: Not Found!");
    } else {
      LOG("Client_CP_Location_Chr: Found!");
      // Read the value of the characteristic.
      if(pRemote_CP_Location_Chr->canRead()) {
        client_CP_Location_Value = pRemote_CP_Location_Chr->readValue<uint8_t>(); 
        if(server_CP_Location_Chr)
          server_CP_Location_Chr->setValue(&client_CP_Location_Value, 1); // Transfer/Update to the server side
        // CP sensor location value is 8 bit
#ifdef DEBUG_CP_DECODE
        if(client_CP_Location_Value <= client_CP_Sensor_Location_Str_Len) 
          LOG(" -> Client Reads CP Location Sensor: Loc#: %d %s", client_CP_Location_Value, \
                                                        client_CP_Sensor_Location_Str[client_CP_Location_Value]);
        else 
          LOG(" -> Client Reads CP Location Sensor: Loc#: %d", client_CP_Location_Value); 
#endif
      }
    }   
    return true;    
}

void CPS::client_CP_Subscribe(void) { 
  if ( pRemote_CP_Measurement_Chr != nullptr )
    if( !pRemote_CP_Measurement_Chr->subscribe(notifications, Static_CP_Measurement_Notify_Callback) )
      LOG(">>> ERROR remote CP Measure Subscribe failed!");               
};

void CPS::client_CP_Unsubscribe(void) {
  if ( pRemote_CP_Measurement_Chr != nullptr )
    if( !pRemote_CP_Measurement_Chr->unsubscribe(false) )
      LOG(">>> ERROR remote CP Measure UNsubscribe failed!");
};

