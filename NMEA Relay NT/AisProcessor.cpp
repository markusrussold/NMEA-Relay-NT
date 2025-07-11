#include "pch.h"
#include <unordered_map>
#include <bitset>
#include "AisProcessor.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype> // for std::toupper
#include <string>

std::string AisProcessor::EncodeTo6Bit(const std::string& input)
{
    // AIS character map
    std::unordered_map<wchar_t, int> aisMap = {
        {L'@', 0},  {L'A', 1},  {L'B', 2},  {L'C', 3},  {L'D', 4},
        {L'E', 5},  {L'F', 6},  {L'G', 7},  {L'H', 8},  {L'I', 9},
        {L'J', 10}, {L'K', 11}, {L'L', 12}, {L'M', 13}, {L'N', 14},
        {L'O', 15}, {L'P', 16}, {L'Q', 17}, {L'R', 18}, {L'S', 19},
        {L'T', 20}, {L'U', 21}, {L'V', 22}, {L'W', 23}, {L'X', 24},
        {L'Y', 25}, {L'Z', 26}, {L'[', 27}, {L'\\', 28}, {L']', 29},
        {L'^', 30}, {L'_', 31}, {L' ', 32}, {L'!', 33}, {L'"', 34},
        {L'#', 35}, {L'$', 36}, {L'%', 37}, {L'&', 38}, {L'\'', 39},
        {L'(', 40}, {L')', 41}, {L'*', 42}, {L'+', 43}, {L',', 44},
        {L'-', 45}, {L'.', 46}, {L'/', 47}, {L'0', 48}, {L'1', 49},
        {L'2', 50}, {L'3', 51}, {L'4', 52}, {L'5', 53}, {L'6', 54},
        {L'7', 55}, {L'8', 56}, {L'9', 57}, {L':', 58}, {L';', 59},
        {L'<', 60}, {L'=', 61}, {L'>', 62}, {L'?', 63}
    };

    std::string encodedString;
    for (const wchar_t& ch : input) {
        // Look up the 6-bit value
        int value = aisMap[ch];
        // Convert the value to a 6-bit binary string
        std::bitset<6> bits(value);
        // Append the binary string to the encoded string
        encodedString += bits.to_string();
    }

    return encodedString;
}

std::string AisProcessor::encodeStringTo6Bit(const std::string& input, size_t size)
{
    std::string encoded;
    for (size_t i = 0; i < size; ++i) {
        if (i < input.size()) {
            char ch = input[i];
            int value = 0;
            if (ch >= 'A' && ch <= 'W') value = ch - 'A' + 1;
            else if (ch >= '0' && ch <= '9') value = ch - '0' + 48;
            else if (ch == ' ') value = 32;
            else if (ch == '@') value = 0;
            else value = ch; // Handle other ASCII characters if needed
            encoded += std::bitset<6>(value).to_string();
        }
        else {
            encoded += std::bitset<6>(32).to_string(); // Pad with spaces
        }
    }
    return encoded;
}

std::string AisProcessor::CalculateChecksum(const std::string& sentence)
{
    unsigned char checksum = 0;
    for (size_t i = 1; i < sentence.size(); ++i) {
        checksum ^= sentence[i];
    }
    std::stringstream ss;
    ss << "*" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(checksum);
    return ss.str();
}

std::string AisProcessor::CreateNMEASentence(const std::string& encodedData)
{
    std::string sentence = "!AIVDM,1,1,,A," + encodedData + ",0";
    sentence += CalculateChecksum(sentence);
    return sentence;
}

std::string AisProcessor::EncodeToAISCharacters(const std::string& binaryString)
{
    std::string aisEncoded;
    for (size_t i = 0; i < binaryString.length(); i += 6) {
        std::bitset<6> bits(binaryString.substr(i, 6));
        int value = static_cast<int>(bits.to_ulong());

        if (value < 40) {
            aisEncoded += static_cast<char>(value + 48);  // 0-9, A-W
        }
        else {
            aisEncoded += static_cast<char>(value + 56);  // X-Z, 'blank', [\]^_
        }
    }
    return aisEncoded;
}

int32_t AisProcessor::encodeCoordinate(float coordinate, bool isLongitude)
{
    int32_t encodedValue;
    if (isLongitude) {
        encodedValue = static_cast<int32_t>(coordinate * 600000.0);
    }
    else {
        encodedValue = static_cast<int32_t>(coordinate * 600000.0);
    }
    return encodedValue;
}

