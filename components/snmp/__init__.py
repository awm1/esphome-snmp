from esphome.const import CONF_ID
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import CORE

CODEOWNERS = ["@aquaticus"]

# No ethernet support at the moment
DEPENDENCIES = []

snmp_ns = cg.esphome_ns.namespace("snmp")
SNMPComponent = snmp_ns.class_("SNMPComponent", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SNMPComponent),
            cv.Optional("contact", default=""): cv.string_strict,
            cv.Optional("location", default=""): cv.string_strict,
            cv.Optional("sensor_count", default=""): cv.int_range(min=0, max=999),
        }
    ),
    cv.only_with_arduino,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add(var.set_location(config["location"]))
    cg.add(var.set_contact(config["contact"]))
    cg.add(var.set_sensor_count(config["sensor_count"]))

    await cg.register_component(var, config)

    if CORE.is_esp8266 or CORE.is_esp32:
        cg.add_library(r"https://github.com/aquaticus/Arduino_SNMP.git", "2.1.0")
