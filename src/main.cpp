#include "NimBLEDevice.h"
#include "NimBLEBeacon.h"
#include "esp_sleep.h"
#include <BTHome.h>

/*
Full message example:
020106 0B094449592D73656E736F72 0A16D2FC4002C40903BF13

Flags: 020106
  0x02 = length (2 bytes)
  0x01 = Flags
  0x06 = in bits, this is 00000110. bit 1: "LE General Discoverable Mode", bit 2: "BR/EDR Not Supported"

Locale Name: 0B094449592D73656E736F72
  0x0B = length (11 bytes)
  0x09 = Complete local name
  0x4449592D73656E736F72 = the complete local name. After converting it to text (https://www.rapidtables.com/convert/number/hex-to-ascii.html), it corresponds to "DIY-sensor". The name can be used to identify your sensor.

Service Data: 0A16D2FC4002C40903BF13
  0X0A = length (10 bytes)
  0x16 = Service Data - 16-bit UUID
  0xD2FC 40 02C409 03BF13 = BTHome data (see below)
    0xD2FC = 16 bit UUID.
      The UUID has to be read reversed per byte, so the UUID is 0xFCD2.
      This UUID should be used by receivers to recognize BTHome messages.
      Allterco Robotics, the manufacturer of Shelly devices,
      has sponsored this UUID for BTHome and it's free to use for everyone with this license.
    0x40 = BTHome Device Information
      The first byte after the UUID 0x40 is the BTHome device info byte,
      which has several bits indicating the capabilities of the device.
      - bit 0: "Encryption flag"
      - bit 1-4: "Reserved for future use"
      - bit 5-7: "BTHome Version"
      In the example, we have 0x40. After converting (https://www.mathsisfun.com/binary-decimal-hexadecimal-converter.html) this to bits,
      we get 01000000. Note. bit 0 is the most right number, bit 7 is the most left number.
      - bit 0: 0 = False: No encryption
      - bit 5-7: 010 = 2: BTHome Version 2. Other version numbers are currently not allowed
    0x02 C409 = Temperature measurement
      - The first measurement is a temperature measurement in the example.
        The first byte 0x02 is defining the type of measurement (temperature, see table below)
      - The remaining bytes 0xC409 comprise the object value (little-endian).
        This value has to be in the format as given in the table below.
        The value will be multiplied with a factor, as given in the table below, to get a sufficient number of digits.
          - According to the table, a 2 bytes long unsigned integer is used in little-endian with a factor 0.01,
            for temperature measurements with object id 0x02.
          - 0xC409 as unsigned integer in little-endian is equal to 2500.
          - The factor for a temperature measurement is 0.01, resulting in a temperature of 25.00°C
    0x03 BF12 = Humidity measurement
      - The second measurement is a humidity measurement in the example.
        The first byte 0x03 is defining the type of measurement (humidity), 2 bytes, as a signed integer (see table below).
      - 0xBF13 as unsigned integer in little-endian is equal to 5055,
        multiplied with a factor 0.01, this becomes a humidity of 50.55%.
Object ids have to be applied in numerical order (from low to high) in your advertisement.
This will make sure that if you have a device (sensor)
that is broadcasting a new measurement type that is added in a new (minor) BTHome update,
while your BTHome receiver isn't updated yet to the same version,
it will still be able to receive the older supported measurement types.
A BTHome receiver will stop parsing object ids as soon as it finds an object id that isn't supported.
*/

#define DEEP_SLEEP_DURATION 1                              // sleep x seconds and then wake up
#define BEACON_UUID "e61bd6a6-f5bd-4004-9256-0843610b9de7" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)

RTC_DATA_ATTR static uint32_t bootCount = 0; // remember number of boots in RTC Memory

BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
bthome::PayloadBuilder btHomePayloadBuilder("HON 203");

void setBeacon()
{
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oAdvertisementData.setFlags(0x06); // this is 00000110. Bit 1 and bit 2 are 1, meaning: Bit 1: “LE General Discoverable Mode” Bit 2: “BR/EDR Not Supported”

  btHomePayloadBuilder.resetServiceData();
  btHomePayloadBuilder.addPressure(1000 + static_cast<float>(bootCount));
  btHomePayloadBuilder.addTemperature(5 + static_cast<float>(bootCount) / 10);
  btHomePayloadBuilder.addHumidity(40 + static_cast<float>(bootCount) / 10);

  std::string payload = btHomePayloadBuilder.getAdvertisingPayload();

  oAdvertisementData.addData(payload);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
  /**  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
   *    Advertising mode. Can be one of following constants:
   *  - BLE_GAP_CONN_MODE_NON (non-connectable; 3.C.9.3.2).
   *  - BLE_GAP_CONN_MODE_DIR (directed-connectable; 3.C.9.3.3).
   *  - BLE_GAP_CONN_MODE_UND (undirected-connectable; 3.C.9.3.4).
   */
  pAdvertising->setAdvertisementType(BLE_GAP_CONN_MODE_NON);
}

void setup()
{
  Serial.begin(115200);
  bootCount++;
  BLEDevice::init(""); // Create the BLE Device

  BLEDevice::setPower(ESP_PWR_LVL_N12);

  setBeacon();
  pAdvertising->start();
  Serial.println("Advertizing started for 10s...");
  delay(10000);
  pAdvertising->stop();
  Serial.printf("Enter deep sleep for %ds\n", DEEP_SLEEP_DURATION);
  esp_deep_sleep(1000000LL * DEEP_SLEEP_DURATION);
  Serial.printf("In deep sleep\n");
}

void loop()
{
}