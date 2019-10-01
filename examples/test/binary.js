var stripe = $("div#stripe"),
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
    rgbw = $("#rgbw"), whiteIcon = $("#whiteIcon"),
    red = $("#r"), green = $("#g"), blue = $("#b"), white = $("#w"),
    stripe_length = $("#stripe_length"),
    output = $("#out"),
    ip = $("#esp32_ip"),
    compression = $("#compression"),
    delay = $("#duration"),
    colorInvert = $("#color_invert"),
    colorRandom = $("#color_random"),
    iX = 0;

tl1 = TweenMax.to(stripe, delay.val(), {
    left: 288,
    onUpdate: lineUpdate,
    onUpdateParams: ["{self}"],
    paused: true
});

let lastPushHex;
let lastPush;

function toHexString(byteArray) {
    let out = Array.from(byteArray, function (byte) {
        return ('0' + (byte & 0xFF).toString(16)).slice(-2).toUpperCase();
    }).join(' ');
    return out + ' ';
}

function convertPixel(x) {
    let displayHex = "";
    let pixLength = stripe_length.val();
    isRgbW = rgbw.is(":checked") ? 1 : 0;

    let off = [0, 0, 0];
    let bufferLen = (pixLength * 3) + 5;

    // create an ArrayBuffer with a size in bytes
    var buffer = new ArrayBuffer(bufferLen);
    var bytesToPost = new Uint8Array(buffer);
    bi = 0;

    let MSB = parseInt(pixLength / 256);
    let LSB = pixLength - (MSB * 256);
    // header bytes - Todo: Add this in his own function
    hByte = [80, 0, 0, LSB, MSB];
    //console.log(hByte); // Debug headers
    bytesToPost[bi] = hByte[0];
    bi++;  // p
    bytesToPost[bi] = hByte[1];
    bi++;  // Future features (not used)
    bytesToPost[bi] = hByte[2];
    bi++;  // unsigned 8-bit LED channel number
    bytesToPost[bi] = hByte[3];
    bi++;  // count(pixels) 16 bit, next too
    bytesToPost[bi] = hByte[4];
    bi++;  // Second part of count(pixels) not used here for now
    displayHex += toHexString(hByte); // Start the preview in HEX with headers

    for (var k = 1; k <= parseInt(stripe_length.val()); k++) {
      isInverted = colorInvert.is(":checked") ? 1 : 0;
      isRandom = colorRandom.is(":checked") ? 1 : 0;
      let rC = (isRandom) ? Math.floor(Math.random() * 255)  : parseInt(red.val());
      let gC = (isRandom) ? Math.floor(Math.random() * 255)  : parseInt(green.val());
      let bC = (isRandom) ? Math.floor(Math.random() * 255)  : parseInt(blue.val());
      let wC = (isRandom) ? Math.floor(Math.random() * 255)  : parseInt(white.val());
      let rgb = [rC, gC, bC];
      if (isRgbW) {
        rgb.push(wC);
        off.push(0);
        bufferLen = (pixLength * 4) + 5;
      }

        if (x >= k - 1 && x <= k + 1) {

            bytesToPost[bi] = (!isInverted) ? rC: 0;
            bi++;
            bytesToPost[bi] = (!isInverted) ? gC : 0;
            bi++;
            bytesToPost[bi] = (!isInverted) ? bC : 0;
            bi++;
            displayHex += (!isInverted) ? toHexString(rgb) : toHexString(off);
            if (isRgbW) {
                bytesToPost[bi] = (!isInverted) ? wC : 0;
                bi++;
            }
        } else {
            bytesToPost[bi] = (!isInverted) ? 0 : rC;
            bi++;
            bytesToPost[bi] = (!isInverted) ? 0 : gC;
            bi++;
            bytesToPost[bi] = (!isInverted) ? 0 : bC;
            bi++;
            if (isRgbW) {
                bytesToPost[bi] = (!isInverted) ? 0 : wC;
                bi++;
            }
            displayHex += (!isInverted) ? toHexString(off) : toHexString(rgb);
        }
    }

    if (displayHex !== lastPushHex) {
        sendToEsp(bytesToPost);
    }

    //console.log(displayHex);
    output.val(displayHex);
    lastPushHex = displayHex;
    lastPush = bytesToPost;
}

