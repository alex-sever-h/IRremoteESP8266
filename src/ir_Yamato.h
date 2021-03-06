#include <stdint.h>
#include "IRsend.h"

#include <list>
#include <map>
using namespace std;

#define YAMATO_LENGTH 14

class YamatoProperty {
    uint8_t * _byte;
    uint8_t _mask;
    uint8_t _command;
    std::map<const String, uint8_t> _valMap;

public:
YamatoProperty(uint8_t command,
               uint8_t * byte,
               uint8_t mask,
               std::map<const String, uint8_t> valMap) :
    _byte(byte), _mask(mask), _valMap(valMap), _command(command)
        {
//            set(_valMap.begin()->first);
        }

    std::list<String> list(void) {
        std::list<String> all;
        for(std::map<String,uint8_t>::iterator it = _valMap.begin();
            it != _valMap.end(); ++it) {
            all.push_back(it->first);
        }
        return all;
    }

    bool set(const char prop[]) {
        const String sprop(prop);
        return set(sprop);
    }

    bool set(const String prop) {
        uint8_t v;

        if(_valMap.find(prop) == _valMap.end())
            return false;
        v = _valMap.at(prop);

        *_byte &= ~_mask;
        *_byte |= v;

        return true;
    }

    const String get(void) {
        uint8_t v;

        v = (*_byte) & _mask;

        for(std::map<String,uint8_t>::iterator it = _valMap.begin();
            it != _valMap.end(); ++it) {
            uint8_t cmp = it->second;
            if(v == cmp)
                return it->first;
        }
        return "";
    }

    const uint8_t getCmd(void) {return _command;}
};


class IRYamato {
private:
    const uint8_t packetHeader[5] = {0x23, 0xCB, 0x26, 0x01, 0x00};
    uint8_t packetData[YAMATO_LENGTH];

    IRsend _irsend;

    YamatoProperty _powerState;
    YamatoProperty _operationMode;
    YamatoProperty _temperature;
    YamatoProperty _fanSpeed;
    YamatoProperty _verticalPosition;
    YamatoProperty _horizontalPosition;

public:
    typedef enum prop_t {
        POWER_STATE,
        OPERATION_MODE,
        TEMPERATURE,
        FAN_SPEED,
        VERTICAL_POSITION,
        HORIZONTAL_POSITION,
        PROP_CNT
    } prop_t;
private:
    YamatoProperty * _prop[PROP_CNT];

public:
    std::list<String> list (prop_t prop) {
        if(prop >= PROP_CNT)
            return std::list<String>();
        return _prop[prop]->list();
    }

    bool set (prop_t prop, String value)
    {
        if(prop >= PROP_CNT)
            return false;
        if(!_prop[prop]->set(value))
            return false;

        packetData[YAMATO_LENGTH - 2] = _prop[prop]->getCmd();
        packetData[YAMATO_LENGTH - 1] = calcChecksum(packetData);

        _irsend.sendYamato(packetData);

        return true;
    }

    IRYamato(int recvPin, int sendPin): _irsend(sendPin),
        _powerState(0x04, &packetData[5], 0x24, {{"OFF", 0x20}, {"ON", 0x24}}),
        _operationMode(0x09, &packetData[6], 0x0F, {{"AUTO", 0x08}, {"FAN", 0x07},
                                              {"COOL", 0x03}, {"HEAT", 0x01},
                                              {"DEHUMIDIFY", 0x02}}),
        _temperature(0x10, &packetData[7], 0x0F, {{"24", 0x07},
                                            {"31", 0x00},
                                            {"30", 0x01},
                                            {"29", 0x02},
                                            {"28", 0x03},
                                            {"27", 0x04},
                                            {"26", 0x05},
                                            {"25", 0x06},
                                            {"23", 0x08},
                                            {"22", 0x09},
                                            {"21", 0x0A},
                                            {"20", 0x0B},
                                            {"19", 0x0C},
                                            {"18", 0x0D},
                                            {"17", 0x0E},
                                            {"16", 0x0F}}),
        _fanSpeed(0x0D, &packetData[8], 0x07, {{"AUTO", 0x00}, {"LOW", 0x02},
                                         {"MID", 0x03}, {"HIGH", 0x05},
                                         {"TURBO", 0x07}}),
        _verticalPosition(0x19, &packetData[8], 0x38, {{"SWING", 0x38}, {"UP", 0x08},
                                                 {"MIDUP", 0x10}, {"MID", 0x18},
                                                 {"MIDDOWN", 0x20}, {"DOWN", 0x28}}),
        _horizontalPosition(0x1D, &packetData[11], 0x0F, {{"SWING", 0x0F}, {"LEFT", 0x01},
                                                    {"MIDLEFT", 0x02}, {"MID", 0x03},
                                                    {"MIDRIGHT", 0x04}, {"RIGHT", 0x05}})

    {
        memcpy(packetData, packetHeader, sizeof(packetHeader));

        _prop[POWER_STATE] = &_powerState;
        _prop[OPERATION_MODE] = &_operationMode;
        _prop[TEMPERATURE] = &_temperature;
        _prop[FAN_SPEED] = &_fanSpeed;
        _prop[VERTICAL_POSITION] = &_verticalPosition;
        _prop[HORIZONTAL_POSITION] = &_horizontalPosition;

        _operationMode.set("AUTO");
        _temperature.set("24");
        _fanSpeed.set("AUTO");
        _verticalPosition.set("SWING");
        _horizontalPosition.set("SWING");
        _powerState.set("OFF");

        _irsend.begin();
    }

    static uint8_t calcChecksum(const uint8_t data[]);

    void dumpState(void)
    {
        Serial.print(__func__);
        Serial.print("  ");
        for (int i = 0 ; i < YAMATO_LENGTH ; i++)
        {
            Serial.print(packetData[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
};
