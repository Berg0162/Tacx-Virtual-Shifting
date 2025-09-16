#include "FitnessEquipmentCycling.h"
#include "config/configTacx.h"
#include "NimBLEManager.h"

// ------------------------------------------------------------------------------------------------
// COMPILER DIRECTIVE to allow/suppress DEBUG messages that help debugging...
// Uncomment general "#define DEBUG" to activate debug messages for this class
#define DEBUG
// Include these debug utility macros in all cases!
#include "config/configDebug.h"
// ------------------------------------------------------------------------------------------------

#ifdef DEBUG
//  Restrict activating one or more of the following DEBUG directives --> process intensive 
//  The overhead can lead to spurious side effects and a loss of quality of service handling!!
//  Notice: Training Apps hardly ever subscribe to server_FEC_Rxd_Chr, they use CPS and CSC data instead!
//#define DEBUG_FEC_RXD
//#define DEBUG_FEC_DECODE
#endif

constexpr size_t kPacketSize = 13;

// Generic ANT+ packet
typedef struct {  // — no padding
    uint8_t HEADER[4];  // 4 bytes
    uint8_t BODY[8];    // 8 bytes
    uint8_t CHCKSM;     // 1 byte
} packet_data_t;

typedef union {
    packet_data_t values;
    uint8_t bytes[kPacketSize];
} packet_data_ut;

// Data Page 16 (0x10) – General FE Data
typedef struct {   // — no padding
    uint8_t pageNumber;        // BODY[0] Data Page Number
    uint8_t equipBitField;     // BODY[1] Equipment Type Bit Field
    uint8_t elapsedTime;       // BODY[2] Elapsed Time
    uint8_t distTravelled;     // BODY[3] Distance Travelled
    uint8_t lsbSpeed;          // BODY[4] Speed LSB
    uint8_t msbSpeed;          // BODY[5] Speed MSB
    uint8_t heartRate;         // BODY[6] Heart Rate
    uint8_t bitField;  			   // BODY[7] Capabilities Bit Field + FE State Bit Field
} page_0x10_payload_t;

// Data Page 25 (0x19) – Specific Trainer/Stationary Bike Data
typedef struct {   // — no padding
    uint8_t pageNumber;        // BODY[0] Data Page Number
    uint8_t updateEventCount;  // BODY[1] Update Event Count
    uint8_t instantCadence;    // BODY[2] Instantaneous Cadence
    uint8_t lsbAccumPower;     // BODY[3] Accumulated Power LSB
    uint8_t msbAccumPower;     // BODY[4] Accumulated Power MSB
    uint8_t lsbInstantPower;   // BODY[5] Instantaneous Power LSB (1.5 bytes)
    uint8_t msbInstantPower;   // BODY[6] Instantaneous Power MSN (bits 0-3) + Trainer Status Bit Field (bits 4-7)
    uint8_t bitField;  			   // BODY[7] Flags Bit Field + FE State Bit Field 
} page_0x19_payload_t;

// Common Page 71 (0x47) – Command Status
// --> SPECIFIC for Requested page number 51 (0x33) – Track Resistance
typedef struct {   // — no padding
    uint8_t pageNumber;      // BODY[0] Data Page Number
    uint8_t reqPageNum;      // BODY[1] Last Received Command ID -> requested page number 51 (0x33)
    uint8_t seqNumber;       // BODY[2] Sequence # 
    uint8_t comStatus;       // BODY[3] Command Status
    uint8_t reserved1;       // BODY[4] Reserved. Set to 0xFF
    uint8_t gradeLSB;        // BODY[5] Grade (Slope) LSB
    uint8_t gradeMSB;        // BODY[6] Grade (Slope) MSB
    uint8_t crr;  			     // BODY[7] Coefficient of Rolling Resistance
} page_0x47_payload_t;

