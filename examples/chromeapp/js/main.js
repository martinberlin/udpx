chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('app.html', {
  	id: "mainwin",
    bounds: {
      width: 1050,
      height: 500
    }
  });
});
