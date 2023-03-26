var hex;
var lastHex;
var connected = false;
var isOn;

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
    // Create a new color picker instance
    let colorPicker = new ColorPickerControl({ container: document.querySelector('.color-picker-dark-theme'), theme: 'dark' });

    colorPicker.on(["color:init", "color:change"], function (color) {
        hex = color.hexString;
        document.getElementById('buttons').style.borderTop = '1px solid' + hex;
    });

    colorPicker.on('change', (color) => {
        document.getElementById("butterfly").style.setProperty('--butterfly-color', color.toHEX());
        document.getElementById("butterfly").style.setProperty('--butterfly-opacity', color.a / 255);
        light_color_picker.color.fromHSVa(color.h, color.s, color.v, color.a);
    });

    // update the "selected color" values whenever the color changes
    var values = document.getElementById("values");
    colorPicker.on(["color:init", "color:change"], function (color) {
        // Show the current color in different formats
        values.innerHTML = [
            "hex: " + color.hexString,
            "rgb: " + color.rgbString,
            "hsl: " + color.hslString,
        ].join("<br>");
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
    console.log(data);
    document.getElementById('Signal').className = data.bars;
    document.getElementById("Neo").className = data.neostatus;
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
}

function onToggleNeo(event) {
    const toggle = document.getElementById("Neo");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    const json = JSON.stringify({
        'action': 'toggle',
        'effectId': 0,
    });
    console.log(json);
    websocket.send(json);
}

function onChangeBrightness(event) {
    var brightness = document.getElementById("pwmSlider").value;
    document.getElementById("textSliderValue").innerHTML = brightness;
    console.log(brightness);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value=" + brightness, true);
    xhr.send();
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
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
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': isOn
    });
    console.log(json);
    websocket.send(json);
}

/*
function checkColor() {
    if (isOn) {
        if (hex != lastHex) {
            sendMessage(hex);
        }
        lastHex = hex;
    }
}

function sendCommand(value) {
    sendMessage(value, '!');
}

function power() {
    isOn = !isOn;
    if (!isOn)
        sendMessage('#000000');
    else
        sendMessage(hex);
}
*/