function xPixel(x) {
    let pixLength = stripe_length.val();
    if (x < 1 || x > pixLength) {
        console.log("Receiving X:" + x + " out of display length");
        return;
    }
    isRgbW = rgbw.is(":checked") ? 1 : 0;
    let displayHex = "";
    let off = [0, 0, 0];
    let rgb = [parseInt(red.val()), parseInt(green.val()), parseInt(blue.val())];
    let bufferLen = (pixLength * 3) + 5;
    if (isRgbW) {
        rgb.push(parseInt(white.val()));
        off.push(0);
        bufferLen = (pixLength * 4) + 5;
    }
    let MSB = parseInt(pixLength / 256);
    let LSB = pixLength - (MSB * 256);
    // header bytes
    hByte = [80, 0, 0, LSB, MSB];

    // create an ArrayBuffer with a size in bytes
    var buffer = new ArrayBuffer(bufferLen);
    // Treat buffer as a view of 8-bit unsigned integer
    var bytesToPost = new Uint8Array(buffer);
    bi = 0;
    bytesToPost[bi] = hByte[0];
    bi++;  // p
    bytesToPost[bi] = hByte[1];
    bi++;  // Future features (not used)
    bytesToPost[bi] = hByte[2];
    bi++;  // unsigned 8-bit LED channel number
    bytesToPost[bi] = hByte[3];
    bi++;  // count(pixels) 16 bit, next too
    bytesToPost[bi] = hByte[4];
    bi++;  // Second part of count(pixels) not used here for now
    displayHex += toHexString(hByte); // Start the preview in HEX with headers

    for (var k = 1; k <= parseInt(pixLength); k++) {
        if (x === k) {
            bytesToPost[bi] = parseInt(red.val());
            bi++;
            bytesToPost[bi] = parseInt(green.val());
            bi++;
            bytesToPost[bi] = parseInt(blue.val());
            bi++;
            if (isRgbW) {
                bytesToPost[bi] = parseInt(white.val());
                bi++;
            }
            displayHex += toHexString(rgb);

        } else {
            bytesToPost[bi] = 0;
            bi++;
            bytesToPost[bi] = 0;
            bi++;
            bytesToPost[bi] = 0;
            bi++;
            if (isRgbW) {
                bytesToPost[bi] = 0;
                bi++;
            }
            displayHex += toHexString(off);
        }
    }
    output.val(displayHex);
    //console.log(displayHex);
    sendToEsp(bytesToPost);
}

function sendToEsp(bytesToPost) {
    isHttp = http_mode.is(":checked") ? 1 : 0;
    if (isHttp) {
        // Send using TCP/IP directly to ESP32 IP
        $.ajax("http://" + ip.val() + "/post",
            {
                'data': bytesToPost,
                'type': 'POST',
                async: true, /* Turn to false to see a different result */
                processData: false,
                contentType: false,
                crossDomain: true,
                success: function (response) {
                    console.log(response);
                    $('#bytes').text("Bytes rec: " + response.bytes);
                    $('#millis').text("Millis: " + response.millis);
                },
                error: function (error) {
                    console.log(error)
                }
            });
    } else {
        // Send using UDP to middleware
        $.ajax("http://127.0.0.1:1234?ip=" + ip.val() + "&c=0",
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

function lineUpdate(tween) {
    tl1.progress(tween.progress());
    stripepos = stripe.position();
    stripex = stripepos.left - 10;
    ledstripex = parseInt((stripex * stripe_length.val()) / 288);

    convertPixel(ledstripex);
}

function colorIt() {
    stripe.css("background-color", "rgb(" + red.val() + "," + green.val() + "," + blue.val() + ")");
}

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
  formData.forEach((item) =>{
    const selector = `input[name="${ item.name }"], textarea[name="${ item.name }"]`
    const input = $form.find(selector)
    const newVal = item.value
    input.val(newVal);
  })
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


btn1.click(function () {
    colorIt();
    tl1.duration(delay.val());
    tl1.play(0);
});

btn2.click(function () {
    colorIt();
    tl1.duration(delay.val());
    tl1.reverse();
});

btn3.click(function () {
    convertPixel(10);
});

btnFirst.click(function () {
    colorIt();
    iX = 1;
    tl1.pause(0);
    xPixel(iX); //iX represents Pixel ID that needs to be on
});
btnPlus.click(function () {
    colorIt();
    iX++;
    xPixel(iX);
});
btnMinus.click(function () {
    colorIt();
    iX--;
    xPixel(iX);
});
btnLast.click(function () {
    colorIt();
    iX = parseInt(stripe_length.val());
    //tl1.end(); // TODO: Don't know how to send animation to the end
    xPixel(iX);
});

/**
 * Saves the last sent frame and allows the user to download.
 * Uses timestamp in the filename to get unique filenames
 */
btnSaveFrame.click(function () {
    const dateObj = new Date();
    createAndDownloadBlobFile(lastPush, `${dateObj.getTime()}-led-frame`)
})

rgbw.click(function () {
    whiteIcon.addClass("rgbw");
    whiteIcon.removeClass("nowhite");
    white.prop("readonly", false);
});
rgb.click(function () {
    whiteIcon.addClass("nowhite");
    whiteIcon.removeClass("rgbw");
    white.prop("readonly", true);
});
