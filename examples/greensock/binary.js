var stripe = $("div#stripe"),
    btn1 = $("button#btn1"),
    btn2 = $("button#btn2"),
    btn3 = $("button#btn3"),
    http_mode = $("#http"),
    red = $("#r"), green = $("#g"), blue = $("#b"),
    stripe_length = $("#stripe_length"),
    output = $("#out"),
    ip = $("#esp32_ip"),
    compression = $("#compression"),
    delay = $("#duration");

    tl1 = TweenMax.to(stripe, delay.val(), {
      left:288,
      onUpdate:lineUpdate,
      onUpdateParams:["{self}"],
      paused:true
     });

     let lastPush;

function toHexString(byteArray) {
  let out = Array.from(byteArray, function(byte) {
    return ('0' + (byte & 0xFF).toString(16)).slice(-2).toUpperCase();
  }).join(' '); 
   return out+' ';
}

function convertPixel(x) {
  let displayHex = "";

  var bytes = [];
  let off = [0,0,0];
  let pixLength = stripe_length.val();
  rgb = [parseInt(red.val()), parseInt(green.val()), parseInt(blue.val())];
  let bufferLen = (pixLength*3)+5;

  // create an ArrayBuffer with a size in bytes
  var buffer = new ArrayBuffer(bufferLen);
  // Treat buffer as a view of 8-bit unsigned integer 
  var bytesToPost = new Uint8Array(buffer); 
  bi = 0;

/*
  Papa explanation of LSB & MSB
   MSB = Int(Num Dec/256)
   LSB = Num Dec -(MSB*256)
 */
  let MSB = parseInt(pixLength/256);
  let LSB = pixLength - (MSB*256);
  // header bytes
  hByte = [80,0,0,LSB,MSB];
  console.log(hByte);
  bytesToPost[bi] = hByte[0];bi++;  // p
  bytesToPost[bi] = hByte[1];bi++;  // Future features (not used)
  bytesToPost[bi] = hByte[2];bi++;  // unsigned 8-bit LED channel number
  bytesToPost[bi] = hByte[3];bi++;  // count(pixels) 16 bit, next too
  bytesToPost[bi] = hByte[4];bi++;  // Second part of count(pixels) not used here for now
  displayHex += toHexString(hByte); // Start the preview in HEX with headers

  for (var k = 1; k <= parseInt(stripe_length.val()); k++) {
    if (x >= k-1 && x <= k+1) {
      bytesToPost[bi] = parseInt(red.val());bi++;
      bytesToPost[bi] = parseInt(green.val());bi++;
      bytesToPost[bi] = parseInt(blue.val());bi++;
      displayHex += toHexString(rgb);
      bytes.push(rgb);
      
    } else {
      bytesToPost[bi] = 0;bi++;
      bytesToPost[bi] = 0;bi++;
      bytesToPost[bi] = 0;bi++;
      displayHex += toHexString(off);
      bytes.push(off);
    }
  }
  
  if (displayHex !== lastPush) { 
    isHttp = http_mode.is(":checked") ?1:0;
    if (isHttp) {
      // Send using TCP/IP directly to ESP32 IP
      $.ajax("http://"+ ip.val() +"/post",
        {
        'data': bytesToPost, 
        'type': 'POST',
        async: false,
        processData: false,
        contentType: false,
        crossDomain: true
        }); 
    } else {
      // Send using UDP to middleware
        $.ajax("http://127.0.0.1:1234?ip="+ ip.val() +"&c=0",
        {
        'data': bytesToPost, 
        'type': 'POST',
        async: false,
        processData: false,
        crossDomain: true,
        contentType: false
        });  
    }
  }
  output.val(displayHex);
  lastPush = displayHex;
}

function lineUpdate(tween)
{
  tl1.progress(tween.progress());
  stripepos = stripe.position();
  stripex = stripepos.left -10;
  ledstripex = parseInt((stripex*stripe_length.val())/288);

  convertPixel(ledstripex);
}

function colorIt() {
  stripe.css("background-color", "rgb(" + red.val() + "," + green.val() + "," + blue.val() +")");
}
btn1.click(function()
{
  colorIt();
  tl1.duration(delay.val());
  tl1.play(0);
});

btn2.click(function()
{
  colorIt();
  tl1.duration(delay.val());
  tl1.reverse();
});

btn3.click(function()
{
  convertPixel(10);
});