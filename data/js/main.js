var connected = false;
var isOn;
let brightness = "";
const json = {
    'action': '',
    'effectId': 0,
    'color': { 'r': 0, 'g': 0, 'b': 0 },
    'brightness': 0
};

const units = {
    Celcius: "°C",
    Fahrenheit: "°F" };
  
  const config = {
    minTemp: 0,
    maxTemp: 50,
    unit: "Celcius" };

const batt = { "level": 0, "charging": false };

document.addEventListener('DOMContentLoaded', function () {
    const tabs = document.querySelectorAll('ul.tabs li a');
    const sections = document.querySelectorAll('.sections > div');

    tabs[0].classList.add('active');
    sections.forEach(section => section.style.display = 'none');
    sections[0].style.display = 'block';

    tabs.forEach(tab => {
        tab.addEventListener('click', function (event) {
            event.preventDefault();
            tabs.forEach(tab => tab.classList.remove('active'));
            this.classList.add('active');
            sections.forEach(section => section.style.display = 'none');
            const activeTab = document.querySelector(this.getAttribute('href'));
            activeTab.style.display = 'block';
        });
    });
});

window.onload = function () {
    brightness = parseInt(document.getElementById("pwmSlider").value);
    var colors = document.getElementById("picker_bridge").className;
    const json_colors = colors;
    const obj = JSON.parse(json_colors);
    json.color.r = colorR = obj.color.r;
    json.color.g = colorG = obj.color.g;
    json.color.b = colorB = obj.color.b;
    // Create a new color picker instance
    var colorPicker = new iro.ColorPicker("#wheelPicker", {
        width: 250,
        color: { r: colorR, g: colorG, b: colorB },
        borderWidth: 2,
        borderColor: "#fff",
        layout: [{ component: iro.ui.Wheel, },]
    });

    colorPicker.on('color:change', function (color) {
        document.getElementById("butterfly").style.setProperty('--butterfly-color', color.hexString);
        json.action = 'picker';
        json.color = [color.red, color.green, color.blue];
        json.brightness = brightness;
        console.log(json);
        websocket.send(JSON.stringify(json));
    });
}
// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initButton();

}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
    websocket.onerror = onError;
}

function onOpen(event) {
    console.log('Connection opened');
    connected = true;
    setStatus();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
    connected = false;
    setStatus();
}

function onMessage(event) {
    // Print out our received message
    console.log("Received: " + event.data);
    var data = JSON.parse(event.data);
    //BATT
    batt.level = data.level;
    batt.charging = data.charging;
    initBattery(batt);
    document.getElementById('battVolt').innerHTML = data.battVoltage + ' V';
    // DHT
    const externalTemperature = data.temperature;
    temperature.style.height = (externalTemperature - config.minTemp) / (config.maxTemp - config.minTemp) * 100 + "%";
    temperature.dataset.value = externalTemperature + units[config.unit];
    document.getElementById('temp').innerHTML = data.temperature + ' &deg;C';
    document.getElementById('hum').innerHTML = data.humidity + ' %';
    // WiFi
    document.getElementById('Signal').className = data.bars;
    // Neo Effects
    document.getElementById("Neo").className = data.neostatus;
    document.getElementById("lamp").className = data.lampstatus;
    document.getElementById("picker_bridge").className = data.color;
    document.getElementById("textSliderValue").innerHTML = data.neobrightness;
    document.getElementById("pwmSlider").value = data.neobrightness;
    document.getElementById('Firebutton').className = data.fireStatus;
    document.getElementById('MovingDotbutton').className = data.movingdotStatus;
    document.getElementById('RainbowBeatbutton').className = data.rainbowbeatStatus;
    document.getElementById('RWBbutton').className = data.rwbStatus;
    document.getElementById('Ripplebutton').className = data.rippleStatus;
    document.getElementById('Twinklebutton').className = data.twinkleStatus;
    document.getElementById('Ballsbutton').className = data.ballsStatus;
    document.getElementById('Jugglebutton').className = data.juggleStatus;
    document.getElementById('Sinelonbutton').className = data.sinelonStatus;
    document.getElementById('Cometbutton').className = data.cometStatus;
    //VU
    document.getElementById('RainbowVU').className = data.rainbowVUStatus;
    document.getElementById('Old-skoolVU').className = data.oldVUStatus;
    document.getElementById('RainbowHueVU').className = data.rainbowHueVUStatus;
    document.getElementById('RippleVU').className = data.rippleVUStatus;
    document.getElementById('ThreebarsVU').className = data.threebarsVUStatus;
    document.getElementById('OceanVU').className = data.oceanVUStatus;
    document.getElementById('BlendingVU').className = data.blendingVUStatus;
}

