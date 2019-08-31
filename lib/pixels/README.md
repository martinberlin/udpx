# PIXELS - A super simple LED application layer

This lib will be able to support up to ~21830 pixels on each of 255 channels if we wanted to add channels (Multiple strips) when using UDP and s. ESP wont handle all that tho XD

I made this lib also the neo-pixel driver because I thought that made reasonable sense. I was thinking we could add effects or something as well to this lib, idk.

# Protocol 

**Not final yet**

I'm thinking a starting byte if a 0x50 for a ascii `p` for pixels. This is to verify that the message is in fact pixel data. There could be other protocols that always start with 0x50, but this gets the job done. After that comes the sync word, this is to verify the right client is connected and the right mode is on (Future features ;) ). Following that is a unsigned 8-bit LED channel number, after that, a 16 bit LSB unsigned integer follows that is the number of RGB or RGBW values. The RGB or RGBW must be selected and is a part of the mode mentioned earlier. The device should inform the application if it is RGB or RGBW in the used control communication (MQTT in this case). After this, there is an array of three or four byte RGB or RGBW values respectively the length of the number of pixels in the uint16 size value. After this, there is a one byte CRC for checking the UDP message arrived correctly (Optional, setting constant `USECRC` to false in the header file will remove the CRC check).