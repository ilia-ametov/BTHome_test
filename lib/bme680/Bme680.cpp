#include <Bme680.h>
#include <EEPROM.h>

/* Configure the BSEC library with information about the sensor
    18v/33v = Voltage at Vdd. 1.8V or 3.3V
    3s/300s = BSEC operating mode, BSEC_SAMPLE_RATE_LP or BSEC_SAMPLE_RATE_ULP
    4d/28d = Operating age of the sensor in days
    generic_18v_3s_4d
    generic_18v_3s_28d
    generic_18v_300s_4d
    generic_18v_300s_28d
    generic_33v_3s_4d
    generic_33v_3s_28d
    generic_33v_300s_4d
    generic_33v_300s_28d
*/
const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

#define STATE_SAVE_PERIOD UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

bool Bme680::begin() {
	bool isInitializationSuccess = false;
	EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1); // 1st address for the length

	// Wire.begin() must be called before
	this->iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
	if (this->checkIaqSensorStatus()) {
		this->iaqSensor.setConfig(bsec_config_iaq);
		if (this->checkIaqSensorStatus()) {
			this->loadSensorState();

			bsec_virtual_sensor_t sensorList[7] = {
				// BSEC_OUTPUT_RAW_TEMPERATURE,
				BSEC_OUTPUT_RAW_PRESSURE,
				// BSEC_OUTPUT_RAW_HUMIDITY,
				// BSEC_OUTPUT_RAW_GAS,
				BSEC_OUTPUT_IAQ,
				BSEC_OUTPUT_STATIC_IAQ,
				BSEC_OUTPUT_CO2_EQUIVALENT,
				BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
				BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
				BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
			};

			this->iaqSensor.updateSubscription(sensorList, sizeof(sensorList) / sizeof(sensorList[0]), BSEC_SAMPLE_RATE_LP);
			isInitializationSuccess = this->checkIaqSensorStatus();
		}
	}

	return isInitializationSuccess;
}

bool Bme680::run() {
	bool hasNewData = this->iaqSensor.run();
	if (hasNewData) { // If new data is available;
		Serial.printf(
		    "Temperature %.2f*C\tHumidity %.2f%\tPressure %.2f hPa\tIAQ %.0f (%d)\tStatic IAQ %.0f (%d)\n",
		    this->iaqSensor.temperature,
		    this->iaqSensor.humidity,
		    this->iaqSensor.pressure / 100,
		    this->iaqSensor.iaq, this->iaqSensor.iaqAccuracy,
		    this->iaqSensor.staticIaq, this->iaqSensor.staticIaqAccuracy);
		
		Serial.printf(
		    "CO2 equiv %.2f (%d)\tBreath VOC %.2f (%d)\tComp Gas Value %.2f (%d)\tGas Percentage %.2f (%d)\tGasResistance %.2f\n\n",
		    this->iaqSensor.co2Equivalent, this->iaqSensor.co2Accuracy,
		    this->iaqSensor.breathVocEquivalent, this->iaqSensor.breathVocAccuracy,
		    this->iaqSensor.compGasValue, this->iaqSensor.compGasAccuracy,
		    this->iaqSensor.gasPercentage, this->iaqSensor.gasPercentageAcccuracy,
			this->iaqSensor.gasResistance);

		this->saveSensorStateIfNeeded();
	} else {
		this->checkIaqSensorStatus();
	}
	return hasNewData;
}

bool Bme680::hasNotRecoverableError() {
	return this->notRecoverableError;
}

bool Bme680::hasWarning() {
	return this->iaqSensor.status > BSEC_OK || this->iaqSensor.bme680Status > BME680_OK;
}

bsec_library_return_t Bme680::getStatus() {
	return this->iaqSensor.status;
}

bool Bme680::checkIaqSensorStatus() {
	if (this->iaqSensor.status != BSEC_OK) {
		if (this->iaqSensor.status < BSEC_OK) {
			Serial.println("BSEC error code : " + String(this->iaqSensor.status));
			this->notRecoverableError = true;
		} else {
			Serial.println("BSEC warning code: " + String(this->iaqSensor.status));
			this->lastWarningCode = String(this->iaqSensor.status);
		}
	}

	if (this->iaqSensor.bme680Status != BME680_OK) {
		if (this->iaqSensor.bme680Status < BME680_OK) {
			Serial.println("BME680 error code: " + String(this->iaqSensor.bme680Status));
			this->notRecoverableError = true;
		} else {
			Serial.println("BME680 warning code: " + String(this->iaqSensor.bme680Status));
			this->lastBme680WarningCode = String(this->iaqSensor.bme680Status);
		}
	}
	if (!this->notRecoverableError) {
		this->iaqSensor.status = BSEC_OK;
	}
	return !this->notRecoverableError;
}

void Bme680::loadSensorState(void) {
	if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
		// Existing state in EEPROM
		Serial.println("Reading state from EEPROM");

		for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
			bsecState[i] = EEPROM.read(i + 1);
			Serial.println(bsecState[i], HEX);
		}

		this->iaqSensor.setState(bsecState);
		this->checkIaqSensorStatus();
	} else {
		// Erase the EEPROM with zeroes
		Serial.println("Erasing EEPROM");

		for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++) {
			EEPROM.write(i, 0);
		}

		EEPROM.commit();
	}
}

void Bme680::saveSensorStateIfNeeded(void) {
	bool update = false;
	/* Set a trigger to save the state. Here, the state is saved every STATE_SAVE_PERIOD with the first state being saved once the algorithm achieves full calibration, i.e. iaqAccuracy = 3 */
	if (this->stateUpdateCounter == 0) {
		if (this->iaqSensor.iaqAccuracy >= 3) {
			update = true;
			this->stateUpdateCounter++;
		}
	} else {
		/* Update every STATE_SAVE_PERIOD milliseconds */
		if ((this->stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
			update = true;
			this->stateUpdateCounter++;
		}
	}

	if (update) {
		this->iaqSensor.getState(this->bsecState);
		this->checkIaqSensorStatus();

		Serial.println("Writing state to EEPROM");

		for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
			EEPROM.write(i + 1, this->bsecState[i]);
			Serial.println(this->bsecState[i], HEX);
		}

		EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
		EEPROM.commit();
	}
}
