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
    document.getElementById('led').className = data.stripledStatus;
    document.getElementById("Stripledtoggle").className = data.stripledStatus;
    document.getElementById('Rainbowtoggle').className = data.rainbowStatus;
    document.getElementById('Theatertoggle').className = data.theaterStatus;
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('Stripledtoggle').addEventListener('click', onToggleStripled);
    document.getElementById('Rainbowtoggle').addEventListener('click', onToggleRainbow);
    document.getElementById('Theatertoggle').addEventListener('click', onToggleTheater);
}

function onToggleStripled(event) {
    const toggle = document.getElementById("Stripledtoggle");
    if (toggle.className == "on") {
        toggle.className = "off";
        const json = JSON.stringify({
            "action": "toggle",
            "pattern": 0,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    } else {
        toggle.className = "on";
        const json = JSON.stringify({
            "action": "toggle",
            "pattern": 1,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    }
}

function onToggleRainbow(event) {
    const toggle = document.getElementById("Rainbowtoggle");
    if (toggle.className == "on") {
        toggle.className = "off";
        const json = JSON.stringify({
            "action": "toggle",
            "pattern": 1,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    } else {
        toggle.className = "on";
        const json = JSON.stringify({
            "action": "animation",
            "pattern": 2,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    }
}

function onToggleTheater(event) {
    const toggle = document.getElementById("Theatertoggle");
    if (toggle.className == "on") {
        toggle.className = "off";
        const json = JSON.stringify({
            "action": "toggle",
            "pattern": 1,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    } else {
        toggle.className = "on";
        const json = JSON.stringify({
            "action": "animation",
            "pattern": 3,
            "color": "r=255,g=0,b=0"
        });
        console.log(json);
        websocket.send(json);
    }
}


