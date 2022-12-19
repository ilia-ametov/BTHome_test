#ifndef Bme680_H
#define Bme680_H

#include "bsec.h"

/*struct Bme680Data {
	float temperature;
	float humidity;
	float pressure;
	float iaq;
	uint8_t iaqAccuracy;
	float staticIaq;
	uint8_t staticIaqAccuracy;
	float gasResistance;
};*/

class Bme680 {
      public:
	bool begin();
	bool run();
	bool hasNotRecoverableError();
	bool hasWarning();
	bsec_library_return_t getStatus();
	String lastWarningCode;
	String lastBme680WarningCode;
	Bsec iaqSensor;

      private:
	uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
	uint16_t stateUpdateCounter = 0;
	bool notRecoverableError = false;
	
	bool checkIaqSensorStatus();
	void loadSensorState();
	void saveSensorStateIfNeeded();
};

#endif