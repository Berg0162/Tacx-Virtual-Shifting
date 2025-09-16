#include "NimBLEManager.h"

// Include all NimBLE constants
#include "config/configNimBLE.h"
#include "Utilities.h"

#include "GenericAccess.h"
#include "DeviceInformation.h"
#include "CyclingPower.h"
#include "CyclingSpeedCadence.h"
#include "FitnessEquipmentCycling.h"
#include "VirtualShifting.h"

// ------------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate debug messages for this class
#define DEBUG
// Include these debug utility macros in all cases!
#include "config/configDebug.h"
// ------------------------------------------------------------------------------------------------

// For insert in advertisement data
#define UUID16_SVC_CYCLING_POWER BLEUUID((uint16_t)0x1818)
#define UUID16_SVC_CYCLING_SPEED_AND_CADENCE NimBLEUUID((uint16_t)0x1816)
const NimBLEUUID UUID_TACX_FEC_PRIMARY_SERVICE("6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E");
#define ZWIFT_VIRTUAL_SHIFTING_SERVICE NimBLEUUID((uint16_t)0xFC82)

// Initialize the static members
NimBLEManager* NimBLEManager::instance = nullptr; // Singleton class instance

// Server Connect and Disconnect callbacks defined
class server_Connection_Callbacks : public BLEServerCallbacks {
private:
    NimBLEManager* serverInstance;  // LOCAL Pointer to store the NimBLEManager class instance
public:
    // Constructor to accept a pointer to the ServerSide class instance
    server_Connection_Callbacks(NimBLEManager* instance) : serverInstance(instance) {}
protected: 
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    serverInstance->serverConnectionCallbacksOnConnect(pServer, connInfo);
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    serverInstance->serverConnectionCallbacksOnDisconnect(pServer, connInfo, reason);
  }
  void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) {
#ifdef DEBUG
     LOG("Central updated MTU to: %d for connection ID: %d", MTU, connInfo.getConnHandle());
#endif
  }
};

// Client Connect and Disconnect callbacks defined
class client_Connection_Callbacks : public NimBLEClientCallbacks {
private:
    NimBLEManager* clientInstance;  // LOCAL Pointer to store the NimBLEManager class instance
public:
    // Constructor to accept a pointer to the NimBLEManager class instance
    client_Connection_Callbacks(NimBLEManager* instance) : clientInstance(instance) {}
protected: 
  void onConnect(NimBLEClient* pClient) override {
      clientInstance->clientConnectionCallbacksOnConnect(pClient);
  }
  void onDisconnect(NimBLEClient* pClient, int reason) override {
      clientInstance->clientConnectionCallbacksOnDisconnect(pClient, reason);
  }
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, ble_gap_upd_params* params) {
#ifdef DEBUG
    LOG("Client Connection Parameter Update Request!");
    /** Minimum value for connection interval in 1.25ms units */
    uint16_t clientConnectionMinInterval = params->itvl_min;
    LOG("Min Interval: [%d]\n", clientConnectionMinInterval);
    /** Maximum value for connection interval in 1.25ms units */
    uint16_t clientConnectionMaxInterval = params->itvl_max;
    LOG("Max Interval: [%d]\n", clientConnectionMaxInterval);
    /** Connection latency */
    uint16_t clientConnectionLatency = params->latency;
    LOG("Latency: [%d]\n", clientConnectionLatency);
    /** Supervision timeout in 10ms units */
    uint16_t clientConnectionSupTimeout = params->supervision_timeout;
    LOG("Sup. Timeout: [%d]\n", clientConnectionSupTimeout);
    /** Minimum length of connection event in 0.625ms units */
    uint16_t clientMinLenEvent = params->min_ce_len;
    LOG("Min Length Event: [%d]\n", clientMinLenEvent);
    /** Maximum length of connection event in 0.625ms units */
    uint16_t clientMaxLenEvent = params->max_ce_len;
    LOG("Max Length Event: [%d]\n", clientMaxLenEvent);
#endif
  return true;  // That is OK!
    }
};

// Constructor
NimBLEManager::NimBLEManager() { }

// Destructor
NimBLEManager::~NimBLEManager() {}

