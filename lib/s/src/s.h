/**
 * s - Little Stream - A simple data transport protocol
 * 
 * Please read the README for more information
 */
#ifndef s_h
#define s_h

#include <Arduino.h>
#include "brotli/decode.h"

#define VERSION 0
#define BROTLI_DECOMPRESSION_BUFFER 3000
#define DEFAULT_TRANSACTION_BUFFER 10

#define PKT_ACK 0x0
#define PKT_STREAM 0x1

#define RECEIVE_NO_FAIL 0x0 // Good packet
#define RECEIVE_FAIL_ZERO_SESS 0x1 // The session was zero
#define RECEIVE_FAIL_BAD_HEADER 0x2 // Bad header, ie bad version or not a s packet
#define RECEIVE_FAIL_INVALID_SESSION 0x3 // Session did not match with the current session
#define RECEIVE_FAIL_INCORRECT_NODE 0x4 // Packet was meant for another node
#define RECEIVE_FAIL_BAD_FRAME_ID 0x5 // Frame was late or out of order

#define MAX_PAYLOAD_SIZE 1500
#define TTL 1000

class s
{
    public:
    typedef struct{
        bool downstream = false;
        bool compressed = false;
        uint8_t version = 0x0;
        uint8_t nodeID = 0;
        uint8_t session = 0;
        uint8_t frame = 0;
        uint8_t fragment = 0;
        bool valid = false;
    } header;

    typedef struct{
        header h;
        uint8_t *pyld;
        uint16_t len;
        uint16_t transactionLength = 0;
        uint8_t transactionChecksum = 0x0;
        bool valid = false;
    } message;

    typedef struct {
        unsigned index = 0;
        uint16_t size = 0;
        uint8_t *pyld = NULL;
        uint8_t checksum = 0x0;
        uint8_t frame = 0;
        bool valid = false;
        unsigned long started = 0; // Millis when started
    } transaction;

    // Constructor
    // Takes in a node id and a current session. The sess ID can be ignored and provided later as well
    s(uint8_t nodeId, uint8_t currentSession = 0x0);

    void setCallback(void(*callbackFnc)(uint8_t *pyld, uint16_t len));
    void setNodeID(uint8_t ID);
    void setSessionID(uint8_t ID);
    // Use this if you want to add the ability to disable compression using the control layer
    // such as MQTT, or want to disable it from startup. The client must be aware of this.
    void disableCompression();
    // Enables compression, use this to re-enable compression when using the function above
    void enableCompression();

    uint8_t receive(uint8_t *pyld, uint16_t len);

    private:
    message parseMessage(uint8_t *pyld, uint16_t len);
    header parseHeader(uint8_t *pyld);

    bool recentTransaction(uint8_t frame);

    uint8_t *compress(uint8_t *pyld, uint16_t *len);
    uint8_t *decompress(uint8_t *pyld, uint16_t *len);

    uint8_t lastDwnFrame = 0;
    uint8_t lastUpFrame = 0;

    // NodeID must not be 0
    uint8_t nodeID = 0;
    uint8_t currentSession = 0x0;

    transaction *transactionBuffer;
    unsigned transactionBufferSize = 0;
    unsigned transactionIndex = 0; // Used as sort of an ID

    bool compressData = false;

    void (*callback)(uint8_t *pyld, uint16_t len);
};
#endif