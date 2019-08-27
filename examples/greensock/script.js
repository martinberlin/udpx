var div1 = $("div#div1"),
    div2 = $("div#div2"),
    btn1 = $("button#btn1"),
    btn2 = $("button#btn2"),
    
    tl1 = TweenMax.to(div1, 2, {left:300, onUpdate:lineUpdate, onUpdateParams:["{self}"], paused:true});

function lineUpdate(tween)
{
  tl1.progress(tween.progress());
}

btn1.click(function()
{
  tl1.play(0);
});
btn2.click(function()
{
  tl1.reverse();
});