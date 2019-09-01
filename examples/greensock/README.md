![udpx Logo](/examples/udpix-logo.png)

To run this example you need to start a middleware that forwards HTTP Post packages to UDP packages since the ESP32 works with that protocol but it's imposible to send it natively from your Browser due to security restrictions.

So do simply a:
npm install  (*not required: only for future compression)

nodejs middleware.js 

and run index.html in your browser.

### Requirements

Npm / nodejs installed.
In linux is just one line: apt-get install npm

### How it works 

The simple animation is generated in your browser frontend using greensock. On the lineUpdate() callback we just read the DIV position and generate a binary representation that is sent to the stripe.

Animation was taken from a greensock example: 

    tl1 = TweenMax.to(stripe, delay.val(), {
      left:288,
      onUpdate:lineUpdate,
      onUpdateParams:["{self}"],
      paused:true
     });

Following Samuel's Pixel protocol this is sent in the raw body of an Ajax request to localhost:1234 where there is a nodejs script running, that acts as a proxy, redirecting is "as is" to a dgram.Socket (UDP) to the ESP32 IP.
Check readme file in ../lib/pixels for more details about the protocol.