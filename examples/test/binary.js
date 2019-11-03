let stripe = $("div#stripe"),
    btn1 = $("button#btn1"),
    btn2 = $("button#btn2"),
    btn3 = $("button#btn3"),
    btnFirst = $("button#btnFirst"),
    btnMinus = $("button#btnMinus"),
    btnPlus = $("button#btnPlus"),
    btnLast = $("button#btnLast"),
    btnSaveFrame = $("#btnSaveFrame"),
    http_mode = $("#http"),
    rgb = $("#rgb"),
    rgbw = $("#rgbw"),whiteIcon = $("#whiteIcon"),
    red = $("#r"), green = $("#g"), blue = $("#b"), white = $("#w"),
    stripe_length = $("#stripe_length"),
    output = $("#out"),
    ip = $("#esp32_ip"),
    udp_port = $("#udp_port"),
    compression = $("#compression"),
    delay = $("#duration"),
    enable_hex = $("#enable_hex"),
    color_invert = $("#color_invert"),
    background_random = $("#background_random"),
    protocol = $("#protocol"),
    iX = 0;

let randomGeneratorMax = 100;

tl1 = TweenMax.to(stripe, delay.val(), {
  left:288,
  onUpdate:lineUpdate,
  onUpdateParams:["{self}"],
  paused:true
  });

let lastPushDownload;

function toHexString(byteArray) {
  let out = Array.from(byteArray, function(byte) {
    return ('0' + (byte & 0xFF).toString(16)).slice(-2).toUpperCase();
  }).join(' '); 
   return out+' ';
}

/**
 * Main animation x to pixels converter
 * @param {*} x 
 */
function convertPixel(x) {
  let displayHex = "";
  let pixLength = stripe_length.val();
  isRgbW = rgbw.is(":checked") ?1:0;
  enableHex = enable_hex.is(":checked");
  bi = 0;

  let MSB = parseInt(pixLength/256);
  let LSB = pixLength - (MSB*256);
  // header bytes - Todo: Add this in his own function

  switch (protocol.val()) {
    case 'pixels':
        hByte = [80,0,0,LSB,MSB];
        headerBytes = 5;
        cByte1 = parseInt(red.val());
        cByte2 = parseInt(green.val());
        cByte3 = parseInt(blue.val());
    break;
    case 'RGB888':
        hByte = [1,0,0,0,LSB,MSB];
        headerBytes = 6;
        cByte1 = parseInt(blue.val());
        cByte2 = parseInt(red.val());
        cByte3 = parseInt(green.val());
    break;
  }
  let bufferLen = (pixLength*3)+headerBytes;
  if (isRgbW) {
    bufferLen = (pixLength*4)+headerBytes;
  }
  // create an ArrayBuffer with a size in bytes
  var buffer = new ArrayBuffer(bufferLen);
  var bytesToPost = new Uint8Array(buffer); 
  
  //console.log(hByte); // Debug headers
  bytesToPost[bi] = hByte[0];bi++;  // p
  bytesToPost[bi] = hByte[1];bi++;  // Future features (not used)
  bytesToPost[bi] = hByte[2];bi++;  // unsigned 8-bit LED channel number
  bytesToPost[bi] = hByte[3];bi++;  // count(pixels) 16 bit, next too
  bytesToPost[bi] = hByte[4];bi++;  // Second part of count(pixels) not used here for now
  if (enableHex) {
    displayHex += toHexString(hByte); // Start the preview in HEX with headers
  }

  for (var k = 1; k <= parseInt(stripe_length.val()); k++) {
    // Calculate background and foreground colors
    let backR = 0;
    let backG = 0;
    let backB = 0;

    if (background_random.is(":checked")) {
      backR = Math.floor(Math.random() * randomGeneratorMax);
      backG = Math.floor(Math.random() * randomGeneratorMax);
      backB = Math.floor(Math.random() * randomGeneratorMax);
    }
    let foregroundColor = [backR, backG, backB];
    let backgroundColor = [cByte1, cByte2, cByte3];

    if (color_invert.is(":checked") === false) {
      foregroundColor = [cByte1, cByte2, cByte3];
      backgroundColor = [backR, backG, backB];
      if (isRgbW) {
        foregroundColor.push(parseInt(white.val()));
        backgroundColor.push(0);
      }
    } else {
      if (isRgbW) {
        foregroundColor.push(0);
        backgroundColor.push(parseInt(white.val()));
      }
    }

    if (x >= k-1 && x <= k+1) {
      bytesToPost[bi] = foregroundColor[0];bi++;
      bytesToPost[bi] = foregroundColor[1];bi++;
      bytesToPost[bi] = foregroundColor[2];bi++;
      if (enableHex) displayHex += toHexString(foregroundColor);
      if (isRgbW) {
        bytesToPost[bi] = foregroundColor[3];bi++;
      }
    } else {

      bytesToPost[bi] = backgroundColor[0];bi++;
      bytesToPost[bi] = backgroundColor[1];bi++;
      bytesToPost[bi] = backgroundColor[2];bi++;
      if (isRgbW) {
        bytesToPost[bi] = backgroundColor[3];bi++;
      }
      if (enableHex) displayHex += toHexString(backgroundColor);
    }
  }

  if (JSON.stringify(bytesToPost) != JSON.stringify(lastPushDownload)) { 
    sendToEsp(bytesToPost);
  }
  if (enableHex) {
    output.val(displayHex);
  }  
  lastPushDownload = bytesToPost;
}