NimBLEManager* NimBLEManager::getInstance() {
    if (NimBLEManager::instance == nullptr) {
        NimBLEManager::instance = new NimBLEManager();
    }
    return NimBLEManager::instance;
}

void NimBLEManager::init(void) {
  NimBLEDevice::init(THISDEVICENAME);  // shortname
  // Set MTU to be negotiated during request from client peer
  if (NimBLEDevice::setMTU(ConnectionMTU)) {
    LOG("Preferred connection MTU succesfully set to: %d", ConnectionMTU);
  }
  // Optional: set the transmit power, default is 3db
  NimBLEDevice::setPower(3);  // +9db
  // Set the ESP-board hardware static address to PUBLIC
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);

  pServer = NimBLEDevice::createServer();

  // -------------------------
#ifdef DEBUG
  NimBLEAddress MyAddress = NimBLEDevice::getAddress();
  uint8_t ESP32Address[6] = {};
  memcpy(&ESP32Address, MyAddress.getBase()->val, 6);
  uint8_t addressType = MyAddress.getBase()->type;
  char fullMacAddress[18] = {}; 
  UTILS::ConvertMacAddress(fullMacAddress, ESP32Address, false);  // true -> little endian representation!
  LOG("New ESP32 Server/Peripheral created with local Mac Address: [%s][%d]", fullMacAddress, addressType);
#endif
  // -------------------------
  //Setup callbacks onConnect and onDisconnect
  pServer->setCallbacks(new server_Connection_Callbacks(this));
  // Set server auto-restart advertise on
  pServer->advertiseOnDisconnect(true);
  // -------------------------------------------------------
  LOG("Configuring the Generic Access Service");
  GAS::getInstance()->server_setupGAS(pServer);
  LOG("Configuring the Server Device Information Service");
  DIS::getInstance()->server_setupDIS(pServer);
  // Setup the Cycle Power Service, Speed & Cadence Service and FTMS
  LOG("Configuring the Server Cycle Power Service");
  CPS::getInstance()->server_setupCPS(pServer);
  LOG("Configuring the Server Tacx FE-C Service");
  FEC::getInstance()->server_setupFEC(pServer);
  LOG("Configuring the Server Cadence and Speed Service");
  CSC::getInstance()->server_setupCSC(pServer);
  LOG("Configuring the Zwift Virtual Shifting Service");
  ZVS::getInstance()->server_setupZVS(pServer);
  // Start the GATT server. Required to be called after setup of all services and 
  // characteristics / descriptors for the NimBLE host to register them.
  LOG("Server is setup and started!"); 
  pServer->start();
  LOG("Setting up the Server advertising payload(s)");
  setupAdvertising();
}

void NimBLEManager::setupAdvertising(void) {
  // Prepare for advertising
  pAdvertising = NimBLEDevice::getAdvertising();
  if(pAdvertising->addServiceUUID(UUID16_SVC_CYCLING_POWER))
    LOG("Setting Service in Advertised data to    [CPS]");
  if(pAdvertising->addServiceUUID(UUID16_SVC_CYCLING_SPEED_AND_CADENCE))
    LOG("Setting Service in Advertised data to    [CSC]");

  if(pAdvertising->addServiceUUID(ZWIFT_VIRTUAL_SHIFTING_SERVICE))
    LOG("Setting Service in Advertised data to    [ZVS]");

  // Put 128-bit UUID in Scan Response
  NimBLEAdvertisementData scanResp;
  if (scanResp.addServiceUUID(UUID_TACX_FEC_PRIMARY_SERVICE))
    LOG("Setting Service in Scan Response to      [FEC]");
  if (pAdvertising->setScanResponseData(scanResp))
    LOG("Setting Scan Response Data in Advertised data");

  if(pAdvertising->setAppearance(GAS::getInstance()->client_GA_Appearance_Value))
    LOG("Setting Appearance in Advertised data to [%d]", GAS::getInstance()->client_GA_Appearance_Value);

  if(pAdvertising->setName(THISDEVICENAME))
    LOG("Setting DeviceName in Advertised data to [%s]", THISDEVICENAME);
  //Add the transmission power level to the advertisement packet.
  if(pAdvertising->addTxPower())
    LOG("Setting Transmission Power in Advertised data");
  pAdvertising->enableScanResponse(true);
  LOG("Scan Response Enabled!");
  pAdvertising->setMinInterval(AdvertiseMinInterval);
  pAdvertising->setMaxInterval(AdvertiseMaxInterval);
}

