#include "VirtualShifting.h"

// Include FEC class
#include "FitnessEquipmentCycling.h"

// ------------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate debug messages for this class
#define DEBUG
// Include these debug utility macros in all cases!
#include "config/configDebug.h"
// ------------------------------------------------------------------------------------------------

// Unsigned little endian base 128 (ULEB128) implementation, 
// which stores arbitrarily large unsigned integers in a variable length format. 
#include <uleb128.h>

// ------------------------------------------------------------------------------------
// Server Virtual Shifting ------------------------------------------------------------
// ------------------------------------------------------------------------------------
#define ZWIFT_VIRTUAL_SHIFTING_SERVICE  NimBLEUUID((uint16_t)0xFC82)
static NimBLEUUID ZWIFT_ASYNC_CHARACTERISTIC_UUID("00000002-19CA-4651-86E5-FA29DCDD09D1");   // Measurement
static NimBLEUUID ZWIFT_SYNCRX_CHARACTERISTIC_UUID("00000003-19CA-4651-86E5-FA29DCDD09D1");  // Control Point (commands)
static NimBLEUUID ZWIFT_SYNCTX_CHARACTERISTIC_UUID("00000004-19CA-4651-86E5-FA29DCDD09D1");  // Response (to commands)

const uint8_t zwiftAsyncRideOnResponse1[18] = {0x2a, 0x08, 0x03, 0x12, 0x0d, 0x22, 0x0b, 0x52, 0x49, 0x44, 0x45, 0x5f, 0x4f, \
                                             0x4e, 0x28, 0x30, 0x29, 0x00};
const uint8_t zwiftAsyncRideONResponse2[18] = {0x2a, 0x08, 0x03, 0x12, 0x0d, 0x22, 0x0b, 0x52, 0x49, 0x44, 0x45, 0x5f, 0x4f, \
                                             0x4e, 0x28, 0x32, 0x29, 0x00};
const uint8_t zwiftSyncRideOnResponse[8] = {0x52, 0x69, 0x64, 0x65, 0x4f, 0x6e, 0x02, 0x00};
// Version with TACX NEO
const uint8_t zwiftSyncInfoReqResponse[55] = {0x3c, 0x08, 0x00, 0x12, 0x32, 0x0a, 0x30, 0x08, 0x80, 0x04, 0x12, 0x04, 0x05, \
                                            0x00, 0x05, 0x01, 0x1a, 0x0b, 0x54, 0x41, 0x43, 0x58, 0x20, 0x4e, 0x45, 0x4f, \
                                            0x00, 0x00, 0x00, 0x32, 0x0f, 0x34, 0x30, 0x32, 0x34, 0x31, 0x38, 0x30, 0x30, \
                                            0x39, 0x38, 0x34, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x01, 0x31, 0x42, 0x04, 0x08, \
                                            0x01, 0x10, 0x14};
/* Original with KICKR CORE
const uint8_t zwiftSyncInfoReqResponse[55] = {0x3c, 0x08, 0x00, 0x12, 0x32, 0x0a, 0x30, 0x08, 0x80, 0x04, 0x12, 0x04, 0x05, \
                                            0x00, 0x05, 0x01, 0x1a, 0x0b, 0x4b, 0x49, 0x43, 0x4b, 0x52, 0x20, 0x43, 0x4f, \
                                            0x52, 0x45, 0x00, 0x32, 0x0f, 0x34, 0x30, 0x32, 0x34, 0x31, 0x38, 0x30, 0x30, \
                                            0x39, 0x38, 0x34, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x01, 0x31, 0x42, 0x04, 0x08, \
                                            0x01, 0x10, 0x14};
*/
const uint8_t zwiftSyncReq3Response[12] = {0x3c, 0x08, 0x88, 0x04, 0x12, 0x06, 0x0a, 0x04, 0x40, 0xc0, 0xbb, 0x01};
const uint8_t zwiftSyncReq10Response[41] = {0x3c, 0x08, 0x00, 0x12, 0x24, 0x08, 0x80, 0x04, 0x12, 0x04, 0x00, 0x04, 0x00, \
                                          0x0c, 0x1a, 0x00, 0x32, 0x0f, 0x42, 0x41, 0x2d, 0x45, 0x34, 0x33, 0x37, 0x32, \
                                          0x44, 0x39, 0x32, 0x37, 0x42, 0x44, 0x45, 0x3a, 0x00, 0x42, 0x04, 0x08, 0x01, \
                                          0x10, 0x53};

