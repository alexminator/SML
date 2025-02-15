let connected = false;
let brightness = "";
const json = {
    action: '',
    effectId: 0,
    color: { r: 0, g: 0, b: 0 },
    brightness: 0
};

const units = {
    Celcius: "°C",
    Fahrenheit: "°F" };
  
  const config = {
    minTemp: 0,
    maxTemp: 50,
    unit: "Celcius" };

const batt = { "level": 0, "charging": false };

document.addEventListener('DOMContentLoaded', () => {
    const tabs = document.querySelectorAll('ul.tabs li a');
    const sections = document.querySelectorAll('.sections > div');
    let activeTabIndex = 0; // Variable to store the index of the active tab

    tabs[0].classList.add('active');
    sections.forEach(section => section.style.display = 'none');
    sections[0].style.display = 'block';

    tabs.forEach((tab, index) => {
        tab.addEventListener('click', function (event) {
            event.preventDefault();
            tabs.forEach(tab => tab.classList.remove('active'));
            this.classList.add('active');
            sections.forEach(section => section.style.display = 'none');
            const activeTab = document.querySelector(this.getAttribute('href'));
            activeTab.style.display = 'block';

            // Update active tab index
            activeTabIndex = index;

            // Show in the console which tab and section are active
            console.log(`Tab seleccionada: ${this.textContent.trim()}`);
            console.log(`Sección activa: ${activeTab.id}`);
        });
    });
});

window.onload = function () {
    brightness = parseInt(document.getElementById("pwmSlider").value);
    const colors = document.getElementById("picker_bridge").className;
    const obj = JSON.parse(colors);
    json.color = { r: obj.color.r, g: obj.color.g, b: obj.color.b };
    // Create a new color picker instance
    var colorPicker = new iro.ColorPicker("#wheelPicker", {
        width: 250,
        color: json.color,
        borderWidth: 2,
        borderColor: "#fff",
        layout: [{ component: iro.ui.Wheel }]
    });

    colorPicker.on('color:change', (color) => {
        document.getElementById("butterfly").style.setProperty('--butterfly-color', color.hexString);
        json.action = 'picker';
        json.color = [color.red, color.green, color.blue];
        json.brightness = brightness;
        sendJson();
    });
}
// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------
const gateway = `ws://${window.location.hostname}/ws`;
let websocket;

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

function onOpen() {
    console.log('Connection opened');
    connected = true;
    setStatus();
}

function onClose() {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
    connected = false;
    setStatus();
}

function onMessage(evt) {
    // Print out our received message
    console.log("Received: " + evt.data);
    const data = JSON.parse(evt.data);
    //BATT
    batt.level = data.level;
    batt.charging = data.charging;
    batt.fullbatt = data.fullbatt;
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
    document.getElementById("Bluetooth").className = data.btstatus;
    document.getElementById("lamp").className = data.lampstatus;
    document.getElementById("picker_bridge").className = data.color;
    document.getElementById("textSliderValue").innerHTML = data.neobrightness;
    document.getElementById("pwmSlider").value = data.neobrightness;
    //Buttons effects
    updateButtonStatus('Firebutton', data.fireStatus);
    updateButtonStatus('MovingDotbutton', data.movingdotStatus);
    updateButtonStatus('RainbowBeatbutton', data.rainbowbeatStatus);
    updateButtonStatus('RWBbutton', data.rwbStatus);
    updateButtonStatus('Ripplebutton', data.rippleStatus);
    updateButtonStatus('Twinklebutton', data.twinkleStatus);
    updateButtonStatus('Ballsbutton', data.ballsStatus);
    updateButtonStatus('Jugglebutton', data.juggleStatus);
    updateButtonStatus('Sinelonbutton', data.sinelonStatus);
    updateButtonStatus('Cometbutton', data.cometStatus);
    //Buttons VU
    updateButtonStatus('RainbowVU', data.rainbowVUStatus);
    updateButtonStatus('OldSkoolVU', data.oldVUStatus);
    updateButtonStatus('RainbowHueVU', data.rainbowHueVUStatus);
    updateButtonStatus('RippleVU', data.rippleVUStatus);
    updateButtonStatus('ThreebarsVU', data.threebarsVUStatus);
    updateButtonStatus('OceanVU', data.oceanVUStatus);
    //Buttons Indicators
    updateButtonStatus('TempNEO', data.tempNEOStatus);
    updateButtonStatus('BattNEO', data.battNEOStatus);
}