void NimBLEManager::startAdvertising(void) {
  if(NimBLEDevice::startAdvertising()) LOG("ESP32 Server is advertising!");
  else LOG("ESP32 Server failed to start advertising!");
}

bool NimBLEManager::isAdvertising(void) {
  if(pAdvertising) {
    return pAdvertising->isAdvertising();
  } else return false;
}

void NimBLEManager::serverConnectionCallbacksOnConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
  NimBLEDevice::stopAdvertising();
  serverConnectionHandle = connInfo.getConnHandle();
  pServer->updateConnParams(serverConnectionHandle, ConnectionMinInterval, ConnectionMaxInterval,
                                                                  ConnectionLatency, ConnectionTimeout);
#ifdef DEBUG
  // Get some connection parameters of the peer device.
  memcpy(&serverPeerAddress, connInfo.getIdAddress().getBase()->val, 6);
  uint8_t addressType = connInfo.getIdAddress().getBase()->type;
  char fullMacAddress[18] = {};
  UTILS::ConvertMacAddress(fullMacAddress, serverPeerAddress, false);  // true -> little endian representation!
  uint16_t peer_MTU = pServer->getPeerMTU(serverConnectionHandle);
  LOG("ESP32 Server connected to Client device with MAC Address: [%s][%d] MTU: [%d] Handle: [%d]",
      fullMacAddress, addressType, peer_MTU, serverConnectionHandle);
  LOG("Waiting for Client Device to set 'Notify/Indicate' enabled for relevant Characteristics...");
#endif
  serverIsConnected = true;
  if (clientIsConnected)
    clientSubscribeToAll(true);  // Tell the client to send data now!
};


void NimBLEManager::serverConnectionCallbacksOnDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  serverIsConnected = false;  // to disable loop() a.s.a.p.
  // Get some Disconnection parameters of the peer device.
  uint16_t ConnectionHandle = connInfo.getConnHandle();
  uint8_t RemoteAddress[6] = {};
  memcpy(&RemoteAddress, connInfo.getIdAddress().getBase()->val, 6);
  char fullMacAddress[18] = {};                                    //
  UTILS::ConvertMacAddress(fullMacAddress, RemoteAddress, false);  // true -> little endian representation!
  LOG("ESP32 Server disconnected from Client device with MAC Address: [%s] Handle: [%d] Reason: [%d]", fullMacAddress,
                                                                                              ConnectionHandle, reason);
  LOG(" -> Server is advertising again!");
  serverConnectionHandle = BLE_HS_CONN_HANDLE_NONE;
  if (clientIsConnected)
    clientSubscribeToAll(false);  // Tell the client to stop sending data now!
};

void NimBLEManager::clientConnectionCallbacksOnConnect(NimBLEClient* pClient) {
  /* Client initiates request to set the desired MTU immediately after connecting.
  if (pClient->exchangeMTU()) {
    LOG("Client Initiates MTU Exchange Request!");
    delay(20);  // Give some time to settle
  }
  */
  NimBLEAddress MyAddress = trainerDevice->getAddress();
  clientPeerName = trainerDevice->getName().c_str();
  memcpy(&clientPeerAddress, MyAddress.getBase()->val, 6);
  uint8_t addressType = MyAddress.getBase()->type;
#ifdef DEBUG
  uint16_t client_MTU = pClient->getMTU();
  uint16_t client_ConnInterval = pClient->getConnInfo().getConnInterval();
  uint16_t client_ConnLatency = pClient->getConnInfo().getConnLatency();
  uint16_t client_ConnTimeout = pClient->getConnInfo().getConnTimeout();
  char fullMacAddress[18] = {};                                        //
  UTILS::ConvertMacAddress(fullMacAddress, clientPeerAddress, false);  // true -> Native Format
  LOG("ESP32 Client connected to Server device with Name: [%s] MAC Address: [%s][%d]",
      clientPeerName.c_str(), fullMacAddress, addressType);
  LOG(" -> Connection Parameters: Interval: [%d] Latency: [%d] Timeout: [%d] MTU: [%d]",
      client_ConnInterval, client_ConnLatency, client_ConnTimeout, client_MTU);
#endif
};

