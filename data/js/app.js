var hex;
var lastHex;
var connected = false;
var isOn;

window.onload = function () {
    // Create a new color picker instance
    let colorPicker = new ColorPickerControl({ container: document.querySelector('.color-picker-dark-theme'), theme: 'dark' });

    colorPicker.on(["color:init", "color:change"], function (color) {
        hex = color.hexString;
        document.getElementById('buttons').style.borderTop = '1px solid' + hex;
    });

    colorPicker.on('change', (color) =>  {
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
    document.getElementById('Signal').className = data.bars;
    document.getElementById("rssi").innerHTML = data.signalStrength;
    document.getElementById("toggle").className = data.status;
    document.getElementById('Firebutton').className = data.fireStatus;
    document.getElementById('MovingDotbutton').className = data.movingdotStatus;
    document.getElementById('RainbowBeatbutton').className = data.rainbowbeatStatus;
    document.getElementById('RWBbutton').className = data.rwbStatus;
    document.getElementById('Ripplebutton').className = data.rippleStatus;
    document.getElementById('Twinklebutton').className = data.twinkleStatus;

}

function onError(event) {
    console.log('WebSocket Error ', error);
}

function setStatus() {
    var stat = document.getElementById('status');
    var ind = document.getElementById('indicator');
    var lvl = document.getElementById('Signal');
    var rssi = document.getElementById("rssi");
    if (connected) {
        stat.innerHTML = "Connected";
        ind.style.backgroundColor = '#0f0';
    } else {
        stat.innerHTML = "Disconnected";
        ind.style.backgroundColor = 'red';
        rssi.innerHTML = 0;
        lvl.className = "no-signal";
    }
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('toggle').addEventListener('click', onToggle);
    document.getElementById('Firebutton').addEventListener('click', onToggleFireEffect);
    document.getElementById('MovingDotbutton').addEventListener('click', onToggleMovingDotEffect);
    document.getElementById('RainbowBeatbutton').addEventListener('click', onToggleRainbowBeatEffect);
    document.getElementById('RWBbutton').addEventListener('click', onToggleRWBEffect);
    document.getElementById('Ripplebutton').addEventListener('click', onToggleRippleffect);
    document.getElementById('Twinklebutton').addEventListener('click', onToggleTwinkleffect);

}

function onToggle(event) {
    const toggle = document.getElementById("toggle");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    const json = JSON.stringify({
        'action': 'toggle',
        'effectId': 0,
        'color': "r=255,g=0,b=0"
    });
    console.log(json);
    websocket.send(json);
}

function onToggleSimpleColor(event) {
    const toggle = document.getElementById("SimpleColor");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    // TODO: Tomar los valores del picker del color
    const json = JSON.stringify({
        'action': 'animation',
        'effectId': 0,
        'color': "r=255,g=0,b=0"
    });
    console.log(json);
    websocket.send(json);
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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
        'effectId': isOn,
        'color': "r=255,g=0,b=0"
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