// Initialize the static member
ZVS* ZVS::instance = nullptr;

class server_VS_ASYNC_Chr_callbacks final : public NimBLECharacteristicCallbacks {
private:
    ZVS* zvsInstance;  // LOCAL Pointer to the ZVS class instance
public:
    // Constructor to accept a pointer to the ZVS class instance
    server_VS_ASYNC_Chr_callbacks(ZVS* instance) : zvsInstance(instance) {}
protected:
    inline void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        zvsInstance->serverVSASYNCOnSubscribe(pCharacteristic, connInfo, subValue);
    } // onSubscribe
}; 

class server_VS_SYNCTX_Chr_callbacks final : public NimBLECharacteristicCallbacks {
private:
    ZVS* zvsInstance;  // LOCAL Pointer to the ZVS class instance
public:
    // Constructor to accept a pointer to the ZVS class instance
    server_VS_SYNCTX_Chr_callbacks(ZVS* instance) : zvsInstance(instance) {}
protected:
    inline void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        zvsInstance->serverVSSYNCTXOnSubscribe(pCharacteristic, connInfo, subValue);
    } // onSubscribe
}; 

class server_VS_SYNCRX_Chr_callbacks final : public NimBLECharacteristicCallbacks {
private:
    ZVS* zvsInstance;  // LOCAL Pointer to the ZVS class instance
public:
    // Constructor to accept a pointer to the ZVS class instance
    server_VS_SYNCRX_Chr_callbacks(ZVS* instance) : zvsInstance(instance) {}
protected:
    inline void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        zvsInstance->serverVSSYNCRXOnWrite(pCharacteristic, connInfo);
    }
};

void ZVS::serverVSSYNCRXOnWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string syncRXData = pCharacteristic->getValue();
    size_t syncRXDataLen = syncRXData.length();
    if(syncRXDataLen == 0) return;
    /* Decodes a raw incoming SYNCRX request for extra debugging only
    std::string hexString = UTILS::getHexString( reinterpret_cast<uint8_t const*>(syncRXData.data()), syncRXDataLen );
    LOG("--> Rec'd SYNCRX request [len: %d] [Values: %s]", syncRXDataLen, hexString.c_str()); 
    */
    // Convert std::string to std::vector<uint8_t>
    std::vector<uint8_t> requestData(syncRXData.begin(), syncRXData.end());
    processZwiftSyncRequest(&requestData);
};

void ZVS::serverVSASYNCOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) {
    std::string str = std::string(pCharacteristic->getUUID());
    if(subValue == 0) {
      LOG("Central Unsubscribed: [%s] ASYNC Status", str.c_str());
      isServerVSasyncNotifyEnabled = false;
    } else {
      LOG("Central Subscribed: [%s] ASYNC Status", str.c_str());
      isServerVSasyncNotifyEnabled = true;
    }
};

void ZVS::serverVSSYNCTXOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) {
    std::string str = std::string(pCharacteristic->getUUID());
    if(subValue == 0) {
      LOG("Central Unsubscribed: [%s] SYNCTX Data", str.c_str());
      isServerVSsynctxIndicateEnabled = false;
    } else {
      LOG("Central Subscribed: [%s] SYNCTX Data", str.c_str());
      isServerVSsynctxIndicateEnabled = true;
    }
};

// Constructor
ZVS::ZVS() { }

// Destructor
ZVS::~ZVS() { }

ZVS* ZVS::getInstance() {
    if (ZVS::instance == nullptr) {
        ZVS::instance = new ZVS();
    }
    return ZVS::instance;
}

