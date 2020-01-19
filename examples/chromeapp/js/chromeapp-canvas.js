let d = document;
let v = d.getElementById('video');
let canvas = d.getElementById('c');
let context = canvas.getContext('2d');
let ip = d.getElementById('esp32_ip'),
    port = d.getElementById('udp_port'),
    contrast = d.getElementById('v_contrast'),
    v_width  = d.getElementById('v_width'),
    v_height = d.getElementById('v_height'),
    v_units  = d.getElementById('v_units'),
    video = d.getElementById('video'),
    video_select = d.getElementById('video_select'),
    millis_frame = d.getElementById('millis_frame'),
    protocol = d.getElementById('protocol'),
    transmission = d.getElementById('transmission'),
    quality = d.getElementById('bro_quality'),
    v_brightness = d.getElementById('v_brightness');
let o_chunk = d.getElementById('o_chunk'), o_chunk_label = d.getElementById('o_chunk_label'),o_chunk_pre = "Set header chunk size to ";
let m_rotate_lines = d.getElementById('m_rotate_lines'),
m_invert_unit = d.getElementById('m_invert_unit');
let t_empty = '&#9633;';
let socketId, oldPort = port.value; 
let cw = parseInt(v_width.value),
ch = parseInt(v_height.value)*parseInt(v_units.value),
unitH = parseInt(v_height.value);
let ua = navigator.userAgent.toLowerCase();
let isAndroid = ua.indexOf("android") > -1;
let adv_invert = 0;
d.addEventListener('DOMContentLoaded', function(){
    openSocket();
    cleanTransmission();
    /**
     * Load form state from localstorage and set handler
     * to save form state on form change ( @hputzek )
     */
    loadFormState($('#main-form'))
    $("#main-form input").change(function() {
      saveFormState($(this).closest('form'));
    });

    video_select.onchange = function() {
        if (video_select.value !== '') {
            cleanTransmission();
            video.setAttribute('src','video/'+video_select.value);
            ch = parseInt(v_height.value)*parseInt(v_units.value);
            canvas.width = parseInt(v_width.value);
            canvas.height = ch;
        }
    };
    ip.onchange = function() {
        cleanTransmission();
    };
    port.onchange = function() {
        cleanTransmission();
    };
    m_invert_unit.onchange = function() {
        saveFormState();
       if (m_invert_unit.checked) {
         adv_invert = -1;
        } else {
         adv_invert = 0;
       }
     }
    v_width.onchange = function() {
        o_chunk_label.innerText = o_chunk_pre + (v_width.value * v_height.value);
    }
    v_height.onchange = function() {
        cleanTransmission();
        ch = parseInt(v_height.value)*parseInt(v_units.value);
        canvas.height = ch;
        o_chunk_label.innerText = o_chunk_pre + (v_width.value * v_height.value);
    }
    v_units.onchange = function() {
        ch = parseInt(v_height.value)*parseInt(v_units.value);
        canvas.height = ch;
        cleanTransmission();
    }
    protocol.onchange = function() {
        cleanTransmission();
        ch = parseInt(v_height.value)*parseInt(v_units.value);
        canvas.height = ch;
        switch (protocol.value) {
            case 'bro888':
            case 'rgb888':
            oldPort = port.value;
            port.value = '6454';
            break;
            case 'pixzlib':
            case 'pixels':
            port.value = oldPort;
            break;
            case 'pixbro':
                if (isAndroid){
                    transmission.className = 'red';
                    transmission.innerHTML = 'NOTE: Brotli compression will not run in Android';
                }
                port.value = oldPort;
            break;
        }
    };
    
    canvas.width = parseInt(v_width.value);
    canvas.height = parseInt(v_height.value)*parseInt(v_units.value);

    v.addEventListener('play', function(){
        draw(this,context,cw,ch);
    },false);
    v.addEventListener('pause', function(){
        cleanTransmission();
    },false);
},false);

