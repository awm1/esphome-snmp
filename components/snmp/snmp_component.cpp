#include "snmp_component.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/version.h"
#ifdef USE_ESP32
#include "esphome/components/ethernet/ethernet_component.h"
#endif
#ifdef USE_ESP8266
#include "esphome/components/wifi/wifi_component.h"
#endif

// Integration test available: https://github.com/aquaticus/esphome_snmp_tests

namespace esphome {
namespace snmp {

#define CUSTOM_OID ".1.3.9999."

static const char *const TAG = "snmp";

/// @brief Returns network uptime
/// @return time in hundreds of seconds
/// @warning 
/// Function returns real value if @c wifi_connected_timestamp() function
/// was implemented in WiFi module.
uint32_t SNMPComponent::get_net_uptime() {
  return 0; //not available
}

void SNMPComponent::setup_system_mib_() {
  // sysDesc
  const char *desc_fmt = "ESPHome version " ESPHOME_VERSION " compiled %s, Board " ESPHOME_BOARD;
  char description[128];
  snprintf(description, sizeof(description), desc_fmt, App.get_compilation_time().c_str());
  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysDescr, description);

  // sysName
  snmp_agent_.addDynamicReadOnlyStringHandler(RFC1213_OID_sysName, []() -> std::string { return App.get_name(); });

  // sysServices
  snmp_agent_.addReadOnlyIntegerHandler(RFC1213_OID_sysServices, 64 /*=2^(7-1) applications*/);

  // sysObjectID
  snmp_agent_.addOIDHandler(RFC1213_OID_sysObjectID,
#ifdef USE_ESP32
                            CUSTOM_OID "32"
#else
                            CUSTOM_OID "8266"
#endif
  );

  // sysContact
  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysContact, contact_);

  // sysLocation
  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysLocation, location_);
}

#ifdef USE_ESP32
/// @brief Gets PSI RAM size and usage
/// @param used Pointer to a location where to store used memory value
/// @return Size of PSI RAM or 0 if not present
int SNMPComponent::setup_psram_size(int *used) {
  int total_size = 0;
  *used = 0;

  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  bool available = free_size > 0;

  if (available) {
    total_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    if (total_size > 0) {
      *used = total_size - free_size;
    }
  }

  return total_size;
}
#endif

void SNMPComponent::setup_storage_mib_() {
  //  hrStorageIndex
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.1.1", 1);

  // hrStorageDesc
  snmp_agent_.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.25.2.3.1.3.1", "FLASH");

  // hrAllocationUnit
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.4.1", 1);

  // hrStorageSize
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.1",
                                       []() -> int { return ESP.getFlashChipSize(); });  // NOLINT

  // hrStorageUsed
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.1",
                                       []() -> int { return ESP.getSketchSize(); });  // NOLINT

  // SPI RAM

  //  hrStorageIndex
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.1.2", 2);

  // hrStorageDesc
  snmp_agent_.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.25.2.3.1.3.2", "SPI RAM");

  // hrAllocationUnit
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.4.2", 1);

#ifdef USE_ESP32
  // hrStorageSize
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.2", []() -> int {
    int u;
    return setup_psram_size(&u);
  });

  // hrStorageUsed
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.2", []() -> int {
    int u;
    setup_psram_size(&u);
    return u;
  });
#endif




#ifdef USE_ESP8266
  // hrStorageSize
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.2", 0);

  // hrStorageUsed
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.2", 0);

#endif

  // hrMemorySize [kb]
#ifdef USE_ESP32
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.2", get_ram_size_kb());
#endif

#ifdef USE_ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.2", 160);
#endif
}

#if USE_ESP32
void SNMPComponent::setup_esp32_heap_mib_() {
  // heap size
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.1.0", []() -> int { return ESP.getHeapSize(); });

  // free heap
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.2.0", []() -> int { return ESP.getFreeHeap(); });

  // min free heap
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.3.0", []() -> int { return ESP.getMinFreeHeap(); });

  // max alloc heap
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.4.0", []() -> int { return ESP.getMaxAllocHeap(); });
}
#endif

#ifdef USE_ESP8266
void SNMPComponent::setup_esp8266_heap_mib_() {
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.1.0", []() -> int { return ESP.getFreeHeap(); });  // NOLINT

  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.2.0",
                                       []() -> int { return ESP.getHeapFragmentation(); });  // NOLINT

  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.3.0",
                                       []() -> int { return ESP.getMaxFreeBlockSize(); });  // NOLINT
}
#endif

void SNMPComponent::setup_chip_mib_() {
  // esp32/ esp8266
#if ESP32
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.1.0", 32);
#endif
#if ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.1.0", 8266);
#endif

  // CPU clock
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.2.0", []() -> int { return ESP.getCpuFreqMHz(); });  // NOLINT

  // chip model
#if ESP32
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "2.3.0", []() -> std::string { return ESP.getChipModel(); });
#endif
#ifdef USE_ESP8266
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "2.3.0",
                                              []() -> std::string { return ESP.getCoreVersion().c_str(); });  // NOLINT