void ZVS::server_setupZVS(NimBLEServer* pServer) {
    server_VS_Service = pServer->createService(ZWIFT_VIRTUAL_SHIFTING_SERVICE);
    server_VS_ASYNC_Chr = server_VS_Service->createCharacteristic(ZWIFT_ASYNC_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);
    server_VS_ASYNC_Chr->setCallbacks(new server_VS_ASYNC_Chr_callbacks(this));
    server_VS_SYNCRX_Chr = server_VS_Service->createCharacteristic(ZWIFT_SYNCRX_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
    server_VS_SYNCRX_Chr->setCallbacks(new server_VS_SYNCRX_Chr_callbacks(this));
    server_VS_SYNCTX_Chr = server_VS_Service->createCharacteristic(ZWIFT_SYNCTX_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::INDICATE);
    server_VS_SYNCTX_Chr->setCallbacks(new server_VS_SYNCTX_Chr_callbacks(this));   
    server_VS_Service->start();   
}

std::map<uint8_t, uint64_t> ZVS::getZwiftDataValues(std::vector<uint8_t> *requestData) {
  std::map<uint8_t, uint64_t> returnMap;
  if (requestData->size() > 2) {
    if (requestData->at(0) == 0x04) {
      size_t processedBytes = 0;
      size_t dataIndex = 0;
      uint8_t currentKey = 0;
      uint64_t currentValue = 0;
      if (((requestData->at(1) == 0x22) || (requestData->at(1) == 0x2A)) && requestData->size() > 4) {
        dataIndex = 3;
        if (requestData->size() == (requestData->at(2) + dataIndex)) {
          while (dataIndex < requestData->size()) {
            currentKey = requestData->at(dataIndex);
            dataIndex++;
            processedBytes = bfs::DecodeUleb128(requestData->data() + dataIndex, requestData->size() - dataIndex, &currentValue);
            dataIndex = dataIndex + processedBytes;
            if (processedBytes == 0) {
              LOG("Error parsing unsigned Zwift data values, hex: ", UTILS::getHexString(requestData).c_str());
              dataIndex++;
            }
            returnMap.emplace(currentKey, currentValue);
          }
        } else {
          LOG("Error parsing unsigned Zwift data values, length mismatch");
        }
      } else if (requestData->at(1) == 0x18) {
        dataIndex = 2;
        processedBytes = bfs::DecodeUleb128(requestData->data() + dataIndex, requestData->size() - dataIndex, &currentValue);
        if (processedBytes == 0) {
          LOG("Error parsing unsigned Zwift data value, hex: ", UTILS::getHexString(requestData).c_str());
        } else {
          returnMap.emplace(currentKey, currentValue);
        }
      }
    }
  }
  return returnMap;
}

std::vector<uint8_t> ZVS::generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, \
                                                              int64_t unknown2, int64_t unknown3, int64_t unknown4) {
  std::vector<uint8_t> notificationData;
  int64_t currentData = 0;
  uint8_t leb128Buffer[16];
  size_t leb128Size = 0;

  notificationData.push_back(0x03);

  for (uint8_t dataBlock = 0x08; dataBlock <= 0x30; dataBlock += 0x08) {
    notificationData.push_back(dataBlock);
    if (dataBlock == 0x08) {
      currentData = power;
    }
    if (dataBlock == 0x10) {
      currentData = cadence;
    }
    if (dataBlock == 0x18) {
      currentData = unknown1;
    }
    if (dataBlock == 0x20) {
      currentData = unknown2;
    }
    if (dataBlock == 0x28) {
      currentData = unknown3;
    }
    if (dataBlock == 0x30) {
      currentData = unknown4;
    }
    leb128Size = bfs::EncodeUleb128(currentData, leb128Buffer, sizeof(leb128Buffer));
    for (uint8_t leb128Byte = 0; leb128Byte < leb128Size; leb128Byte++) {
      notificationData.push_back(leb128Buffer[leb128Byte]);
    }
  }
  return notificationData;
}

void ZVS::notifyZwiftAsyncTrainerData(void) {
  uint16_t trainerPower = FEC::getInstance()->getInstantPower();
  uint8_t trainerCadence = FEC::getInstance()->getInstantCadence();
  LOG("Notify ASYNC Trainer Data -> Power: %d Cadence: %d isMoving: %X", trainerPower, trainerCadence, FEC::getInstance()->isTrainerMoving());
  std::vector<uint8_t> notificationData = generateZwiftAsyncNotificationData(trainerPower, trainerCadence, 0, 0, 0, 0);
  if(isServerVSasyncNotifyEnabled) {
    server_VS_ASYNC_Chr->setValue(notificationData.data(), notificationData.size());
    server_VS_ASYNC_Chr->notify();
  }
}

void ZVS::updateTrainerMode(UTILS::TrainerMode newZwiftTrainerMode) {
    if(zwiftTrainerMode != newZwiftTrainerMode) {
#ifdef DEBUG
      // Show new trainer mode requested
      switch (newZwiftTrainerMode) {
        case UTILS::TrainerMode::SIM_MODE:
          LOG(" -> SIM mode requested");
          break;
        case UTILS::TrainerMode::SIM_MODE_VIRTUAL_SHIFTING:
          LOG(" -> SIM mode + VS requested");
          break;
        case UTILS::TrainerMode::ERG_MODE:
          LOG(" -> ERG mode requested");
          break;
        default:
          LOG(" -> Mode error!");
          break;
      }
#endif
    zwiftTrainerMode = newZwiftTrainerMode; // Set new trainer mode to active
    }
}