// Data Page 51 (0x33) – Track Resistance (inside BODY[0..7])
typedef struct {   // — no padding
    uint8_t pageNumber;      // BODY[0] Data Page Number
    uint8_t reserved1;       // BODY[1] 0xFF (reserved for future use)
    uint8_t reserved2;       // BODY[2] 0xFF (reserved for future use)
    uint8_t reserved3;       // BODY[3] 0xFF (reserved for future use)
    uint8_t reserved4;       // BODY[4] 0xFF (reserved for future use)
    uint8_t gradeLSB;        // BODY[5] Grade (Slope) LSB
    uint8_t gradeMSB;        // BODY[6] Grade (Slope) MSB
    uint8_t crr;  			     // BODY[7] Coefficient of Rolling Resistance
} page_0x33_payload_t;

// Data Page 54 (0x36) - FE Capabilities
typedef struct {   // — no padding
    uint8_t pageNumber;      // BODY[0] Data Page Number
    uint8_t reserved1;       // BODY[1] Reserved. Set to 0xFF
    uint8_t reserved2;       // BODY[2] Reserved. Set to 0xFF
    uint8_t reserved3;       // BODY[3] Reserved. Set to 0xFF
    uint8_t reserved4;       // BODY[4] Reserved. Set to 0xFF
    uint8_t maxResLSB;       // BODY[5] Maximum Resistance LSB
    uint8_t maxResMSB;       // BODY[6] Maximum Resistance MSB
    uint8_t bitField;  			 // BODY[7] Capabilities Bit Field
} page_0x36_payload_t;

enum FECPageType : uint8_t {
    GENERAL_FE_DATA = 0x10,      // Data Page 16 (0x10) – General FE Data
    STATIONARY_BIKE_DATA = 0x19, // Data Page 25 (0x19) – Specific Trainer/Stationary Bike Data
    TRACK_RESISTANCE = 0x33,     // Data Page 51 (0x33) – Track Resistance
    COMMAND_STATUS = 0x47,       // Common Page 71 (0x47) – Command Status
    FE_CAPABILITIES = 0x36       // Data Page 54 (0x36) - FE Capabilities
};

///////////////////////////////////////////////
/////////// TACX FE-C ANT+ over BLE ///////////
///////////////////////////////////////////////
const NimBLEUUID UUID_TACX_FEC_PRIMARY_SERVICE("6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E");
const NimBLEUUID UUID_TACX_FEC_RXD_CHARACTERISTIC("6E40FEC2-B5A3-F393-E0A9-E50E24DCCA9E");
const NimBLEUUID UUID_TACX_FEC_TXD_CHARACTERISTIC("6E40FEC3-B5A3-F393-E0A9-E50E24DCCA9E");

class server_FEC_Rxd_Chr_callbacks final : public NimBLECharacteristicCallbacks {
private:
    FEC* fecInstance;  // LOCAL Pointer to the FEC class instance
public:
    // Constructor to accept a pointer to the FEC class instance
    server_FEC_Rxd_Chr_callbacks(FEC* instance) : fecInstance(instance) {}
protected:
    inline void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        fecInstance->serverFECRxdOnSubscribe(pCharacteristic, connInfo, subValue);
    } // onSubscribe
}; 

class server_FEC_Txd_Chr_callback final : public NimBLECharacteristicCallbacks {
private:
    FEC* fecInstance;  // LOCAL Pointer to the FEC class instance
public:
    // Constructor to accept a pointer to the FEC class instance
    server_FEC_Txd_Chr_callback(FEC* instance) : fecInstance(instance) {}
protected:
    inline void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        fecInstance->serverFECTxdOnWrite(pCharacteristic, connInfo);
    }
};

// Initialize the static member
FEC* FEC::instance = nullptr;

/*  We only define onSubscribe !!!
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo);
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo); 
    void onStatus(NimBLECharacteristic* pCharacteristic, int code)); 

    Notice: Training Apps hardly ever subscribe to server_FEC_Rxd_Chr, they use CPS and CSC data instead!   
*/ 
void FEC::serverFECRxdOnSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) {
    std::string str = std::string(pCharacteristic->getUUID());
    if(subValue == 0) {
      LOG("Central Unsubscribed: [%s] FEC Rxd", str.c_str());
      isServerFECRxdNotifyEnabled = false;
    } else {
      LOG("Central Subscribed: [%s] FEC Rxd", str.c_str());
      isServerFECRxdNotifyEnabled = true;
    }
}