function xPixel(x) {
  let pixLength = stripe_length.val();
  if (x<1 || x>pixLength) {
    console.log("Receiving X:" +x + " out of display length");
    return;
  }
  isRgbW = rgbw.is(":checked") ?1:0;
  let displayHex = "";
  let off = [0,0,0];
  
  let MSB = parseInt(pixLength/256);
  let LSB = pixLength - (MSB*256);

  switch (protocol.val()) {
    case 'pixels':
        hByte = [80,0,0,LSB,MSB];
        headerBytes = 5;
        cByte1 = parseInt(red.val());
        cByte2 = parseInt(green.val());
        cByte3 = parseInt(blue.val());
    break;
    case 'RGB888':
        hByte = [1,0,0,0,LSB,MSB];
        headerBytes = 6;
        cByte1 = parseInt(blue.val());
        cByte2 = parseInt(red.val());
        cByte3 = parseInt(green.val());
    break;
  }
  let rgb = [cByte1, cByte2, cByte3];

  let bufferLen = (pixLength*3)+headerBytes;
  if (isRgbW) {
    bufferLen = (pixLength*4)+headerBytes;
  }
  if (isRgbW) {
    rgb.push(parseInt(white.val()));
    off.push(0);
    bufferLen = (pixLength*4)+5;
  }   
  // create an ArrayBuffer with a size in bytes
  var buffer = new ArrayBuffer(bufferLen);
  // Treat buffer as a view of 8-bit unsigned integer 
  var bytesToPost = new Uint8Array(buffer); 
  bi = 0;
  bytesToPost[bi] = hByte[0];bi++;  // p
  bytesToPost[bi] = hByte[1];bi++;  // Future features (not used)
  bytesToPost[bi] = hByte[2];bi++;  // unsigned 8-bit LED channel number
  bytesToPost[bi] = hByte[3];bi++;  // count(pixels) 16 bit, next too
  bytesToPost[bi] = hByte[4];bi++;  // Second part of count(pixels) not used here for now
  displayHex += toHexString(hByte); // Start the preview in HEX with headers

  for (var k = 1; k <= parseInt(pixLength); k++) {
    if (x === k) {
      bytesToPost[bi] = cByte1;bi++;
      bytesToPost[bi] = cByte2;bi++;
      bytesToPost[bi] = cByte3;bi++;
      if (isRgbW) {
        bytesToPost[bi] = parseInt(white.val());bi++;
      }
      displayHex += toHexString(rgb);
      
    } else {
      bytesToPost[bi] = 0;bi++;
      bytesToPost[bi] = 0;bi++;
      bytesToPost[bi] = 0;bi++;
      if (isRgbW) {
        bytesToPost[bi] = 0;bi++;
      }
      displayHex += toHexString(off);
    }
  }
  output.val(displayHex);
  sendToEsp(bytesToPost);
  lastPushDownload = bytesToPost;
}