#endif

  // number of cores
#if USE_ESP32
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.4.0", []() -> int { return ESP.getChipCores(); });
#endif
#if USE_ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.4.0", 1);
#endif

  // chip id
#if ESP32
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.5.0", []() -> int { return ESP.getChipRevision(); });
#endif
#if ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.5.0", 0 /*no data for ESP8266*/);
#endif


}


std::string SNMPComponent::getSensor_() {
  char buf[30];
  //float temp1 = id("temp1");
   float temp1 = 123123;
    //ESP_LOGI("main", "Raw Value of my sensor: %f", id("temp1"));

auto sensors = App.get_sensors();
  ESP_LOGD("app", "Sensor: %s", sensors[1]->get_name().c_str());
  ESP_LOGD("app", "Sensor state: %f", sensors[1]->state);





  sprintf(buf, "%f", temp1 );
  return buf;
} 




std::string SNMPComponent::getTemperature_(const int index) {
  char buf[30];

  auto sensors = App.get_sensors();

  if (sensors.size() > index) {
    ESP_LOGD("app", "Sensor: %s", sensors[index]->get_name().c_str());
    ESP_LOGD("app", "Sensor state: %f", sensors[index]->state);
    sprintf(buf, "%f", sensors[index]->state );
  } else {
    sprintf(buf, "%f", 0.0 );
  }

  return buf;
}


std::string SNMPComponent::getAC_(const int index) {
  char buf[30];
/*
auto pins = App.get_binary_sensors();
  ESP_LOGD("app", "Sensor: %s", pins[index]->get_name().c_str());
  ESP_LOGD("app", "Sensor state: %i", pins[index]->state);


  sprintf(buf, "%i", pins[index]->state );
*/

  return buf;
}

std::string SNMPComponent::getTemperature1_() {
  return getTemperature_(0);
}

std::string SNMPComponent::getTemperature2_() {
  return getTemperature_(1);
}

std::string SNMPComponent::getTemperature3_() {
  return getTemperature_(2);
}

std::string SNMPComponent::getTemperature4_() {
  return getTemperature_(3);
}

std::string SNMPComponent::getTemperature5_() {
  return getTemperature_(4);
}

std::string SNMPComponent::getTemperature6_() {
  return getTemperature_(5);
}

std::string SNMPComponent::getTemperature7_() {
  return getTemperature_(6);
}

std::string SNMPComponent::getTemperature8_() {
  return getTemperature_(7);
}

void SNMPComponent::custom_vars_() {
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.1", getTemperature1_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.2", getTemperature2_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.3", getTemperature3_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.4", getTemperature4_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.5", getTemperature5_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.6", getTemperature6_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.7", getTemperature7_);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "5.1.8", getTemperature8_);
}






void SNMPComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SNMP...");

  // sysUpTime
  // this is uptime of network management part of the system
  // WARNING: Only available if wifi_connected_timestamp() function was implemented
  // in WiFi module.
  // By default returns always 0
  snmp_agent_.addDynamicReadOnlyTimestampHandler(RFC1213_OID_sysUpTime, get_net_uptime);

  // hrSystemUptime
  snmp_agent_.addDynamicReadOnlyTimestampHandler(".1.3.6.1.2.1.25.1.1.0", get_uptime);

  custom_vars_();
  setup_system_mib_();
  setup_storage_mib_();
#if USE_ESP32
  setup_esp32_heap_mib_();
#endif
#if USE_ESP8266
  setup_esp8266_heap_mib_();
#endif
  setup_chip_mib_();


  snmp_agent_.sortHandlers();  // for walk to work properly

  snmp_agent_.setUDP(&udp_);
  
  snmp_agent_.begin();
}

void SNMPComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SNMP Config: Test");
  ESP_LOGCONFIG(TAG, "  Contact: \"%s\"", contact_.c_str());
  ESP_LOGCONFIG(TAG, "  Location: \"%s\"", location_.c_str());
  //ESP_LOGCONFIG(TAG, "  Sensor count: \"%s\"", sensor_count_);
}


void SNMPComponent::loop() { snmp_agent_.loop(); }





#if USE_ESP32
int SNMPComponent::get_ram_size_kb() {
  // use hardcoded values (number of values in esp_chip_model_t depends on IDF version)
  // from esp_system.h
  const int chip_esp32 = 1;
  const int chip_esp32_s2 = 2;
  const int chip_esp32_s3 = 9;
  const int chip_esp32_c3 = 5;
  const int chip_esp32_h2 = 6;
  const int chip_esp32_c2 = 12;
  const int chip_esp32_c6 = 13;

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  switch ((int) chip_info.model) {
    case chip_esp32:
      return 520;

    case chip_esp32_s2:
      return 320;

    case chip_esp32_s3:
      return 512;

    case chip_esp32_c2:
    case chip_esp32_c3:
    case chip_esp32_c6:
      return 400;

    case chip_esp32_h2:
      return 256;
  }

  return 0;
}
#endif

}  // namespace snmp
}  // namespace esphome