function sendUdp(bytesToPost) {
    let compressed;
    let t0 = performance.now();
    let t1;
    switch (protocol.value) {
        case 'bro888':
            // 6 bytes header + Compressed data
            let headerBytes = bytesToPost.slice(0,6);
            let data = bytesToPost.slice(headerBytes.length-1, bytesToPost.length);
            compressed = compress(data, data.length, quality.value, lg_window_size);
            let send = new Int8Array(headerBytes.length + compressed.length);
            send.set(headerBytes);
            send.set(compressed, headerBytes.length);
            t1 = performance.now();
            chrome.sockets.udp.send(socketId, send, ip.value, parseInt(port.value), function(sendInfo) {
                transmission.innerText = "Took "+Math.round(t1-t0)+" ms. sent "+bytesToPost.length+"/"+sendInfo.bytesSent+" compressed bytes ";
            });
        break;
        case 'pixbro':
            compressed = compress(bytesToPost, bytesToPost.length, quality.value, lg_window_size);
            t1 = performance.now();
            chrome.sockets.udp.send(socketId, compressed, ip.value, parseInt(port.value), function(sendInfo) {
                    transmission.innerText = "Took "+Math.round(t1-t0)+" ms. sent "+bytesToPost.length+"/"+sendInfo.bytesSent+" bro compressed bytes ";
            });
        break;
        case 'pixzlib':
            compressed = flate.zlib_encode_raw(bytesToPost);
            t1 = performance.now();
            console.log("Implementing this")
            /* chrome.sockets.udp.send(socketId, compressed, ip.value, parseInt(port.value), function(sendInfo) {
                    transmission.innerText = "Zlib took "+Math.round(t1-t0)+" ms. sent "+bytesToPost.length+"/"+sendInfo.bytesSent+" bro compressed bytes ";
            }); */
        break;
        default:
            chrome.sockets.udp.send(socketId, bytesToPost, ip.value, parseInt(port.value), function(sendInfo) {
                    transmission.innerText = "Sending "+sendInfo.bytesSent+" bytes";
            });
        break;
   }

}


function convertChannel(pixels) {
    let pixLength = pixels.length;
    // Line data flow direction
    // ----> Line 1
    // <---- Line 2
    // ----> Line 3  ...
    // ----> Line 12 (in 2nd Unit but only if the units have impair rows like 11)
    let lineCount = 1;
    let cw = parseInt(v_width.value);

    if (m_rotate_lines.checked) {
        let invert = false;

        for (var x = 0; x <= pixLength-cw; x=x+cw) {

            // Invert pixels in pair lines for this Led matrix
            if (invert) {
                let pixelsInvertedCopy = pixels.slice(x,x+cw);
                pixelsInvertedCopy.reverse();

                let invIndex = 0;
                for (var inv = x; inv <= x+cw-1; inv++) {
                    pixels[inv] = pixelsInvertedCopy[invIndex];
                    invIndex++
                }
            }
            if ((lineCount % unitH === adv_invert) === false) {
              invert = !invert;
            }
            lineCount++;
        }

    }
    
    let MSB = parseInt(pixLength/256);
    let LSB = pixLength - (MSB*256);

    let cMSB = 0;
    let cLSB = 0;
    if (o_chunk.checked) {
        let chunk_size = v_width.value*v_height.value;
        cMSB = parseInt(chunk_size/256);
        cLSB = chunk_size - (cMSB*256);
    }

    let headerBytes = 6;
    let bytesPerPixel = 3;
    // Header bytes 
    switch (protocol.value) {
        case 'rgb888':
            hByte = [1,0,0,0,LSB,MSB];
        break;
        case 'rgb565':
            hByte = [3,0,0,0,LSB,MSB];
        break;
        case 'bro888':
            hByte = [14,0,0,0,LSB,MSB];
        break;
        case 'pix565':
            hByte = [82,0,cLSB,cMSB,LSB,MSB];
            bytesPerPixel = 2;
        break;
        default:
            // 1: p  2: Chunk LSB  3: Chunk MSB  4: Length LSB  5: Length MSB  6: protocol (0 pixels)
            hByte = [80,0,cLSB,cMSB,LSB,MSB];
        break;
      }

    let bufferLen = (pixLength*bytesPerPixel)+headerBytes;
    // create an ArrayBuffer with a size in bytes
    var buffer = new ArrayBuffer(bufferLen);
    var bytesToPost = new Uint8Array(buffer); 
    bi = 0;
    bytesToPost[bi] = hByte[0];bi++;  // p
    bytesToPost[bi] = hByte[1];bi++;  // chunk (16 bit)
    bytesToPost[bi] = hByte[2];bi++;  //
    bytesToPost[bi] = hByte[3];bi++;  // count(pixels) 16 bit
    bytesToPost[bi] = hByte[4];bi++;  //
    bytesToPost[bi] = hByte[5];bi++;  // protocol
    let r,g,b;
    for (var k = 0; k < pixLength; k++) {
        r = Math.round(pixels[k][0]*v_brightness.value);
        g = Math.round(pixels[k][1]*v_brightness.value);
        b = Math.round(pixels[k][2]*v_brightness.value);

        if (protocol.value === 'pix565' || protocol.value === 'rgb565') {
        // 565
        let rgbsum = Math.round(b/8)+Math.round(g/4)*32+Math.round(r/8)*2048;
        let first565 = Math.round(rgbsum/256);
        bytesToPost[bi] = first565;bi++;
        bytesToPost[bi] = Math.round(rgbsum-first565*256);bi++;

        } else {
        // pixels
        bytesToPost[bi] = r;bi++;
        bytesToPost[bi] = g;bi++;
        bytesToPost[bi] = b;bi++;
        }
    }

    sendUdp(bytesToPost);
  }
  
