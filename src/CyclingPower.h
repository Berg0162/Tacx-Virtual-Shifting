#ifndef CYCLINGPOWER_H
#define CYCLINGPOWER_H

#include <Arduino.h>
#include <string>

// Include all NimBLE constants
#include "config/configNimBLE.h"
#include <NimBLEDevice.h>

// Cycling Power Service
class CPS {
private:
  CPS();
  static CPS* instance;  // Singleton class instance

  // Client Cycling Power Service --------------------------------------------------------
  NimBLERemoteService* pRemote_CyclingPower_Service = nullptr;
  NimBLERemoteCharacteristic* pRemote_CP_Measurement_Chr = nullptr;           // Notify, Read
  NimBLERemoteCharacteristic* pRemote_CP_Feature_Chr = nullptr;               // Read
  uint32_t client_CP_Feature_Flags = { 0b00000000000000010000011010001011 };  // Relevant Cycling Power features
  NimBLERemoteCharacteristic* pRemote_CP_Location_Chr = nullptr;              // Read
  uint8_t client_CP_Location_Value = { 0x0C };                                // rear wheel !

  // Server Cycling Power Service ---------------------------------------------------------------
  NimBLEService* server_CyclingPower_Service = nullptr; 
  NimBLECharacteristic* server_CP_Measurement_Chr = nullptr;   // Notify, Read
  NimBLECharacteristic* server_CP_Feature_Chr = nullptr;       // Read
  NimBLECharacteristic* server_CP_Location_Chr = nullptr;      // Read

  void client_CP_Measurement_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                                                        uint8_t* pData, size_t length, bool isNotify);
  inline void static Static_CP_Measurement_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                            uint8_t* pData, size_t length, bool isNotify) __attribute__((always_inline));

public:
  ~CPS();
  static CPS* getInstance();  // Singleton access method
  void serverCPMeasurementOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
  boolean isServerCPMeasurementNotifyEnabled = false;
  void server_CP_Measurement_Chr_Notify(uint8_t* pData, size_t length);

  void server_setupCPS(NimBLEServer* pServer);
  bool client_CyclingPower_Connect(NimBLEClient* pClient); 
  void client_CP_Subscribe(void); 
  void client_CP_Unsubscribe(void);
};  // CPS -> Cycling Power Service

#endif  // CYCLINGPOWER_H