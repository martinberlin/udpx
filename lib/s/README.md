# s - Little Stream - Embedded streaming layer for UDPX

**NOTE - This network layer is in development**

## Overview
s, also known as little stream, is a super simple data transport layer that is meant to be used in the UDPX project that both is small as possible, and made for realtime applications, which has the ability to be compressed. This is only meant for streaming, as command and control of devices should be over MQTT. 

What I'm thinking for this current project with neopixels is the following-

| Communication Stack |
|---------------------|
| PIXELS Data         |
| d                   |
| Brotli Compression  |
| UDP                 |

This will use __significantly__ less bandwidth than JSON.

## Sessions
A session is started via MQTT, and must be agreed uppon by both the client and the application before using this layer. To create a session, a NODE_ID, SESSION_ID, and VERSION must be set.  The NODE_ID is a unsigned eight bit integer to identify a stream, the SESSION_ID is used to ensure the correct session is in operation after being set by MQTT, and the VERSION is used to prevent different versions from communicating when incompatible.

## Protocol

### Header

The first byte of the header consists of a three bit version, a downstream flag, and a compressed flag (Rest of bits are reserved for future functions). After that, a nodeID byte, session byte, and frame counter are passed to complete the header. After that, the rest of length is message that can be passed to callback.

If the compression bit is set, the test of the message has received brotli compression and required decompression.