function sendToEsp(bytesToPost) {
  isHttp = http_mode.is(":checked") ?1:0;
    if (isHttp) {
      // Send using TCP/IP directly to ESP32 IP
      $.ajax("http://"+ ip.val() +"/post",
        {
        'data': bytesToPost, 
        'type': 'POST',
        async: true, /* Turn to false to see a different result */
        processData: false,
        contentType: false,
        crossDomain: true,
        success: function (response) {
          console.log(response);
          $('#bytes').text("Bytes rec: "+response.bytes);
          $('#millis').text("Millis: "+response.millis);
        },
        error: function (error) {
            console.log(error)
        }
      }); 
    } else {
      // Send using UDP to middleware
        $.ajax("http://127.0.0.1:1234?ip="+ ip.val() +"&port=" + udp_port.val(),
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
function lineUpdate(tween)
{
  tl1.progress(tween.progress());
  stripepos = stripe.position();
  stripex = stripepos.left - xCorrection;
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

btnFirst.click(function()
{
  colorIt();
  iX = 1;
  tl1.pause(0);
  xPixel(iX); //iX represents Pixel ID that needs to be on
});
btnPlus.click(function()
{
  colorIt();
  iX++;
  xPixel(iX);
});
btnMinus.click(function()
{
  colorIt();
  iX--;
  xPixel(iX);
});
btnLast.click(function()
{
  colorIt();
  iX = parseInt(stripe_length.val());
  //tl1.end(); // TODO: Don't know how to send animation to the end
  xPixel(iX);
});

rgbw.click(function()
{
  whiteIcon.addClass("rgbw");
  whiteIcon.removeClass("nowhite");
  white.prop("readonly", false);
});
rgb.click(function()
{
  whiteIcon.addClass("nowhite");
  whiteIcon.removeClass("rgbw");
  white.prop("readonly", true);
});

/**
 * Saves the last sent frame and allows the user to download.
 * Uses timestamp in the filename to get unique filenames
 */
btnSaveFrame.click(function () {
  const dateObj = new Date();
  createAndDownloadBlobFile(lastPushDownload, `${dateObj.getTime()}-led-frame`)
})

/**
 * Saves form state to localstorage
 * @param $form to save in localstorage(jQuery object)
 */
function saveFormState($form) {
  const formData = JSON.stringify($form.serializeArray());
  window.localStorage.setItem($form.attr('id'), formData);
}

/**
 * Loads form state from localstorage
 * @param $form to load from localstorage(jQuery object)
 */
function loadFormState($form) {
  const formData = JSON.parse(window.localStorage.getItem($form.attr('id')));
  // This is not working, here is null formData: 
  if (formData !== null) {
    formData.forEach((item) =>{
      const selector = `input[name="${ item.name }"], textarea[name="${ item.name }"]`
      const input = $form.find(selector)
      const newVal = item.value
      input.val(newVal);
    })
  }
}

/**
 * Load form state from localstorage and set handler
 * to save form state on form change
 */
$('document').ready(function () {
  loadFormState($('#main-form'))
  $("#main-form input").change(function() {
    saveFormState($(this).closest('form'));
  });
})

/**
 * Creates a file out of the given body parameter and triggers a download of the created file
 * @param body the payload to add to the file
 * @param filename
 * @param extension (of the file)
 */
function createAndDownloadBlobFile(body, filename, extension = 'bin') {
    const blob = new Blob([body]);
    const fileName = `${filename}.${extension}`;
    if (navigator.msSaveBlob) {
        // IE 10+
        navigator.msSaveBlob(blob, fileName);
    } else {
        const link = document.createElement('a');
        // Browsers that support HTML5 download attribute
        if (link.download !== undefined) {
            const url = URL.createObjectURL(blob);
            link.setAttribute('href', url);
            link.setAttribute('download', fileName);
            link.style.visibility = 'hidden';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    }
}