function draw(v,c,w,h) {
    if(v.paused || v.ended) return false;
    c.filter = "contrast("+contrast.value+")";
    c.drawImage(v,0,0,w,h);
    // Read image from canvas
    imageObj = c.getImageData(0, 0, parseInt(v_width.value), parseInt(v_height.value)*parseInt(v_units.value));
    pData = imageObj.data;
    let pixels = new Array;
    // Byte progression is R,G,B,A (We discard the 4th value)
   for (var i = 1; i <= pData.length+3; i ++) {
        
        if (i % 4 === 0) {
            pixels.push([
                pData[i-4],
                pData[i-3],
                pData[i-2],
            ]);
            continue;
        }
    } 
    convertChannel(pixels);
    
    setTimeout(draw,millis_frame.value,v,c,w,h);
}

function openSocket() {
    chrome.sockets.udp.create({}, function(socketInfo) {
        // The socket is created, now we can send some data
        socketId = socketInfo.socketId;
    
        chrome.sockets.udp.bind(socketId,
            "0.0.0.0", 0, function(result) {
              if (result < 0) {
                console.log("Error binding socket.");
                return;
              }
    });
    });
}

(function localFileVideoPlayer() {
	'use strict'
  var URL = window.URL || window.webkitURL
  var displayMessage = function (message, isError) {
    transmission.innerHTML = message
    transmission.className = isError ? 'error' : 'info'
  }
  canvas.width = parseInt(v_width.value);
  canvas.height = ch;
  var playSelectedFile = function (event) {
    var file = this.files[0]
    var type = file.type
    var videoNode = document.querySelector('video')
    var canPlay = videoNode.canPlayType(type)
    if (canPlay === '') canPlay = 'no'
    var message = 'Can play type "' + type + '": ' + canPlay
    var isError = canPlay === 'no'
    displayMessage(message, isError)

    if (isError) {
      return
    }

    var fileURL = URL.createObjectURL(file)
    videoNode.src = fileURL
  }
  var inputNode = document.querySelector('input')
  inputNode.addEventListener('change', playSelectedFile, false)
})()

/**
 * Saves form state to chrome.storage.local
 * @param $form to save in localstorage(jQuery object)
 */
function saveFormState($form) {
    const formData = JSON.stringify($form.serializeArray());
    chrome.storage.local.set({'form': formData});
  }
  
  /**
   * Loads form state from chrome.storage.local
   * @param $form to load from localstorage(jQuery object)
   */
  function loadFormState($form) {
    const formData = chrome.storage.local.get('form', function (result) {
        if (typeof result.form === "undefined") return;
        formKeyValue = JSON.parse(result.form);
        $.each( formKeyValue, function( key, value ) {
            const selector = $('input[name="'+value.name+'"]');
            selector.val(value.value);
        });
    });
    
  }
function cleanTransmission(){
    transmission.innerHTML = t_empty;
    transmission.className = 'white';
}