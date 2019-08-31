![udpx Logo](/examples/udpix-logo.png)

To run this example you need to start a middleware that forwards HTTP Post packages to UDP packages since the ESP32 works with that protocol but it's imposible to send it natively due to security restrictions.

So do simply a:
npm install  (only in case you want to use json format)

nodejs middleware.js 

and run index.html in your browser.

### Requirements

Npm / nodejs installed.
In linux is just one line: apt-get install npm

