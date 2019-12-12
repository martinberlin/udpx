
let minizBase = 'js/miniz/';
function fetchLocal(url) {
  return new Promise(function(resolve, reject) {
    var xhr = new XMLHttpRequest
    xhr.responseType = "arraybuffer";
    xhr.onload = function() {
      resolve(new Response(new Uint8Array(xhr.response), {status: xhr.status}))
    }
    xhr.onerror = function() {
      reject(new TypeError('Local request failed'))
    }
    xhr.open('GET', url)
    xhr.send(null)
  })
}