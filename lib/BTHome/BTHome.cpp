#include <algorithm>
#include <bthome.h>

bool serviceDataItemCompareFn(bthome::ServiceDataItem x1, bthome::ServiceDataItem x2)
{
	return x1.dataType < x2.dataType;
}

namespace bthome
{

	PayloadBuilder::PayloadBuilder(std::string deviceName) : deviceName(deviceName) {}

	void PayloadBuilder::addCo2(uint16_t const data)
	{
		this->addServiceDataItem(DataType::co2, data);
	}

	void PayloadBuilder::addCount(uint8_t const data)
	{
		this->addServiceDataItem(DataType::count1, data);
	}

	void PayloadBuilder::addCount(uint16_t const data)
	{
		this->addServiceDataItem(DataType::count2, data);
	}

	void PayloadBuilder::addCount(uint32_t const data)
	{
		this->addServiceDataItem(DataType::count3, data);
	}

	void PayloadBuilder::addTvoc(uint16_t const data)
	{
		this->addServiceDataItem(DataType::tvoc, data);
	}

	void PayloadBuilder::addTemperature(float const data)
	{
		this->addServiceDataItem(DataType::temperature, static_cast<uint64_t>(data * 100.0f));
	}

	void PayloadBuilder::addHumidity(float const data)
	{
		this->addServiceDataItem(DataType::humidity, static_cast<uint64_t>(data * 100.0f));
	}

	void PayloadBuilder::addPressure(float const data)
	{
		this->addServiceDataItem(DataType::pressure, static_cast<uint64_t>(data * 100.0f));
	}

	void PayloadBuilder::addGenericBoolean(bool const data)
	{
		this->addServiceDataItem(DataType::genericBoolean, data);
	}

	std::string PayloadBuilder::getAdvertisingPayload()
	{
		std::string advertisingPayload = "";
		advertisingPayload += (char)this->deviceName.length() + 1; // Length + 1 here as we need to capture the null terminator
		advertisingPayload += (char)0x09;						   // TODO: use constant. Type = Complete local name
		advertisingPayload += this->deviceName;
		advertisingPayload += this->getServiceData();
		return advertisingPayload;
	}

	void PayloadBuilder::resetServiceData()
	{
		this->serviceDataPayloadSize = 0;
		delete[] this->serviceDataPayload;
	}

	void PayloadBuilder::addServiceDataItem(DataType dataType, uint64_t scaledDataValue)
	{
		ServiceDataItem *previousServiceDataPayload = this->serviceDataPayload;
		this->serviceDataPayload = new ServiceDataItem[this->serviceDataPayloadSize + 1];
		for (uint8_t i = 0; i < this->serviceDataPayloadSize; i++)
		{
			this->serviceDataPayload[i] = previousServiceDataPayload[i];
		}
		this->serviceDataPayloadSize++;
		this->serviceDataPayload[this->serviceDataPayloadSize - 1] = {dataType, scaledDataValue};
	}

	std::string PayloadBuilder::getServiceData()
	{
		std::string serviceData = "";
		serviceData += 0x16; // Type = Service Data - 16-bit UUID

		// UUID (D2FC):
		serviceData += 0xD2;
		serviceData += 0xFC;

		// BTHome Device Information (0x40): not encrypted, BTHome version 2
		serviceData += 0x40;

		// Object ids have to be applied in numerical order (from low to high) in your advertisement.
		// This will make sure that if you have a device (sensor) that is broadcasting a new measurement type
		// that is added in a new (minor) BTHome update, while your BTHome receiver isn't updated yet to the same version,
		// it will still be able to receive the older supported measurement types.
		// A BTHome receiver will stop parsing object ids as soon as it finds an object id that isn't supported.
		std::sort(this->serviceDataPayload, this->serviceDataPayload + this->serviceDataPayloadSize, serviceDataItemCompareFn);

		uint8_t serviceDataSize = 4; // Type (Service Data - 16-bit UUID) + UUID + BTHome version 2
		for (uint8_t i = 0; i < this->serviceDataPayloadSize; i++)
		{
			serviceData += (char)this->serviceDataPayload[i].dataType;
			serviceDataSize += 1; // Data Type size
			switch (this->serviceDataPayload[i].dataType)
			{
			case DataType::packetId:
			case DataType::battery:
			case DataType::count1:
			case DataType::genericBoolean:
				serviceData += (char)(this->serviceDataPayload[i].dataValue & 0xFF);
				serviceDataSize += 1;
				break;
			case DataType::co2:
			case DataType::count2:
			case DataType::temperature:
			case DataType::humidity:
			case DataType::tvoc:
				serviceData += (char)(this->serviceDataPayload[i].dataValue & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 8) & 0xFF);
				serviceDataSize += 2;
				break;
			case DataType::pressure:
				serviceData += (char)(this->serviceDataPayload[i].dataValue & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 8) & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 16) & 0xFF);
				serviceDataSize += 3;
				break;
			case DataType::count3:
				serviceData += (char)(this->serviceDataPayload[i].dataValue & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 8) & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 16) & 0xFF);
				serviceData += (char)((this->serviceDataPayload[i].dataValue >> 24) & 0xFF);
				serviceDataSize += 4;
				break;
			}
		}
		serviceData = (char)serviceDataSize + serviceData;

		return serviceData;
	}

}