const units = {
  Celcius: "°C",
  Fahrenheit: "°F" };

const config = {
  minTemp: -20,
  maxTemp: 50,
  unit: "Celcius" };
// Switch unit of temperature

const unitP = document.getElementById("unit");

unitP.addEventListener("click", function() {
  config.unit = config.unit === "Celcius" ? "Fahrenheit" : "Celcius";
  unitP.innerHTML = config.unit + ' ' + units[config.unit];
  return setTemperature();
});

// Change temperature

const range = document.querySelector("input[type='range']");
const temperature = document.getElementById("temperature");

function setTemperature() {
  temperature.style.height = (range.value - config.minTemp) / (config.maxTemp - config.minTemp) * 100 + "%";
  temperature.dataset.value = range.value + units[config.unit];
}

range.addEventListener("input", setTemperature);
setTimeout(setTemperature, 1000);