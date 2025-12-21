#ifndef FITNESSEQUIPMENTCYCLING_H
#define FITNESSEQUIPMENTCYCLING_H

#include <Arduino.h>
#include <string>

// Include all NimBLE constants
#include "config/configNimBLE.h"
// Include all TACX settings
#include "config/configTacx.h"
#include "Utilities.h"
#include <NimBLEDevice.h>

class FEC {
private:
FEC();
static FEC* instance;  // Singleton class instance
// Server Fitness Equipment Cycling Service -----------------------------------------------------
NimBLEService* server_FitnessEquipmentCycling_Service = nullptr;  
NimBLECharacteristic* server_FEC_Rxd_Chr = nullptr;         // Notify, Read
NimBLECharacteristic* server_FEC_Txd_Chr = nullptr;         // Write No Response

// Client Fitness Equipment Cycling Service -----------------------------------------------------
NimBLERemoteService* pRemote_FitnessEquipmentCycling_Service = nullptr;
NimBLERemoteCharacteristic* pRemote_FEC_Rxd_Chr = nullptr;  // Notify, Read
NimBLERemoteCharacteristic* pRemote_FEC_Txd_Chr = nullptr;  // Write No Response

uint16_t trainerInstantPower = 0;
uint16_t trainerInstantSpeed = 0;
float zwiftInstantGrade = 0.0f;
uint8_t trainerInstantCadence = 0;
uint16_t trainerMaximumResistance = 0;
#ifdef TACXNEO_HAPTIC_FEEDBACK
// Haptic Feedback
TaskHandle_t xTaskHapticFeedbackHandle = NULL;
void static xTaskGiveHapticFeedback(void *parameter);
#endif

void client_FEC_Rxd_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                                                  uint8_t* pData, size_t length, bool isNotify);
inline void static Static_FEC_Rxd_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                    uint8_t* pData, size_t length, bool isNotify) __attribute__((always_inline));
public:
~FEC();
static FEC* getInstance();  // Singleton access method
void serverFECRxdOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue);
void serverFECTxdOnWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo);
boolean isServerFECRxdNotifyEnabled = false;
void server_FEC_Rxd_Chr_Notify(uint8_t* pData, size_t length);

void server_setupFEC(NimBLEServer* pServer);
bool client_FitnessEquipmentCycling_Connect(NimBLEClient* pClient);
void client_FEC_Subscribe(void);
void client_FEC_Unsubscribe(void);

bool writeFECCommonPage70Req51(void);
bool write_Client_FEC_Txd_Chr(std::vector<uint8_t>* data);
uint8_t getFECChecksum(std::vector<uint8_t>* fecData);
uint8_t calculateChecksum(const uint8_t* data, size_t length);
bool writeFECTargetPower(uint16_t targetPower);
bool writeFECUserConfiguration(uint16_t bicycleWeight, uint16_t userWeight, uint8_t wheelDiameter, uint8_t gearRatio);
bool writeFECTrackResistance(uint16_t grade, uint8_t rollingResistance);
bool writeFECCapabilitiesRequest(void);
bool writeFECRoadFeel(uint8_t type, uint8_t intensity);
void updateTrainerResistance(UTILS::TrainerMode zwiftTrainerMode, uint64_t zwiftPower, int64_t zwiftGrade, \
                                       uint64_t zwiftGearRatio, uint16_t zwiftBicycleWeight, uint16_t zwiftUserWeight);
void setTrainerInNeutral(void);
uint16_t getInstantPower(void);
uint8_t getInstantCadence(void);
uint16_t getInstantSpeed(void);
bool isTrainerMoving(void);
uint8_t getRoadFeelIntensity(void);
#ifdef TACXNEO_HAPTIC_FEEDBACK
void triggerHapticFeedback(uint64_t zwiftGearRatio);
#endif
}; // class FEC

#endif // FITNESSEQUIPMENTCYCLING_H