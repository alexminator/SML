// Player var
const play = document.querySelector(".play-btn");
const back = document.querySelector(".skip-left");
const forward = document.querySelector(".skip-right");
const volUp = document.querySelector(".vol-up");
const volDown = document.querySelector(".vol-down");
const controlBtn = document.querySelector(".btn-overlay");

//Play Button
play.addEventListener("click", function () {
  audioPlayer[songPlaying].volume = 0.1;
  toggleAudio(songPlaying);
});

play.addEventListener("mousedown", function () {
  play.classList.add("pressed");
});

play.addEventListener("mouseup", function () {
  play.classList.remove("pressed");
});

//Skip Left
back.addEventListener("click", () => {
  if (songPlaying !== 0) {
    songPlaying--;
  } else {
    songPlaying = 2;
  }
  audioPlayer[songPlaying].volume = 0.1;
  toggleAudio(songPlaying);
  setSongImg();
});

back.addEventListener("mousedown", function () {
  controlBtn.classList.add("left");
});
back.addEventListener("mouseup", function () {
  controlBtn.classList.remove("left");
});

//Skip Right
forward.addEventListener("click", () => {
  if (songPlaying !== 2) {
    songPlaying++;
  } else {
    songPlaying = 0;
  }
  audioPlayer[songPlaying].volume = 0.1;
  setSongImg();
  toggleAudio(songPlaying);
});

forward.addEventListener("mousedown", function () {
  controlBtn.classList.add("right");
});
forward.addEventListener("mouseup", function () {
  controlBtn.classList.remove("right");
});

//Volume Up

volUp.addEventListener("click", (e) => {
  if (audioPlayer[songPlaying].volume < 1) {
    audioPlayer[songPlaying].volume += 0.1;
  }
});

volUp.addEventListener("mousedown", function () {
  controlBtn.classList.add("up");
});
volUp.addEventListener("mouseup", function () {
  controlBtn.classList.remove("up");
});

//Volume Down

volDown.addEventListener("click", (e) => {
  if (audioPlayer[songPlaying].volume > 0.1) {
    audioPlayer[songPlaying].volume -= 0.1;
  }
});

volDown.addEventListener("mousedown", function () {
  controlBtn.classList.add("down");
});

volDown.addEventListener("mouseup", function () {
  controlBtn.classList.remove("down");
});