void FEC::serverFECTxdOnWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) {
  std::string FEC_Txd_Data = server_FEC_Txd_Chr->getValue();
  // Generic Packet size is 13 bytes (header 4, body 8 and checksum 1)
  if (FEC_Txd_Data.length() != kPacketSize) {
    LOG("Invalid packet size: expected %d, got %d", kPacketSize, FEC_Txd_Data.length());
    return;
  }

  // Transfer to the client side first
  if(pRemote_FEC_Txd_Chr && NimBLEManager::getInstance()->clientIsConnected)
    pRemote_FEC_Txd_Chr->writeValue(FEC_Txd_Data, false); // NO Response

  // Decode Rec'd FEC_Txd_Data packet and interpret
  packet_data_ut server_Data;
  memcpy(server_Data.bytes, FEC_Txd_Data.data(), kPacketSize);
  const auto& body = server_Data.values.BODY;	// Create an alias (reference)
  uint8_t pageValue = body[0]; 		  			    // Extract data page number
  
#ifdef DEBUG_FEC_DECODE
  // Parse Generic ANT+ packet for documentation only
  std::string FEC_Txd_Header = FEC_Txd_Data.substr(0, 4); // 4 bytes for header
  std::string FEC_Txd_Body = FEC_Txd_Data.substr(4, 8);   // 8 bytes for body
  // Convert header and body to HEX presentation
  std::string hexHeader = UTILS::getHexString(FEC_Txd_Header);
  std::string hexBody = UTILS::getHexString(FEC_Txd_Body);
  LOG(" -> Server Rec'd FEC Txd Data Page: %d(0x%02X) - [%s][%s][0x%02X]", \
		    pageValue, pageValue, hexHeader.c_str(), hexBody.c_str(), server_Data.values.CHCKSM);
#endif

  switch(pageValue) {
    case TRACK_RESISTANCE : 
	  // Interpret data page 0x33 content -> Overlay first the specific page 0x33 struct
	  const page_0x33_payload_t* page33 = reinterpret_cast<const page_0x33_payload_t*>(&body); 
	  uint16_t rawGrade = (static_cast<uint16_t>(page33->gradeMSB) << 8) | page33->gradeLSB;
    float gradePercentValue = (static_cast<float>(rawGrade - 20000))/100.0;   // Units: 0.01
#ifdef DEBUG_FEC_DECODE
	  float crr = (static_cast<float>(page33->crr))*5/100000.0;  // Units: 5x0.000001 - Range: 0.0 – 0.0127
    LOG("    Raw Grade: %05d Grade Percentage: %6.2f%% crr: %6.4f", rawGrade, gradePercentValue, crr);
#endif 
  }
};

// Constructor
FEC::FEC() { }

// Destructor
FEC::~FEC() { }

FEC* FEC::getInstance() {
    if (FEC::instance == nullptr) {
        FEC::instance = new FEC();
    }
    return FEC::instance;
}

