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
    document.getElementById('Theatertoogle').className = data.theaterStatus;
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('Stripledtoggle').addEventListener('click', onToggleStripled);
    document.getElementById('Rainbowtoggle').addEventListener('click', onToggleRainbow);
    document.getElementById('Theatertoogle').addEventListener('click', onToggleTheater);
}

function onToggleStripled(event) {
    const toggle = document.getElementById("Stripledtoggle");
    if (toggle.className == "on") {
        toggle.className = "off";
    } else {
        toggle.className = "on";
    }
    const json = JSON.stringify({
        'action': 'toggle',
        'ActivePattern': 'COLOR',
        'direction': 'dir',
        'color': "r=255,g=0,b=0"
    });
    console.log(json);
    websocket.send(json);
}

function onToggleRainbow(event) {
    const toggle = document.getElementById("Rainbowtoggle");
    
    if (toggle.className == "on") {
        
        toggle.className = "off";
    } else {
        
        toggle.className = "on";
    }
    const json = JSON.stringify({
        'action': 'animation',
        'ActivePattern': 'RAINBOW_CYCLE',
        'direction': 'dir',
        'color': "r=255,g=0,b=0"
    });
    console.log(json);
    websocket.send(json);
}

function onToggleTheater(event) {
    const toggle = document.getElementById("Theatertoogle");
    
    if (toggle.className == "on") {
        
        toggle.className = "off";
    } else {
        
        toggle.className = "on";
    }
    const json = JSON.stringify({
        'action': 'animation',
        'ActivePattern': 'THEATER_CHASE',
        'direction': 'dir',
        'color': "r=255,g=0,b=0"
    });
    console.log(toggle.className);
    console.log(json);
    websocket.send(json);
}