var currentDate = new Date();
var day = currentDate.getDate();
var month = currentDate.getMonth() + 1;
var year = currentDate.getFullYear();
var displayDate = day + "/" + month + "/" + year;

var currentTime = new Date().toLocaleTimeString('en-US', {
  hour: 'numeric',
  minute: 'numeric',
  hour12: true
});

document.addEventListener("DOMContentLoaded", function() {
  var time = document.getElementById("time");
  var timeIcon = document.querySelector("#time-icon i");
  var date = document.getElementById("date");
  
  // displaying current date and time
  time.textContent = currentTime;
  date.textContent = displayDate;
});
