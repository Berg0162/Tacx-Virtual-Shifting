#ifndef NIMBLEMANAGER_H
#define NIMBLEMANAGER_H

#include <Arduino.h>
#include <string>

#include <NimBLEDevice.h>

class NimBLEManager {

private:
NimBLEManager();
static NimBLEManager* instance; // Singleton class instance

NimBLEAdvertising* pAdvertising;
uint8_t serverPeerAddress[6];
uint16_t serverConnectionHandle = BLE_HS_CONN_HANDLE_NONE;

const NimBLEAdvertisedDevice* trainerDevice;
NimBLEScan* pNimBLEScan;
uint8_t clientPeerAddress[6] = {};
std::string clientPeerName;
bool hasConnectPassed = false;
// xTask to connect Client with peripheral and check all services + characteristics
static void xTaskClientConnectServer(void *parameter);
TaskHandle_t xTaskClientConnectServerHandle = NULL;
// xTask to subscribe/unsubscribe to all relevant client Characteritics after connect/disconnect
static void xTaskClientSubscribeAll(void *parameter);
static void xTaskClientUnSubscribeAll(void *parameter);
TaskHandle_t xTaskClientSubscribeUnsubscribeHandle = NULL;
bool hasSubscribedToAll = false;

public:
~NimBLEManager();
static NimBLEManager* getInstance(); // Singleton access method
NimBLEServer* pServer = nullptr;     // Pointer to NimBLEServer instance
NimBLEClient* pClient = nullptr;     // Pointer to NimBLEClient instance

bool serverIsConnected = false;
void serverConnectionCallbacksOnConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo); 
void serverConnectionCallbacksOnDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason); 
void init(void);
void startAdvertising(void);
bool isAdvertising(void);
void setupAdvertising(void);

bool clientIsConnected = false;
bool clientIsDiscoveringServices = false;
void clientConnectionCallbacksOnConnect(NimBLEClient* pClient);
void clientConnectionCallbacksOnDisconnect(NimBLEClient* pClient, int reason);
bool clientConnectServer(void);
void clientScanCallbacksOnResult(const NimBLEAdvertisedDevice* advertisedDevice);
void startScanning(void);
bool isScanning(void);
void clientSubscribeToAll(bool isEnable);

};

#endif // NIMBLEMANAGER_H