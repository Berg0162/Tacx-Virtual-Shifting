#ifndef VIRTUALSHIFTING_H
#define VIRTUALSHIFTING_H

#include <Arduino.h>
#include <string>

// Include all NimBLE constants
#include "config/configNimBLE.h"
#include "Utilities.h"
#include <NimBLEDevice.h>
#include <map>

class ZVS {
  private:
  ZVS();
  static ZVS* instance;
  // Server Virtual Shifting Service
  NimBLEService *server_VS_Service;
  BLECharacteristic *server_VS_ASYNC_Chr;   // NOTIFY
  BLECharacteristic *server_VS_SYNCRX_Chr;  // WRITE No-Response
  BLECharacteristic *server_VS_SYNCTX_Chr;  // INDICATE

  UTILS::TrainerMode zwiftTrainerMode; // NO default setting
  uint64_t zwiftPower = 0;
  int64_t zwiftGrade = 0; // ZigZag Encoded Signed Int
  uint64_t zwiftGearRatio = 0;
  uint64_t zwiftCrr = 0;
  uint64_t zwiftCw = 0;

  uint16_t zwiftBicycleWeight = 0;
  uint16_t zwiftUserWeight = 0;
  std::map<uint8_t, uint64_t> getZwiftDataValues(std::vector<uint8_t> *requestData);
  std::vector<uint8_t> generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, \
                        int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4);
  void processZwiftSyncRequest(std::vector<uint8_t> *requestData);
  void updateTrainerMode(UTILS::TrainerMode newZwiftTrainerMode);

  public:
  ~ZVS();
  static ZVS* getInstance(); // Singleton access method

  void notifyZwiftAsyncTrainerData(void);
  void server_setupZVS(NimBLEServer* pServer);
  void serverVSSYNCTXOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
  bool isServerVSsynctxIndicateEnabled = false;
  void serverVSSYNCRXOnWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
  void serverVSASYNCOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
  bool isServerVSasyncNotifyEnabled = false;

};

#endif // VIRTUALSHIFTING_H