std::string AisProcessor::createAisPositionMessage(int mmsi, int status, double lat, double lon, float speed, int course, int heading, int timestamp)
{
    std::bitset<6> messageType(1); // Message Type 1 (Position Report)
    std::bitset<2> repeatIndicator(0); // Repeat Indicator (0)
    std::bitset<30> mmsiBits(mmsi); // MMSI
    std::bitset<4> navigationStatus(status); // Navigation Status (0 for under way using engine)
    std::bitset<8> rateOfTurn(128); // Rate of Turn (128 means no turn data available)
    std::bitset<10> speedOverGround(static_cast<int>(speed * 10)); // Speed Over Ground
    std::bitset<1> positionAccuracy(0); // Position Accuracy (0 for low)
    std::bitset<28> longitudeBits(encodeCoordinate(lon, true)); // Longitude
    std::bitset<27> latitudeBits(encodeCoordinate(lat, false)); // Latitude
    std::bitset<12> courseOverGround(static_cast<int>(course * 10)); // Course Over Ground
    std::bitset<9> trueHeading(heading); // True Heading
    std::bitset<6> timeStamp(timestamp); // Timestamp (UTC second)
    std::bitset<2> maneuverIndicator(0); // Maneuver Indicator (0 means not available)
    std::bitset<1> raimFlag(0); // RAIM flag
    std::bitset<19> radioStatus(0); // Radio status (0 means not available)

    std::string binaryMessage = messageType.to_string() +
        repeatIndicator.to_string() +
        mmsiBits.to_string() +
        navigationStatus.to_string() +
        rateOfTurn.to_string() +
        speedOverGround.to_string() +
        positionAccuracy.to_string() +
        longitudeBits.to_string() +
        latitudeBits.to_string() +
        courseOverGround.to_string() +
        trueHeading.to_string() +
        timeStamp.to_string() +
        maneuverIndicator.to_string() +
        raimFlag.to_string() +
        radioStatus.to_string();

    return binaryMessage;
}

std::string AisProcessor::ccreateAisMessageType5(int mmsi, int imoNumber, const std::string& callSign, const std::string& vesselName, int shipType, int dimensionToBow, int dimensionToStern, int dimensionToPort, int dimensionToStarboard, int epfdType, int etaMonth, int etaDay, int etaHour, int etaMinute, int maxDraught, const std::string& destination)
{
    std::bitset<6> messageType(5); // Message Type 5 (Static and Voyage Related Data)
    std::bitset<2> repeatIndicator(0); // Repeat Indicator (0)
    std::bitset<30> mmsiBits(mmsi); // MMSI
    std::bitset<2> aisVersion(0); // AIS Version Indicator
    std::bitset<30> imoNumberBits(imoNumber); // IMO Number
    std::string callSignBits = encodeStringTo6Bit(stringToUpperCase(callSign), 7); // Call Sign (42 bits)
    std::string vesselNameBits = encodeStringTo6Bit(stringToUpperCase(vesselName), 20); // Vessel Name (120 bits)
    std::bitset<8> shipTypeBits(shipType); // Ship Type
    std::bitset<9> toBow(dimensionToBow); // Dimension to Bow (9 bits)
    std::bitset<9> toStern(dimensionToStern); // Dimension to Stern (9 bits)
    std::bitset<6> toPort(dimensionToPort); // Dimension to Port (6 bits)
    std::bitset<6> toStarboard(dimensionToStarboard); // Dimension to Starboard (6 bits)
    std::bitset<4> epfdTypeBits(epfdType); // Type of EPFD
    std::bitset<4> etaMonthBits(etaMonth); // ETA Month (4 bits)
    std::bitset<5> etaDayBits(etaDay); // ETA Day (5 bits)
    std::bitset<5> etaHourBits(etaHour); // ETA Hour (5 bits)
    std::bitset<6> etaMinuteBits(etaMinute); // ETA Minute (6 bits)
    std::bitset<8> draughtBits(maxDraught); // Maximum Draught (8 bits)
    std::string destinationBits = encodeStringTo6Bit(stringToUpperCase(destination), 20); // Destination (120 bits)
    std::bitset<1> dte(0); // DTE (0 means available)
    std::bitset<1> spare(0); // Spare (1 bit)

    std::string binaryMessage = messageType.to_string() +
        repeatIndicator.to_string() +
        mmsiBits.to_string() +
        aisVersion.to_string() +
        imoNumberBits.to_string() +
        callSignBits +
        vesselNameBits +
        shipTypeBits.to_string() +
        toBow.to_string() +
        toStern.to_string() +
        toPort.to_string() +
        toStarboard.to_string() +
        epfdTypeBits.to_string() +
        etaMonthBits.to_string() +
        etaDayBits.to_string() +
        etaHourBits.to_string() +
        etaMinuteBits.to_string() +
        draughtBits.to_string() +
        destinationBits +
        dte.to_string() +
        spare.to_string();

    return binaryMessage;
}

