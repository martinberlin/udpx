var stripe = $("div#stripe"),
    btn1 = $("button#btn1"),
    btn2 = $("button#btn2"),
    red = $("#r"), green = $("#g"), blue = $("#b"),
    stripe_length = $("#stripe_length"),
    json_out = $("#json_out"),
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

function lineUpdate(tween)
{
  tl1.progress(tween.progress());
  stripepos = stripe.position();
  stripex = stripepos.left -10;
  ledstripex = parseInt((stripex*stripe_length.val())/288);

  var json = [];
  rgb = [parseInt(red.val()), parseInt(green.val()), parseInt(blue.val())];
  for (var k = 1; k <= parseInt(stripe_length.val()); k++) {
    if (ledstripex >= k-1 && ledstripex <= k+1) {
      json.push(rgb);
    } else {
      json.push([0,0,0]);
    }
    
  }
  let jsonString = JSON.stringify(json);

  if (jsonString !== lastPush) {
    compressed = compression.is(":checked") ?1:0;
    $.ajax("http://127.0.0.1:1234?ip="+ 
            ip.val() +"&c="+ compressed,
            {
            'data': jsonString, 
            'type': 'POST',
            async: false
            });
    json_out.val(jsonString);
  }
  lastPush = jsonString;
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