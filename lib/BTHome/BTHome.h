#ifndef BT_HOME_H
#define BT_HOME_H

#include <string>

namespace bthome
{
	enum class DataType : uint8_t
	{
		packetId = 0x00,
		battery = 0x01,

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

		void addTemperature(float const data);
		void addHumidity(float const data);
		void addPressure(float const data);
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