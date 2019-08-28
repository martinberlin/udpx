#include <s.h>

s::s(uint8_t nodeId, uint8_t sessID){
    nodeID = nodeId;
    currentSession = sessID;
}

void s::setCallback(void(*callbackFnc)(uint8_t *pyld, uint8_t len)){
    callback = callbackFnc;
}

void s::setNodeID(uint8_t ID){
    nodeID = ID;
}

void s::setSessionID(uint8_t sessionID){
    currentSession = sessionID;
    lastDwnFrame = 0;
    lastUpFrame = 0;
}

void s::disableCompression(){
    compressData = false;
}

void s::enableCompression(){
    compressData = true;
}

s::message s::parseMessage(uint8_t *pyld, uint16_t len){
    s::message m;
    // TODO reject impossibly small messages before they get to parse header
    m.h = parseHeader(pyld);
    if (!m.h.valid){
        m.valid = false;
        return m;
    }
    callback(&pyld[4], len-4);
}


s::header s::parseHeader(uint8_t *pyld){
    s::header h;
    h.version = pyld[0]>>5;
    if (h.version!=VERSION){
        h.valid = false;
        return h;
    }
    uint8_t flags = h.version <<5 ^ pyld[0]; // Store direction and compression flags
    if (flags >> 4 == 1){
        h.downstream = true;
        flags = flags ^ 0b00010000; // First time I've used binary literals, I wanna see if they work well
    }
    if (flags>>3==1){
        h.compressed = true;
        flags = flags ^ 0b00001000;
    }
    // Add more flag code here as needed
    h.nodeID = pyld[1];
    h.session = pyld[2];
    h.frame = pyld[3];
    h.valid = true;
    return h;
}