void FEC::server_setupFEC(NimBLEServer* pServer) {
  server_FitnessEquipmentCycling_Service = pServer->createService(UUID_TACX_FEC_PRIMARY_SERVICE);
  server_FEC_Rxd_Chr = server_FitnessEquipmentCycling_Service->createCharacteristic(UUID_TACX_FEC_RXD_CHARACTERISTIC, \
                                                                                                   NIMBLE_PROPERTY::NOTIFY);
  server_FEC_Rxd_Chr->setCallbacks(new server_FEC_Rxd_Chr_callbacks(this));
  server_FEC_Txd_Chr = server_FitnessEquipmentCycling_Service->createCharacteristic(UUID_TACX_FEC_TXD_CHARACTERISTIC, \
                                                                        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  server_FEC_Txd_Chr->setCallbacks(new server_FEC_Txd_Chr_callback(this));
  server_FitnessEquipmentCycling_Service->start();    
}

void FEC::server_FEC_Rxd_Chr_Notify(uint8_t* pData, size_t length) {
  if(isServerFECRxdNotifyEnabled) {
    server_FEC_Rxd_Chr->setValue(pData, length);
    server_FEC_Rxd_Chr->notify();
  }
}

void FEC::client_FEC_Rxd_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, \
                                                                                  size_t length, bool isNotify) {
  // The FE-C Read charateristic of ANT+ packets
  // Generic Packet size is 13 bytes (header 4, body 8 and checksum 1)
  if (length != kPacketSize) {
    LOG("Invalid packet size: expected %d, got %d", kPacketSize, length);
    return;
  } 
  // Client FEC Rxd data is tranferred to the Server (if enabled!)
  /*
   * Notice: Training Apps hardly ever subscribe to server_FEC_Rxd_Chr, they use CPS and CSC data instead!
  */
  if(NimBLEManager::getInstance()->serverIsConnected) {
    server_FEC_Rxd_Chr_Notify(pData, length);
  }

  // ------------------------------------------------------------------
  // Decode Rec'd FEC_Rxd_Data packet and interpret
  packet_data_ut server_Data;
  //Serial.printf("Rec'd Raw FE-C Data len: [%02d] [", kPacketSize);
  for (int i = 0; i < kPacketSize; i++) {
      server_Data.bytes[i] = *pData++;
      //Serial.printf("%02X ", server_Data.bytes[i], HEX);
  }
  //Serial.print("] ");

  const auto& body = server_Data.values.BODY;	// Create an alias (reference)
  uint8_t pageValue = body[0]; 		  			    // Extract data page number

  switch(pageValue) {
    case COMMAND_STATUS : {
    ////////////////////////////////////////////////////////////
    /////// Handle Common Page 71(0x47) – Command Status ///////
    ///////////// Requested PAGE 51 for grade info /////////////
    ////////////////////////////////////////////////////////////
    // Interpret data page 71(0x47) content -> Overlay first the specific struct
	  const page_0x47_payload_t* page47 = reinterpret_cast<const page_0x47_payload_t*>(&body);
    // check for requested page Track Resistance 
    if ( page47->reqPageNum == TRACK_RESISTANCE ) { 
      uint16_t rawGrade = (static_cast<uint16_t>(page47->gradeMSB) << 8) | page47->gradeLSB;
      float gradePercentValue = (static_cast<float>(rawGrade - 20000))/100.0;   // Units: 0.01
#ifdef DEBUG_FEC_DECODE
	    float crr = (static_cast<float>(page47->crr))*5/100000.0;  // Units: 5x0.000001 - Range: 0.0 – 0.0127
      LOG("Page: %d(0x%02X) Requested Page 51(0x33) - Raw Grade: %05d Grade Percentage: %6.2f%% crr: %6.4f", \
                      pageValue, pageValue, rawGrade, gradePercentValue, crr);
#endif 
      }
    }  
    break;
  case STATIONARY_BIKE_DATA : {
    ////////////////////////////////////////////////////////////////////////
    //  Handle Data Page 25(0x19) – Specific Trainer/Stationary Bike Data //
    ////////////////////////////////////////////////////////////////////////
    // Interpret data page 25(0x19) content -> Overlay first the specific struct
	const page_0x19_payload_t* page19 = reinterpret_cast<const page_0x19_payload_t*>(&body);    
    uint8_t UpdateEventCount = page19->updateEventCount;
    trainerInstantCadence = page19->instantCadence;
    // POWER is stored in 1.5 byte !!! msbInstantPower, bits 0:3 --> Most Significant Nibble only!!!
    trainerInstantPower = (static_cast<uint16_t>(page19->msbInstantPower & 0x0F) << 8) | page19->lsbInstantPower;
#ifdef TACXNEO_FIRSTGENERATION
	trainerInstantCadence = UTILS::getFilteredCadence(trainerInstantCadence, trainerInstantPower);
#endif
    /*
    Serial.printf("Page: %d(0x%02X) Bike Data - Event count: [%03d]", pageValue, pageValue, UpdateEventCount); 
    Serial.printf(" - Cadence: [%03d]", trainerInstantCadence);
    Serial.printf(" - Power in Watts: [%04d]", trainerInstantPower);
    LOG();
    */
    }
    break;
  case GENERAL_FE_DATA :  {
    //////////////////////////////////////////////////
    //  Handle Data Page 16(0x10) – General FE Data //
    //////////////////////////////////////////////////
    // Interpret data page 16(0x10) content -> Overlay first the specific struct
	  const page_0x10_payload_t* page10 = reinterpret_cast<const page_0x10_payload_t*>(&body);
    uint8_t ElapsedTime = static_cast<uint8_t>((page10->elapsedTime/4)%60); // units of 0.25 seconds --> 256 Rollover (every 64 seconds)
    uint8_t DistanceTravelled = page10->distTravelled; // in meters 256 m rollover 
    trainerInstantSpeed = ((static_cast<uint16_t>(page10->msbSpeed) << 8) | page10->lsbSpeed); // in units of 0,001 m/s
    /*
    Serial.printf("Page: %d(0x%02X) General Data - Elapsed time: [%02d]s", pageValue, pageValue, ElapsedTime);
    Serial.printf(" - Distance travelled: [%05d]m", DistanceTravelled); 
    Serial.printf(" - Speed: [%02d]km/h", (trainerInstantSpeed/1000)*3.6);
    LOG();
    */
    }
    break;
  case FE_CAPABILITIES : {
    //////////////////////////////////////////////////
    //  Handle Data Page 54(0x36) – FE Capabilities //
    //////////////////////////////////////////////////
    // Interpret data page 54(0x36) content -> Overlay first the specific struct
    const page_0x36_payload_t* page36 = reinterpret_cast<const page_0x36_payload_t*>(&body);
    trainerMaximumResistance = ((static_cast<uint16_t>(page36->maxResMSB) << 8) | page36->maxResLSB);
#ifdef DEBUG_FEC_DECODE
    LOG("Page: %d(0x%02X) FE Capabilities - Trainer Maximum Resistance: [%d]", pageValue, pageValue, trainerMaximumResistance);
#endif
    }
    break;
  default : {
    if(calculateChecksum(server_Data.bytes, kPacketSize-1) != server_Data.values.CHCKSM)
      LOG("⚠️  Invalid checksum in packet: discarding!");
    /*else 
      LOG("⚠️  Page %d(0x%02X) unknown: discarding!", pageValue, pageValue);*/
    }
  } // Switch
  //////////////////////// DONE! /////////////////////////
}