void ZVS::processZwiftSyncRequest(std::vector<uint8_t> *requestData) {
    if (requestData->size() < 3) {
      return;
    }
    uint8_t zwiftCommand = requestData->at(0);
    uint8_t zwiftCommandSubtype = requestData->at(1);
    uint8_t zwiftCommandLength = requestData->at(2);
    std::map<uint8_t, uint64_t> requestValues = getZwiftDataValues(requestData);
    LOG("Zwift request: %s", UTILS::getHexString(requestData).c_str());
/* for extra debugging only
    if ( (zwiftCommandSubtype == 0x18) || (zwiftCommandSubtype == 0x22) || (zwiftCommandSubtype == 0x2A)) {
      LOG("Zwift command: %02X, commandsubtype: %02X len: %02X", zwiftCommand, zwiftCommandSubtype, zwiftCommandLength);
      for (auto requestValue = requestValues.begin(); requestValue != requestValues.end(); requestValue++) {
        LOG("Zwift data key: %02X, value %d", requestValue->first, requestValue->second);
      }
    }
*/
    switch (zwiftCommand) {
      // Status request Zwift Play init #
      case 0x00:
        if( (zwiftCommandSubtype == 0x08) && (zwiftCommandLength == 0x00) ) {
            // Status request Zwift Play -> [00 08 00]
            LOG(" -> Zwift 0x08 Init #10 Request: Responded!");
            if(isServerVSsynctxIndicateEnabled) {
              server_VS_SYNCTX_Chr->setValue(zwiftSyncReq10Response, sizeof(zwiftSyncReq10Response));
              server_VS_SYNCTX_Chr->indicate();
            }
        } if( (zwiftCommandSubtype == 0x08) && (zwiftCommandLength == 0x88) && (requestData->at(3) == 0x04) ) { 
            // Status request Zwift Play ->  [00 08 88 04]
            LOG(" -> Zwift 0x08 Status Request: Responded!");
            if(isServerVSsynctxIndicateEnabled) {
              server_VS_SYNCTX_Chr->setValue(zwiftSyncReq3Response, sizeof(zwiftSyncReq3Response));
              server_VS_SYNCTX_Chr->indicate();
            }
        } else
            LOG(" -> Unsolved Zwift 0x08 Request: No Response!");  
        return;
        break;

      // Change request (ERG / SIM mode / Gear ratio / ...)
      case 0x04:
        switch (zwiftCommandSubtype) {
          // ERG Mode
          case 0x18:
            updateTrainerMode(UTILS::TrainerMode::ERG_MODE);
            if (requestValues.find(0x00) != requestValues.end()) {
              zwiftPower = requestValues.at(0x00);
            }
            LOG(" -> Set Target Power: %llu", zwiftPower);
            notifyZwiftAsyncTrainerData();
            FEC::getInstance()->updateTrainerResistance(zwiftTrainerMode, zwiftPower, zwiftGrade, zwiftGearRatio, \
                                                                              zwiftBicycleWeight, zwiftUserWeight);
            break;

          // SIM Mode and SIM Mode + VS --> Parameter update: Inclination and optional Cw and Crr
          case 0x22:
            if (requestValues.find(0x10) != requestValues.end()) {
              zwiftGrade = requestValues.at(0x10); // ZigZag Encoded Signed Int
              // Apply ZigZag decoding to get the original signed value
              // taken from: qdomyos-zwift/src/virtualdevices/virtualbike.cpp {line:572}
              zwiftGrade = (zwiftGrade >> 1) ^ -(zwiftGrade & 1);
              LOG(" -> ZwiftGrade: %6.2f", (float)(zwiftGrade)/100.0);
            }
            if (requestValues.find(0x18) != requestValues.end()) {
              zwiftCw = requestValues.at(0x18); 
              LOG(" -> zwiftCw: %5.3f ", (zwiftCw / 10000.0));
            }
            if (requestValues.find(0x20) != requestValues.end()) {
              zwiftCrr = requestValues.at(0x20); 
              LOG(" -> zwiftCrr: %6.4f", (zwiftCrr / 100000.0));
            }
            notifyZwiftAsyncTrainerData();
            UTILS::setTrainerCrr(zwiftCrr/100000.0); // Update current Coefficient of Rolling Resistance
            UTILS::setTrainerCw(zwiftCw/10000.0); // Update current Coefficient of Wind Resistance
            FEC::getInstance()->updateTrainerResistance(zwiftTrainerMode, zwiftPower, zwiftGrade, zwiftGearRatio, \
                                                                              zwiftBicycleWeight, zwiftUserWeight);
            break;

          // SIM mode and SIM Mode + VS --> parameter update: zwift Gear Ratio, Optional: Bicycle Weight and Rider Weight
          case 0x2A: { // create a scope for temp variables
            if (requestValues.find(0x10) != requestValues.end()) {
              zwiftGearRatio = requestValues.at(0x10);
            }
#ifdef DEBUG
            double gearRatio = (float)zwiftGearRatio/10000;
            int currentGear = UTILS::getGearNumberFromRatio(gearRatio);
            LOG(" -> zwiftGearRatio: %4.2f -> Gear: %d", gearRatio, currentGear);
#endif
            if (zwiftGearRatio == 0) updateTrainerMode(UTILS::TrainerMode::SIM_MODE);
            else updateTrainerMode(UTILS::TrainerMode::SIM_MODE_VIRTUAL_SHIFTING);

            if (requestValues.find(0x20) != requestValues.end()) {
              zwiftBicycleWeight = requestValues.at(0x20);
              if (requestValues.find(0x28) != requestValues.end()) {
                zwiftUserWeight = requestValues.at(0x28);
                if (!FEC::getInstance()->writeFECUserConfiguration((uint16_t)(zwiftBicycleWeight / 5), zwiftUserWeight, \
                                (uint8_t)(UTILS::wheelDiameter / 0.01), (uint8_t)round(UTILS::defaultGearRatio / 0.03)) ) {
                  LOG("Error writing FEC user configuration");
                }
              }
              LOG(" -> zwiftBicycleWeight: %4.1f zwiftUserWeight: %4.1f", (float)zwiftBicycleWeight/100, \
                                                                                         (float)zwiftUserWeight/100);
            }

            // Update NOW the Trainer Resistance accordingly
            FEC::getInstance()->updateTrainerResistance(zwiftTrainerMode, zwiftPower, zwiftGrade, zwiftGearRatio, \
                                                                                zwiftBicycleWeight, zwiftUserWeight);
#ifdef TACXNEO_HAPTIC_FEEDBACK
            // Gear has Changed -> Trigger Haptic Feedback Event
            if(zwiftTrainerMode == UTILS::TrainerMode::SIM_MODE_VIRTUAL_SHIFTING)
              FEC::getInstance()->triggerHapticFeedback(zwiftGearRatio);
#endif
            break;
          } // scope
          // Unknown
          default:
            LOG(" -> Unknown Zwift Request with hex value: %s", UTILS::getHexString(requestData).c_str());
            for (auto requestValue = requestValues.begin(); requestValue != requestValues.end(); requestValue++) {
              LOG("Zwift sync data key: %d, value %d", requestValue->first, requestValue->second);
            }
            break;
        } // switch zwiftCommandSubtype

        return;
        break;

      // Device Info Request, --> Zwift Play Init 2 [41 08 05]
      case 0x41:
        LOG(" -> Zwift 0x41 Device Info Request: Responded!"); 
        if(isServerVSsynctxIndicateEnabled) {
          server_VS_SYNCTX_Chr->setValue(zwiftSyncInfoReqResponse, sizeof(zwiftSyncInfoReqResponse));
          server_VS_SYNCTX_Chr->indicate();
        }       
        return;
        break;

      // Initial Connection Request: RideOn -- > Zwift Play Init 1 
      case 0x52:
        LOG(" -> Zwift 0x52 Initial Connection Request: RideOn Responded!"); 
        if (requestData->size() == 8 || requestData->size() == 6) { // Zwift == 8 and Rouvy == 6
          if(isServerVSasyncNotifyEnabled) {
            server_VS_ASYNC_Chr->setValue(zwiftAsyncRideOnResponse1, sizeof(zwiftAsyncRideOnResponse1));
            server_VS_ASYNC_Chr->notify();
            delay(10);
            server_VS_ASYNC_Chr->setValue(zwiftAsyncRideONResponse2, sizeof(zwiftAsyncRideONResponse2));
            server_VS_ASYNC_Chr->notify();
          }
          if(isServerVSsynctxIndicateEnabled) {
            server_VS_SYNCTX_Chr->setValue(zwiftSyncRideOnResponse, sizeof(zwiftSyncRideOnResponse));
            server_VS_SYNCTX_Chr->indicate();
          }
          return;
        }
        break;

      default:
        LOG(" -> Unknown Zwift request");
        break;
    } // switch
    return;
}