function onError(error) {
    console.log('WebSocket Error ', error);
}

function setStatus() {
    const stat = document.getElementById('status');
    const ind = document.getElementById('indicator');
    const lvl = document.getElementById('Signal');
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
    const buttons = [
        'lamp', 'Neo', 'Bluetooth', 'Firebutton', 'MovingDotbutton', 'RainbowBeatbutton',
        'RWBbutton', 'Ripplebutton', 'Twinklebutton', 'Ballsbutton', 'Jugglebutton',
        'Sinelonbutton', 'Cometbutton', 'RainbowVU', 'OldSkoolVU', 'RainbowHueVU',
        'RippleVU', 'ThreebarsVU', 'OceanVU', 'TempNEO', 'BattNEO'
    ];

    buttons.forEach(buttonId => {
        document.getElementById(buttonId).addEventListener('click', () => handleButtonClick(buttonId));
    });

    document.getElementById('pwmSlider').addEventListener('change', onChangeBrightness);
}

// Mapeo de efectos para cada botón
const effectMap = {
    Firebutton: 1,
    MovingDotbutton: 2,
    RainbowBeatbutton: 3,
    RWBbutton: 4,
    Ripplebutton: 5,
    Twinklebutton: 6,
    Ballsbutton: 7,
    Jugglebutton: 8,
    Sinelonbutton: 9,
    Cometbutton: 10,
    RainbowVU: 11,
    OldSkoolVU: 12,
    RainbowHueVU: 13,
    RippleVU: 14,
    ThreebarsVU: 15,
    OceanVU: 16,
    TempNEO: 17,
    BattNEO: 18
};

function handleButtonClick(buttonId) {
    const toggle = document.getElementById(buttonId);
    const isOn = toggle.className === "on" ? 0 : effectMap[buttonId] || 0; // Usar el valor del mapeo o 0 si no existe
    toggle.className = isOn ? "on" : "off";

    json.action = getActionFromButtonId(buttonId);
    json.effectId = isOn;
    sendJson();
}

function getActionFromButtonId(buttonId) {
    switch (buttonId) {
        case 'lamp': return 'lamp';
        case 'Neo': return 'toggle';
        case 'Bluetooth': return 'music';
        case 'Firebutton': return 'animation';
        case 'MovingDotbutton': return 'animation';
        case 'RainbowBeatbutton': return 'animation';
        case 'RWBbutton': return 'animation';
        case 'Ripplebutton': return 'animation';
        case 'Twinklebutton': return 'animation';
        case 'Ballsbutton': return 'animation';
        case 'Jugglebutton': return 'animation';
        case 'Sinelonbutton': return 'animation';
        case 'Cometbutton': return 'animation';
        case 'RainbowVU': return 'vu';
        case 'OldSkoolVU': return 'vu';
        case 'RainbowHueVU': return 'vu';
        case 'RippleVU': return 'vu';
        case 'ThreebarsVU': return 'vu';
        case 'OceanVU': return 'vu';
        case 'TempNEO': return 'indicator';
        case 'BattNEO': return 'indicator';
        default: return '';
    }
}

function onChangeBrightness() {
    brightness = document.getElementById("pwmSlider").value;
    document.getElementById("textSliderValue").innerHTML = brightness;
    json.action = 'slider';
    json.brightness = brightness;
    sendJson();
}

function sendJson() {
    console.log(json);
    websocket.send(JSON.stringify(json));
}

function updateButtonStatus(buttonId, status) {
    document.getElementById(buttonId).className = status;
}