uint16_t FEC::getInstantPower(void) {
  return trainerInstantPower;
}

uint8_t FEC::getInstantCadence(void){
  return trainerInstantCadence;
}

uint16_t FEC::getInstantSpeed(void){
  return trainerInstantSpeed;
}

void FEC::Static_FEC_Rxd_Notify_Callback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, \
                                                                          uint8_t* pData, size_t length, bool isNotify) {
    if (instance) {
        instance->client_FEC_Rxd_Notify_Callback(pBLERemoteCharacteristic, pData, length, isNotify);
    }
}

bool FEC::client_FitnessEquipmentCycling_Connect(NimBLEClient* pClient) {
    // Obtain a reference to the remote FEC service.
    pRemote_FitnessEquipmentCycling_Service = pClient->getService(UUID_TACX_FEC_PRIMARY_SERVICE);
    if (pRemote_FitnessEquipmentCycling_Service == nullptr) {
      LOG("Cycling Speed Cadence Service: Not Found!");
      return false;
    }
    LOG("Client_FitnessEquipmentCycling_Service: Found!");
    pRemote_FEC_Rxd_Chr = pRemote_FitnessEquipmentCycling_Service->getCharacteristic(UUID_TACX_FEC_RXD_CHARACTERISTIC);
    if (pRemote_FEC_Rxd_Chr == nullptr) {
      LOG("Mandatory client_FEC_Rxd_Chr: Not Found!");
      return false;
    }
    LOG("Client_FEC_Rxd_Chr: Found!");  
    if(!pRemote_FEC_Rxd_Chr->canNotify()) {
      LOG("Mandatory Client_FEC_Rxd_Chr: Cannot Notify!");
      return false;
    }
    pRemote_FEC_Txd_Chr = pRemote_FitnessEquipmentCycling_Service->getCharacteristic(UUID_TACX_FEC_TXD_CHARACTERISTIC);
    if (pRemote_FEC_Txd_Chr == nullptr) {
      LOG("Mandatory Client_FEC_Txd_Chr: Not Found!");
      return false;     
    }
    LOG("Client_FEC_Txd_Chr: Found!");
    if(!pRemote_FEC_Txd_Chr->canWrite()) {
      LOG("Mandatory Client_FEC_Rxd_Chr: Cannot Write!");
      return false;
    }
    return true;    
}