void NimBLEManager::clientConnectionCallbacksOnDisconnect(NimBLEClient* pClient, int reason) {
  clientIsConnected = false;
  clientIsDiscoveringServices = false;
  char fullMacAddress[18] = {};
  UTILS::ConvertMacAddress(fullMacAddress, clientPeerAddress, false);  // true -> Native Format
  LOG("Client Disconnected from Server device with Name: [%s] Mac Address: [%s]!",
      clientPeerName.c_str(), fullMacAddress);
#ifdef CONFIG_NIMBLE_CPP_ENABLE_RETURN_CODE_TEXT
  LOG(" -> [%d][%s]\n", reason, NimBLEUtils::returnCodeToString(reason));
#endif
  LOG("Client Restarts Scanning for Server Device (Tacx)!");
  // Start scanning for undetermined time span -> 0
  // isContinue = false -> clear previous scan results, restart = false
  pNimBLEScan->start(0, false, false);  // NO AUTO RESTART !!
};

void NimBLEManager::xTaskClientSubscribeAll(void* parameter) {
  NimBLEManager* clientInstance = (NimBLEManager *)parameter;
  CPS::getInstance()->client_CP_Subscribe();
  CSC::getInstance()->client_CSC_Subscribe();
  FEC::getInstance()->client_FEC_Subscribe();
  clientInstance->hasSubscribedToAll = true;  // Set true after all have been subscribed to !!
  LOG("Client Subscribed to Peripheral (Trainer)!");
  vTaskDelete(clientInstance->xTaskClientSubscribeUnsubscribeHandle);
};  // Subscribe

void NimBLEManager::xTaskClientUnSubscribeAll(void* parameter) {
  NimBLEManager* clientInstance = (NimBLEManager *)parameter;
  FEC::getInstance()->setTrainerInNeutral();
  CPS::getInstance()->client_CP_Unsubscribe();
  CSC::getInstance()->client_CSC_Unsubscribe();
  FEC::getInstance()->client_FEC_Unsubscribe();
  clientInstance->hasSubscribedToAll = false;  // Set false after all have been unsubscribed to !!
  LOG("Client Unsubscribed from Peripheral (Trainer)!");
  vTaskDelete(clientInstance->xTaskClientSubscribeUnsubscribeHandle);
};  // UnSubscribe

void NimBLEManager::clientSubscribeToAll(bool isEnable) {
  if (isEnable)
    xTaskCreatePinnedToCore(this->xTaskClientSubscribeAll, "xTaskClientSubscribeAll", 4096, (void*)this, 10,
                            &this->xTaskClientSubscribeUnsubscribeHandle, xTaskCoreID0);
  else
    xTaskCreatePinnedToCore(this->xTaskClientUnSubscribeAll, "xTaskClientUnSubscribeAll", 4096, (void*)this, 10,
                            &this->xTaskClientSubscribeUnsubscribeHandle, xTaskCoreID0);
};

