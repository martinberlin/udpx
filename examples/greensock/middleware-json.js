/**
 * Middleware will just receive a POST with :
 * data - Json data (Method: POST)
 * compressed - Has to be compressed? true as default (GET)
 * ip - Destination ip (GET)
 * port - Optional defaults to 1234 (GET)
 * 
 * And will send that data depending on the compressed flag as an UDP package to destination ip
 * 
 * Turn DEBUG false to disable console output
 */
var DEBUG = true;
var PORT = 1234;       // Default port
var HOST = '127.0.0.1';

// Requires
//var bodyParser = require('body-parser');
var dgram = require('dgram');
var brotli = require('brotli');
const http = require('http');
var url = require('url');
var client = dgram.createSocket('udp4');


http.createServer((request, response) => {
  let body = [];
  request.on('data', (chunk) => {
    body.push(chunk);

  }).on('end', () => {
    if (DEBUG) console.log("body L:"+body.length +" \ncontents:"+body)
    
    var url_parts = url.parse(request.url, true);
    var query = url_parts.query;


    // Is compressed?
    if (query.c === "0") {
     var outBuff = Buffer.from(body.toString(), 'utf8');
     
    } else {
      // Compress the json with Brotli
       outBuff = brotli.compress(Buffer.from(body.toString(), 'utf8'), {
        quality: 1, // 0 - 11
        lgwin: 16
      }); 
    }
    // TODO: FIX ip address + compression should come by GET
    sendBuffer(outBuff, query.ip);

    body = '1';//Buffer.concat(body).toString();
    response.end(body);
  })
  .on('error', (err) => {
    console.error(err);
  });
}).listen(1234);


function sendBuffer(inBuffer, outHost) {

    client.send(inBuffer, 0, inBuffer.length, PORT, outHost, function(err, bytes) {
        if (err) throw err;

        if (DEBUG) console.log('Sent: ' + inBuffer.toString() + ' to ' + outHost +':'+ PORT);
    });

}
    