void FEC::client_FEC_Subscribe(void) {
  if( pRemote_FEC_Rxd_Chr != nullptr ) 
    if( !pRemote_FEC_Rxd_Chr->subscribe(notifications, Static_FEC_Rxd_Notify_Callback) ) 
      LOG(">>> ERROR remote FEC Rxd Subscribe failed!");                        
}

void FEC::client_FEC_Unsubscribe(void) {
  if ( pRemote_FEC_Rxd_Chr != nullptr )
    if( !pRemote_FEC_Rxd_Chr->unsubscribe(false) )
      LOG(">>> ERROR remote FEC Rxd Subscribe failed!"); 
}

uint8_t FEC::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

bool FEC::write_Client_FEC_Txd_Chr(std::vector<uint8_t>* data) {

  if(!NimBLEManager::getInstance()->clientIsConnected) return false;

  // value needs to be copied to this scope as it will be handled asynchronously by another core and NimBLE will only write garbage
  std::vector<uint8_t> valueData;
  for (auto currentByte = data->begin(); currentByte != data->end(); currentByte++) {
    valueData.push_back(*currentByte);
  }
  if(pRemote_FEC_Txd_Chr->writeValue(valueData.data(), valueData.size(), false)) {
    return true;
  }
  return false;
}

uint8_t FEC::getFECChecksum(std::vector<uint8_t>* fecData) {
  uint8_t checksum = 0;
  if (fecData->size() > 0) {
    checksum = fecData->at(0);
    if (fecData->size() > 1) {
      for (size_t checksumIndex = 1; checksumIndex < fecData->size(); checksumIndex++) {
        checksum = (checksum ^ fecData->at(checksumIndex));
      }
    }
  }
  return checksum;
}

// Define Packet for Common Page 70 (0x46) – To Request for Data Page 51 (0x33) Track Resistance
bool FEC::writeFECCommonPage70Req51(void) {  
    std::vector<uint8_t> fecData;
    fecData.push_back(0xA4); //Sync
    fecData.push_back(0x09); //Length
    fecData.push_back(0x4F); //Acknowledge message type
    fecData.push_back(0x05); //Channel 
    fecData.push_back(0x46); //Common Page 70
    fecData.push_back(0xFF); //Reserved (0xFF for no value)
    fecData.push_back(0xFF); //Reserved (0xFF for no value)
    fecData.push_back(0xFF); //Reserved (0xFF for no value)
    fecData.push_back(0xFF); //Reserved (0xFF for no value)
    fecData.push_back(0x80); //Requested transmission response
    fecData.push_back(0x33); //Requested page number 51 (0x33)
    fecData.push_back(0x01); //Command type (0x01 for request data page, 0x02 for request ANT-FS session)
    fecData.push_back(0x47); //Fixed Value Checksum
    //LOG("Write FEC Common Page [70] (0x46) with Request for Data Page [51] (0x33)");
    return write_Client_FEC_Txd_Chr(&fecData);
}

bool FEC::writeFECTargetPower(uint16_t targetPower) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4E);  // MSG_ID
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x31);  // PAGE 49 (0x31)
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back((uint8_t)targetPower);
  fecData.push_back((uint8_t)(targetPower >> 8));
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return write_Client_FEC_Txd_Chr(&fecData);
}

