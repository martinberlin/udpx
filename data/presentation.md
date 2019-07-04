## Notes about setting a presentation for the controller ##

This is the SPIFFs folder where the presentation JSON is stored.
Please refer a pointer to one file from config.h setting this definition:

// Set a JSON valid and existing data/presentation-name.json
#define FIRMWARE_PRESENTATION "/stripe-grb-144.json"

## SPIFFs upload ##
Remember to run:
pio run --target uploadfs 

In order to upload SPIFFS to the Espressif board.