std::string AisProcessor::stringToUpperCase(const std::string& input)
{
    std::string result = input; // Create a copy of the input string

    // Convert each character to uppercase
    for (char& c : result) {
        c = std::toupper(static_cast<unsigned char>(c));
    }

    return result; // Return the uppercase version of the string
}

std::string AisProcessor::createAisMessageType18(int mmsi, double speedOverGround, bool positionAccuracy, double longitude, double latitude, double courseOverGround, int trueHeading, int timeStamp)
{
    std::bitset<6> messageType(18); // Message Type 18
    std::bitset<2> repeatIndicator(0); // Repeat Indicator (0)
    std::bitset<30> mmsiBits(mmsi); // MMSI
    std::bitset<8> reserved(0); // Reserved (8 bits)
    std::bitset<10> speedBits(static_cast<int>(speedOverGround * 10)); // Speed Over Ground
    std::bitset<1> positionAccuracyBit(positionAccuracy); // Position Accuracy
    std::bitset<28> longitudeBits = encodeCoordinate(longitude, true); // Longitude
    std::bitset<27> latitudeBits = encodeCoordinate(latitude, false); // Latitude
    std::bitset<12> courseBits(static_cast<int>(courseOverGround * 10)); // Course Over Ground
    std::bitset<9> headingBits(trueHeading); // True Heading
    std::bitset<6> timeStampBits(timeStamp); // Time Stamp
    std::bitset<2> reserved2(0); // Reserved (2 bits)
    std::bitset<1> csUnit(0); // Class B CS Unit
    std::bitset<1> displayFlag(0); // Display Flag
    std::bitset<1> dscFlag(0); // DSC Flag
    std::bitset<1> bandFlag(0); // Band Flag
    std::bitset<1> msg22Flag(0); // Message 22 Flag
    std::bitset<1> assignedModeFlag(0); // Assigned Mode Flag
    std::bitset<1> raimFlag(0); // RAIM Flag
    std::bitset<19> radioStatus(0); // Radio Status (19 bits)

    // Assemble the binary message
    std::string binaryMessage = messageType.to_string() +
        repeatIndicator.to_string() +
        mmsiBits.to_string() +
        reserved.to_string() +
        speedBits.to_string() +
        positionAccuracyBit.to_string() +
        longitudeBits.to_string() +
        latitudeBits.to_string() +
        courseBits.to_string() +
        headingBits.to_string() +
        timeStampBits.to_string() +
        reserved2.to_string() +
        csUnit.to_string() +
        displayFlag.to_string() +
        dscFlag.to_string() +
        bandFlag.to_string() +
        msg22Flag.to_string() +
        assignedModeFlag.to_string() +
        raimFlag.to_string() +
        radioStatus.to_string();

    return binaryMessage;
}

std::string AisProcessor::createAisMessageType24A(int mmsi, const std::string& vesselName)
{
    std::bitset<6> messageType(24); // Message Type 24
    std::bitset<2> repeatIndicator(0); // Repeat Indicator (0)
    std::bitset<30> mmsiBits(mmsi); // MMSI
    std::bitset<2> partNumber(0); // Part A
    std::string vesselNameBits = encodeStringTo6Bit(stringToUpperCase(vesselName), 20); // Vessel Name (120 bits)
    std::bitset<8> spare(0); // Spare (8 bits)

    // Assemble the binary message
    std::string binaryMessage = messageType.to_string() +
        repeatIndicator.to_string() +
        mmsiBits.to_string() +
        partNumber.to_string() +
        vesselNameBits +
        spare.to_string();

    return binaryMessage;
}

