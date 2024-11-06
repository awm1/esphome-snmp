#pragma once

#include <string>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "SNMP_Agent.h"

#ifdef USE_ESP32
#include <WiFi.h>
#include <esp32/himem.h>
#endif
#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <WiFiUdp.h>

namespace esphome {
namespace snmp {

/// The SNMP (Simple Network Management Protocol) component provides support for collecting and organizing
/// information about managed devices on a networks.

class SNMPComponent : public Component {
 public:
  SNMPComponent() : snmp_agent_("public", "private"){};
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void loop() override;

  void set_contact(const std::string &contact) { contact_ = contact; }

  void set_location(const std::string &location) { location_ = location; }

  void set_sensor_count(const int sensor_count) { sensor_count_ = sensor_count; }

 protected:
  WiFiUDP udp_;
  SNMPAgent snmp_agent_;
  
  void setup_system_mib_();
  void setup_storage_mib_();
#ifdef USE_ESP32
  void setup_esp32_heap_mib_();
#endif
#ifdef USE_ESP8266
  void setup_esp8266_heap_mib_();
#endif
  void setup_chip_mib_();
#ifdef USE_ESP32
  static int setup_psram_size(int *used);
#endif
  static uint32_t get_uptime() { return millis() / 10; }

  static uint32_t get_net_uptime();

#ifdef USE_ESP32
  static int get_ram_size_kb();
#endif


 void custom_vars_();
 static std::string  getSensor_();
 static std::string  getTemperature_(const int index);
 static std::string  getAC_(const int index);
 static std::string  getTemperature1_();
 static std::string  getTemperature2_();
 static std::string  getTemperature3_();
 static std::string  getTemperature4_();
 static std::string  getTemperature5_();
 static std::string  getTemperature6_();
 static std::string  getTemperature7_();
 static std::string  getTemperature8_();

  /// contact string
  std::string contact_;

  /// location string
  std::string location_;

  /// sensor count
  int sensor_count_;
};

}  // namespace snmp
}  // namespace esphome
