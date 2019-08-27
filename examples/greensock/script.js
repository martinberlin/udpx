var stripe = $("div#stripe"),
    btn1 = $("button#btn1"),
    btn2 = $("button#btn2"),
    red = $("#r"), green = $("#g"), blue = $("#b"),
    delay = $("#duration");

    tl1 = TweenMax.to(stripe, delay.val(), {
      left:300,
      onUpdate:lineUpdate,
      onUpdateParams:["{self}"],
      paused:true
     });


function lineUpdate(tween)
{
  tl1.progress(tween.progress());
}

btn1.click(function()
{
  stripe.css("background-color", "rgb(" + red.val() + "," + green.val() + "," + blue.val() +")");
  tl1.duration(delay.val());
  tl1.play(0);
});
btn2.click(function()
{
  tl1.duration(delay.val());
  tl1.reverse();
});