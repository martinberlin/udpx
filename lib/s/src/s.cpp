#include <s.h>

s::s(uint8_t nodeId, uint8_t sessID){
    nodeID = nodeId;
    currentSession = sessID;
}

void s::setCallback(void(*callbackFnc)(uint8_t *pyld, uint16_t len)){
    callback = callbackFnc;
}

uint8_t s::receive(uint8_t *pyld, uint16_t len){
    s::message m = parseMessage(pyld, len);
    if(m.h.version!=VERSION){
        return RECEIVE_FAIL_BAD_HEADER;
    }
    if(m.h.session==0){
        return RECEIVE_FAIL_ZERO_SESS;
    }
    if(m.h.session!=currentSession){
        return RECEIVE_FAIL_INVALID_SESSION;
    }
    if(m.h.nodeID!=nodeID){
        return RECEIVE_FAIL_INCORRECT_NODE;
    }
    bool rt = recentTransaction(m.h.frame);
    if(m.h.frame<lastDwnFrame&&!rt){
        return RECEIVE_FAIL_BAD_FRAME_ID;
    }

    if(!rt&&m.h.fragment==0){
        s::transaction t = {transactionIndex++, m.transactionLength, m.pyld, m.transactionChecksum, m.transactionChecksum.frame, true, millis()};
    }

    return RECEIVE_NO_FAIL;
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
    m.len = len - 5;
    if(m.h.fragment==0){ // If this is the first fragment in the transaction, we want to gather the size and checksum
        m.transactionLength = pyld[5] | pyld[6]<<8;
        m.transactionChecksum = pyld[7];
        m.pyld = &pyld[8];
        m.len = len - 8;
    }else{
        m.pyld = &pyld[5];
        m.len = len - 5;
    }
    m.valid = true;
    return m;
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
    h.fragment = pyld[4];
    h.valid = true;
    return h;
}

bool s::recentTransaction(uint8_t frame){
    for(unsigned i =0; i<transactionBufferSize;i++){
        if (transactionBuffer[i].frame==frame&&transactionBuffer[i].valid){
            // We have to loop through all the messages in this function every time we get a message, may as well calculate TTL and expire any entries
            if(millis()-transactionBuffer[i].started>=TTL){
                transactionBuffer[i].valid = false;
                transactionBuffer[i].pyld = NULL;
                continue;
            }
            return true;
        }
    }
    return false;
}