bool NimBLEManager::clientConnectServer(void) {
  // Connect to the TACX BLE Server.
  // Handle first time connect AND a reconnect. One fixed Peripheral (trainer) to account for!
  if (pClient == nullptr) {  // First time -> create new pClient_Wahoo and service database!
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new client_Connection_Callbacks(this));
    // First Time Connect to the TACX Trainer (Server/Peripheral)
    hasConnectPassed = pClient->connect(trainerDevice, true);     // Delete attribute objects and Create service database
  } else if (pClient == NimBLEDevice::getDisconnectedClient()) {  // Allow for a streamlined reconnect
    // Reconnect to the disconnected Trainer (Server/Peripheral)
    hasConnectPassed = pClient->connect(trainerDevice, false);  // Just refresh the service database
  }

  if (!hasConnectPassed)
    return hasConnectPassed;  // Connect failed!

  clientIsDiscoveringServices = true;
  LOG("Now checking all Client Services and Characteristics!");
  LOG("If Mandatory Services Fail --> the Client will disconnect!");
  // Discover all relevant Services and Char's
  if (!GAS::getInstance()->client_GenericAccess_Connect(pClient)) {
    pClient->disconnect();
    return false;
  }
  if (!DIS::getInstance()->client_DeviceInformation_Connect(pClient)) {
    pClient->disconnect();
    return false;
  }
  if (!CPS::getInstance()->client_CyclingPower_Connect(pClient)) {
    pClient->disconnect();
    return false;
  }
  if (!CSC::getInstance()->client_CyclingSpeedCadence_Connect(pClient)) {
    pClient->disconnect();
    return false;
  }
  if (!FEC::getInstance()->client_FitnessEquipmentCycling_Connect(pClient)) {
    pClient->disconnect();
    return false;
  }
  // Discovering Services is done!
  clientIsDiscoveringServices = false;
  // TACX is connected !
  clientIsConnected = true;

  // send the FE-C request to receive the Trainer capabilities
  if (!FEC::getInstance()->writeFECCapabilitiesRequest()) {
    LOG("Error writing FEC capabilities request");
  }

  return true;
}

void NimBLEManager::xTaskClientConnectServer(void* parameter) {
  NimBLEManager* clientInstance = (NimBLEManager *)parameter; // LOCAL Pointer to store the NimBLEManager class instance
  if (!clientInstance->clientConnectServer()) {
    LOG(">>> Failed to connect Peripheral (Trainer)!");
    NimBLEDevice::deleteClient(clientInstance->pClient);  // If connected: disconnect, delete client object and clear from list
    clientInstance->pClient = nullptr;   // Clear to null
  }
  vTaskDelete(clientInstance->xTaskClientConnectServerHandle);
};

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class clientScanCallbacks: public NimBLEScanCallbacks {
private:
    NimBLEManager* clientInstance;  // LOCAL Pointer to store the NimBLEManager class instance
public:
    // Constructor to accept a pointer to the NimBLEManager class instance
    clientScanCallbacks(NimBLEManager* instance) : clientInstance(instance) {}
protected:
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    clientInstance->clientScanCallbacksOnResult(advertisedDevice);
  }
}; // clientScanCallbacks

void NimBLEManager::clientScanCallbacksOnResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    //LOG("BLE Advertised Device found: %s", advertisedDevice->toString().c_str());
    // We have found a device, let us now see if it contains the Tacx FE-C service we are looking for.
    if ( (advertisedDevice->getAppearance() == BLE_APPEARANCE_GENERIC_CYCLING) && \
         (advertisedDevice->isAdvertisingService(UUID_TACX_FEC_PRIMARY_SERVICE)) ) {
      NimBLEDevice::getScan()->stop();
      trainerDevice = advertisedDevice;  // NIMBLE --> Just save the reference now, no need to copy the object
      // Create dedicated xTask and make the client connection work!
      xTaskCreatePinnedToCore(this->xTaskClientConnectServer, "Client Connect", 4096, (void *)this, 15,
                              &this->xTaskClientConnectServerHandle, xTaskCoreID0);
    }  // Found our server
};

void NimBLEManager::startScanning(void) {
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start in loop()
  pNimBLEScan = NimBLEDevice::getScan();
  pNimBLEScan->setScanCallbacks(new clientScanCallbacks(this));
  pNimBLEScan->setInterval(ScanInterval);
  pNimBLEScan->setWindow(ScanWindow);
  pNimBLEScan->setActiveScan(true);
  LOG("\nClient Starts Scanning for Server Device (Tacx)!");
  // Start scanning for undetermined time span -> 0
  // isContinue = false -> clear previous scan results, restart = false
  pNimBLEScan->start(0, false, false);  // NO AUTO RESTART !!
}

bool NimBLEManager::isScanning(void) {
  if(pNimBLEScan) {
    return (pNimBLEScan->isScanning() || clientIsDiscoveringServices);
  } else return false;
}
