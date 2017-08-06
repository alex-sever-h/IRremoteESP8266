#include "IRremoteESP8266.h"
#include "IRrecv.h"
#include "ir_Yamato.h"

//==============================================================================
//                           N   N  EEEEE   CCCC
//                           NN  N  E      C
//                           N N N  EEE    C
//                           N  NN  E      C
//                           N   N  EEEEE   CCCC
//==============================================================================

#define YAMATO_BITS         112
#define YAMATO_HDR_MARK    3800
#define YAMATO_HDR_SPACE   1400
#define YAMATO_BIT_MARK     600
#define YAMATO_ONE_SPACE   1100
#define YAMATO_ZERO_SPACE   500
#define YAMATO_RPT_SPACE   2250

#define MATCH_MARK  matchMark
#define MATCH_SPACE matchSpace

char gdata[14];

//+=============================================================================
#if SEND_YAMATO
void  IRsend::sendYamato (unsigned char data[])
{
    int bit = 0;

    // Set IR carrier frequency
    enableIROut(38);

    DPRINTLN(__func__);

    // Header
    mark(YAMATO_HDR_MARK);
    space(YAMATO_HDR_SPACE);

    DPRINTLN("HEADER SENT false");

    for (int i = 0; i < YAMATO_LENGTH ; i++)
        sendData(YAMATO_BIT_MARK, YAMATO_ONE_SPACE, YAMATO_BIT_MARK,
                 YAMATO_ZERO_SPACE, data[i], 8, false);

    DPRINTLN("DATA SENT");

    // Footer
    mark(YAMATO_BIT_MARK);
    space(0);  // Always end with the LED off

    DPRINTLN("DONE");
    DPRINTLN(__func__);
}
#endif

//+=============================================================================
// YAMATOs have a repeat only 4 items long
//
#if DECODE_YAMATO
bool  IRrecv::decodeYAMATO (decode_results *results)
{
    unsigned char data[14] = {0};  // We decode in to here; Start with nothing
    int   offset  = 1;  // Index in to results; Skip first entry!?
    int bit = YAMATO_BITS - 1;

    // Check we have enough data
    if (results->rawlen < (2 * YAMATO_BITS) + 4)  return false ;

    // Check header "mark"
    if (!MATCH_MARK(results->rawbuf[offset], YAMATO_HDR_MARK))  return false ;
    offset++;

    // Check header "space"
    if (!MATCH_SPACE(results->rawbuf[offset], YAMATO_HDR_SPACE))  return false ;
    offset++;

    // Build the data
    for (int i = 0;  i < YAMATO_BITS;  i++) {
        // Check data "mark"
        if (!MATCH_MARK(results->rawbuf[offset], YAMATO_BIT_MARK)) { DPRINTLN("NO MARK"); return false; }
        offset++;
        // Suppend this bit
        if      (MATCH_SPACE(results->rawbuf[offset], YAMATO_ONE_SPACE ))
            data[(YAMATO_BITS - bit - 1) / 8] |= 1 << ((YAMATO_BITS - bit - 1) % 8) ;
        else if (MATCH_SPACE(results->rawbuf[offset], YAMATO_ZERO_SPACE))
            data[(YAMATO_BITS - bit - 1) / 8] |= 0 << ((YAMATO_BITS - bit - 1) % 8) ;
        else                                                       { DPRINTLN("NO SPACE"); return false; }
        offset++;
        bit--;
    }

    Serial.print("BYTES: ");
    for(int j = 0 ; j < 14 ; j++)
    {
        Serial.print(j);
        Serial.print(":");
        if (gdata[j] != data[j])
            Serial.print("*");
        if(j != 8)
            Serial.print(data[j], HEX);
        else{
            Serial.print(data[j], BIN);
            Serial.print(" ");
            Serial.print(data[j], HEX);
        }
        Serial.print(" | ");

        gdata[j] = data[j];
    }
    Serial.println("\n");

    if(data[13] != IRYamato::calcChecksum(data))
        return false;

    // Success
    results->bits        = YAMATO_BITS;
    results->decode_type = YAMATO;

    interpretYAMATO(results, data);

    return true;
}

uint8_t IRYamato::calcChecksum(const uint8_t data[])
{
    uint8_t crc_acc = 0;
    for(int j = 0 ; j < YAMATO_LENGTH - 1 ; j++)
    {
        crc_acc += data[j];
    }
    return crc_acc;
}

typedef struct dataLUT_t {
    unsigned char value;
    String operation;
} dataLUT_t;

dataLUT_t commandType[] =
{
    {0x18, "vertical"},
    {0x1C, "horizontal"},
    {0x08, "operation"},
    {0x24, "sleep"},
    {0x10, "temp_down"},
    {0x14, "temp_up"},
    {0x04, "on/off"},
    {0x0C, "fanspeed"},
};

bool  IRrecv::interpretYAMATO (const decode_results *results, const unsigned char *data)
{
    unsigned char decode;

    // decode command type
    decode = data[12];

    Serial.println("");
    for(int i = 0 ; i < sizeof(commandType) / sizeof(commandType[0]) ; i++)
    {
        if(commandType[i].value == decode)
            Serial.println(commandType[i].operation);
    }
}

#endif