function onError(event) {
    console.log('WebSocket Error ', error);
}

function setStatus() {
    var stat = document.getElementById('status');
    var ind = document.getElementById('indicator');
    var lvl = document.getElementById('Signal');
    if (connected) {
        stat.innerHTML = "Connected";
        ind.style.backgroundColor = '#0f0';
    } else {
        stat.innerHTML = "Disconnected";
        ind.style.backgroundColor = 'red';
        lvl.className = "no-signal";
    }
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('lamp').addEventListener('click', onToggleLamp);
    document.getElementById('Neo').addEventListener('click', onToggleNeo);
    document.getElementById('pwmSlider').addEventListener('change', onChangeBrightness);
    document.getElementById('Firebutton').addEventListener('click', onToggleFireEffect);
    document.getElementById('MovingDotbutton').addEventListener('click', onToggleMovingDotEffect);
    document.getElementById('RainbowBeatbutton').addEventListener('click', onToggleRainbowBeatEffect);
    document.getElementById('RWBbutton').addEventListener('click', onToggleRWBEffect);
    document.getElementById('Ripplebutton').addEventListener('click', onToggleRippleffect);
    document.getElementById('Twinklebutton').addEventListener('click', onToggleTwinkleffect);
    document.getElementById('Ballsbutton').addEventListener('click', onToggleBallseffect);
    document.getElementById('Jugglebutton').addEventListener('click', onToggleJuggleeffect);
    document.getElementById('Sinelonbutton').addEventListener('click', onToggleSineloneffect);
    document.getElementById('Cometbutton').addEventListener('click', onToggleCometeffect);
    //VU
    document.getElementById('RainbowVU').addEventListener('click', onToggleRainbowVU);
    document.getElementById('Old-skoolVU').addEventListener('click', onToggleOldVU);
    document.getElementById('RainbowHueVU').addEventListener('click', onToggleRainbowHueVU);
    document.getElementById('RippleVU').addEventListener('click', onToggleRippleVU);
    document.getElementById('ThreebarsVU').addEventListener('click', onToggle3barsVU);
    document.getElementById('OceanVU').addEventListener('click', onToggleOceanVU);
    document.getElementById('BlendingVU').addEventListener('click', onToggleBlendingVU);
}

function onToggleLamp(event) {
    const toggle = document.getElementById("lamp");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    json.action = 'lamp';
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleNeo(event) {
    const toggle = document.getElementById("Neo");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    json.action = 'toggle';
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onChangeBrightness(event) {
    brightness = document.getElementById("pwmSlider").value;
    document.getElementById("textSliderValue").innerHTML = brightness;
    console.log(brightness);
    json.action = 'slider';
    json.brightness = brightness;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleFireEffect(event) {
    const toggle = document.getElementById("Firebutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 1;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleMovingDotEffect(event) {
    const toggle = document.getElementById("MovingDotbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 2;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRainbowBeatEffect(event) {
    const toggle = document.getElementById("RainbowBeatbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 3;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRWBEffect(event) {
    const toggle = document.getElementById("RWBbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 4;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRippleffect(event) {
    const toggle = document.getElementById("Ripplebutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 5;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleTwinkleffect(event) {
    const toggle = document.getElementById("Twinklebutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 6;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleBallseffect(event) {
    const toggle = document.getElementById("Ballsbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 7;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleJuggleeffect(event) {
    const toggle = document.getElementById("Jugglebutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 8;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleSineloneffect(event) {
    const toggle = document.getElementById("Sinelonbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 9;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleCometeffect(event) {
    const toggle = document.getElementById("Cometbutton");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 10;
        toggle.className = "on";
    }
    json.action = 'animation';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRainbowVU(event) {
    const toggle = document.getElementById("RainbowVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 11;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleOldVU(event) {
    const toggle = document.getElementById("Old-skoolVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 12;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRainbowHueVU(event) {
    const toggle = document.getElementById("RainbowHueVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 13;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleRippleVU(event) {
    const toggle = document.getElementById("RippleVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 14;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggle3barsVU(event) {
    const toggle = document.getElementById("ThreebarsVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 15;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleOceanVU(event) {
    const toggle = document.getElementById("OceanVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 16;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function onToggleBlendingVU(event) {
    const toggle = document.getElementById("BlendingVU");
    if (toggle.className == "on") {
        isOn = 0;
        toggle.className = "off";
    } else {
        isOn = 17;
        toggle.className = "on";
    }
    json.action = 'vu';
    json.effectId = isOn;
    console.log(json);
    websocket.send(JSON.stringify(json));
}