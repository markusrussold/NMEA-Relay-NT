#ifndef AISPROCESSOR_H
#define AISPROCESSOR_H

class AisProcessor
{
private:
	std::string rawText;

public:
	AisProcessor()
	{
		rawText = "";
	}

	std::string EncodeTo6Bit(const std::string& input);
	std::string encodeStringTo6Bit(const std::string& input, size_t size);
	std::string CalculateChecksum(const std::string& sentence);
	std::string CreateNMEASentence(const std::string& encodedData);
	std::string EncodeToAISCharacters(const std::string& binaryString);
	int32_t encodeCoordinate(float coordinate, bool isLongitude);
	std::string createAisPositionMessage(int mmsi, int status, double lat, double lon, float speed, int course, int heading, int timestamp);
	std::string ccreateAisMessageType5(int mmsi, int imoNumber, const std::string& callSign,
		const std::string& vesselName, int shipType,
		int dimensionToBow, int dimensionToStern,
		int dimensionToPort, int dimensionToStarboard,
		int epfdType, int etaMonth, int etaDay, int etaHour,
		int etaMinute, int maxDraught, const std::string& destination);
	std::string stringToUpperCase(const std::string& input);
	std::string createAisMessageType18(int mmsi, double speedOverGround, bool positionAccuracy,
		double longitude, double latitude, double courseOverGround,
		int trueHeading, int timeStamp);
	std::string createAisMessageType24A(int mmsi, const std::string& vesselName);
};

#endif // AISPROCESSOR_H