bool FEC::writeFECUserConfiguration(uint16_t bicycleWeight, uint16_t userWeight, uint8_t wheelDiameter, uint8_t gearRatio) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4F);  // MSG_ID -> ACKNOWLEDGED
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x37);  // PAGE 55 (0x37)
  fecData.push_back((uint8_t)userWeight);
  fecData.push_back((uint8_t)(userWeight >> 8));
  fecData.push_back(0xFF);
  fecData.push_back((uint8_t)bicycleWeight << 4);
  fecData.push_back((uint8_t)(bicycleWeight >> 8));
  fecData.push_back(wheelDiameter);
  fecData.push_back(gearRatio);
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return write_Client_FEC_Txd_Chr(&fecData);
}

bool FEC::writeFECTrackResistance(uint16_t grade, uint8_t rollingResistance) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4E);  // MSG_ID
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x33);  // PAGE 51 (0x33)
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back((uint8_t)grade);
  fecData.push_back((uint8_t)(grade >> 8));
  fecData.push_back(rollingResistance);
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return write_Client_FEC_Txd_Chr(&fecData);
}

bool FEC::writeFECCapabilitiesRequest(void) {
  std::vector<uint8_t> fecData;
  fecData.push_back(0xA4);  // SYNC
  fecData.push_back(0x09);  // MSG_LEN
  fecData.push_back(0x4F);  // MSG_ID
  fecData.push_back(0x05);  // CONTENT_START
  fecData.push_back(0x46);  // PAGE 70 (0x46)
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0xFF);
  fecData.push_back(0x80);  // REQ TRANSM RESPONSE
  fecData.push_back(0x36);  // PAGE 54 (0x36)
  fecData.push_back(0x01);  // COMMAND_TYPE_RDP
  fecData.push_back(getFECChecksum(&fecData));  // CHECKSUM
  return write_Client_FEC_Txd_Chr(&fecData);
}

void FEC::setTrainerInNeutral(void) {
    if (!writeFECTrackResistance((uint16_t)(UTILS::FEC_ZERO), (uint8_t)round(UTILS::rollingResistanceCoefficient*20000)) ) { 
      LOG("Error writing FEC track resistance to zero");
    }
}

void FEC::updateTrainerResistance(UTILS::TrainerMode zwiftTrainerMode, uint64_t zwiftPower, int64_t zwiftGrade, \
                                    uint64_t zwiftGearRatio, uint16_t zwiftBicycleWeight, uint16_t zwiftUserWeight) {
  // SIM Mode (NO VS), use simple Track Resistance
  if (zwiftTrainerMode == UTILS::TrainerMode::SIM_MODE) {
    if (!writeFECTrackResistance((uint16_t)(UTILS::FEC_ZERO + zwiftGrade), (uint8_t)round(UTILS::rollingResistanceCoefficient*20000)) ) { 
      LOG("Error writing SIM Mode FEC track resistance");
    }
    return;
  }
  if (zwiftTrainerMode == UTILS::TrainerMode::SIM_MODE_VIRTUAL_SHIFTING) {
    // SIM Mode + VS -> use calculated Track Resistance
    uint16_t trainerTrackResistanceGrade = UTILS::calculateFECTrackResistanceGrade( (zwiftBicycleWeight + zwiftUserWeight) / 100.0, \
                                  zwiftGrade / 100.0, trainerInstantSpeed / 1000.0, trainerInstantCadence, zwiftGearRatio / 10000.0);
    if (!writeFECTrackResistance(trainerTrackResistanceGrade, (uint8_t)round(UTILS::rollingResistanceCoefficient*20000)) ) { 
      LOG("Error writing SIM Mode + VS FEC track resistance");
    }
    LOG("Trainer Simulated Grade: %6.2f", (static_cast<float>(trainerTrackResistanceGrade - 20000))/100.0);
    return;
  }
  if (zwiftTrainerMode == UTILS::TrainerMode::ERG_MODE) { 
    // ERG Mode -> Write Target Power   
    // FE-C target power is in 0.25W unit
    if (!writeFECTargetPower((zwiftPower * 4))) {
      LOG("Error writing ERG Mode FEC target power");
    }
  }
}

