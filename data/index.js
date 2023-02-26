/**
 * ----------------------------------------------------------------------------
 * SML Smart Music Lamp with WebSocket
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initButton();
    initWebSocket();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    let data = JSON.parse(event.data);
    document.getElementById('led').className = data.status;
    document.getElementById("toggle").className = data.status;
    document.getElementById('Rainbowbutton').className = data.rainbowStatus;
    document.getElementById('Candychasebutton').className = data.candychaseStatus;
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('toggle').addEventListener('click', onToggle);
    document.getElementById('Rainbowbutton').addEventListener('click', onToggleRainbowEffect);
    document.getElementById('Candychasebutton').addEventListener('click', onToggleCandychaseEffect);
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

function onToggleRainbowEffect(event) {
    const toggle = document.getElementById("Rainbowbutton");
    var isOn;
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

function onToggleCandychaseEffect(event) {
    const toggle = document.getElementById("Candychasebutton");
    var isOn;
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
    console.log(toggle.className);
    console.log(json);
    websocket.send(json);
}