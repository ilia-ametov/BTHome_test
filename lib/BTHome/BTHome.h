#ifndef BT_HOME_H
#define BT_HOME_H

#include <string>

namespace bthome
{
	enum class DataType : uint8_t
	{
		packetId = 0x00,

		// Data type:	uint8 (1 byte)
		// Factor:		1
		// Example:		0161
		// Result:		97
		// Unit:		%
		battery = 0x01,

		// Data type:	uint16 (2 bytes)
		// Factor:		1
		// Example:		12E204
		// Result:		1250
		// Unit:		ppm
		co2 = 0x12,

		// Data type:	uint (1 bytes)
		// Factor:		1
		// Example:		0960
		// Result:		96
		// Unit:
		count1 = 0x09,

		// Data type:	uint (2 bytes)
		// Factor:		1
		// Example:		3D0960
		// Result:		24585
		// Unit:
		count2 = 0x3D,

		// Data type:	uint (4 bytes)
		// Factor:		1
		// Example:		3E2A2C0960
		// Result:		1611213866
		// Unit:
		count3 = 0x3E,

		// Data type:	uint16 (2 bytes)
		// Factor:		1
		// Example:		133301
		// Result:		307
		// Unit:		ug/m3
		tvoc = 0x13,

		// Data type:	sint16 (2 bytes)
		// Factor:		0.01
		// Example:		02CA09
		// Result:		25.06
		// Unit:		°C
		temperature = 0x02,

		// Data type:	uint16 (2 bytes)
		// Factor:		0.01
		// Example:		03BF13
		// Result:		50.55
		// Unit:		%
		humidity = 0x03,

		// Data type:	uint24 (3 bytes)
		// Factor:		0.01
		// Example:		04138A01
		// Result:		1008.83
		// Unit:		hPa
		pressure = 0x04,

		// Data type:	uint8 (1 byte)
		// Factor:		0.01
		// Example:		0F01
		// Result:		1 (True = On)
		genericBoolean = 0x0F,
	};

	struct ServiceDataItem
	{

		enum DataType dataType;
		uint64_t dataValue; // scaled data value
	};

	class PayloadBuilder
	{
	public:
		PayloadBuilder(std::string deviceName);

		void addCo2(uint16_t const data);
		void addCount(uint8_t const data);
		void addCount(uint16_t const data);
		void addCount(uint32_t const data);
		void addTvoc(uint16_t const data);
		void addTemperature(float const data);
		void addHumidity(float const data);
		void addPressure(float const data);
		void addGenericBoolean(bool const data);
		std::string getAdvertisingPayload();
		void resetServiceData();

	protected:
		std::string deviceName;
		ServiceDataItem *serviceDataPayload = new ServiceDataItem[0];
		uint8_t serviceDataPayloadSize = 0;

		std::string getServiceData();
		void addServiceDataItem(DataType dataType, uint64_t dataValue);
	};

}

#endif