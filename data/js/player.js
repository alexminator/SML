// Player var
const play = document.querySelector(".play-btn");
const back = document.querySelector(".skip-left");
const forward = document.querySelector(".skip-right");
const volUp = document.querySelector(".vol-up");
const volDown = document.querySelector(".vol-down");
const controlBtn = document.querySelector(".btn-overlay");

//Play Button
play.addEventListener("click", function () {
});

play.addEventListener("mousedown", function () {
  play.classList.add("pressed");
});

play.addEventListener("mouseup", function () {
  play.classList.remove("pressed");
});

//Play touch
play.addEventListener("touchstart", function () {
  play.classList.add("pressed");
});

play.addEventListener("touchend", function () {
  play.classList.remove("pressed");
});

//Skip Left
back.addEventListener("click", () => {
  json.action = 'skipL';
  console.log(json);
  websocket.send(JSON.stringify(json));
});

back.addEventListener("mousedown", function () {
  controlBtn.classList.add("left");
});
back.addEventListener("mouseup", function () {
  controlBtn.classList.remove("left");
});

//Left Touch
back.addEventListener("touchstart", function () {
  controlBtn.classList.add("left");
});
back.addEventListener("touchend", function () {
  controlBtn.classList.remove("left");
});

//Skip Right
forward.addEventListener("click", () => {
  json.action = 'skipR';
  console.log(json);
  websocket.send(JSON.stringify(json));
});

forward.addEventListener("mousedown", function () {
  controlBtn.classList.add("right");
});
forward.addEventListener("mouseup", function () {
  controlBtn.classList.remove("right");
});

//Left Right
forward.addEventListener("touchstart", function () {
  controlBtn.classList.add("right");
});
forward.addEventListener("touchend", function () {
  controlBtn.classList.remove("right");
});

//Volume Up

volUp.addEventListener("click", (e) => {
  json.action = 'volup';
  console.log(json);
  websocket.send(JSON.stringify(json));
});

volUp.addEventListener("mousedown", function () {
  controlBtn.classList.add("up");
});
volUp.addEventListener("mouseup", function () {
  controlBtn.classList.remove("up");
});

// UP touch
volUp.addEventListener("touchstart", function () {
  controlBtn.classList.add("up");
});
volUp.addEventListener("touchend", function () {
  controlBtn.classList.remove("up");
});

//Volume Down

volDown.addEventListener("click", (e) => {
  json.action = 'voldown';
  console.log(json);
  websocket.send(JSON.stringify(json));
});

volDown.addEventListener("mousedown", function () {
  controlBtn.classList.add("down");
});
volDown.addEventListener("mouseup", function () {
  controlBtn.classList.remove("down");
});

// DOWN touch
volDown.addEventListener("touchstart", function () {
  controlBtn.classList.add("down");
});
volDown.addEventListener("touchend", function () {
  controlBtn.classList.remove("down");
});