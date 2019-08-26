/*
  Handling of led output
*/
#include <ArduinoJson.h>
#ifndef connection_h
#define connection_h

class Output
{
  public:
  Output();
  void setup();
  void setPixels(JsonArray*);
  private:
  
};

#endif
