// Variables del reproductor
const play = document.querySelector('.play-btn');
const back = document.querySelector('.skip-left');
const forward = document.querySelector('.skip-right');
const volUp = document.querySelector('.vol-up');
const volDown = document.querySelector('.vol-down');
const controlBtn = document.querySelector('.btn-overlay');

// Función para enviar comandos a través de WebSocket
const sendWebSocketCommand = (action) => {
  if (websocket.readyState === WebSocket.OPEN) {
    json.action = action;
    websocket.send(JSON.stringify(json));
  } else {
    console.error('WebSocket is not open');
  }
};

// Función para agregar efectos de presión (mousedown, mouseup, touchstart, touchend)
const addPressEffect = (element, className) => {
  element.addEventListener('pointerdown', () => controlBtn.classList.add(className));
  element.addEventListener('pointerup', () => controlBtn.classList.remove(className));
};

// Función para configurar eventos de botones
const setupButton = (element, action, className) => {
  element.addEventListener('click', () => sendWebSocketCommand(action));
  addPressEffect(element, className);
};

// Configurar controles del reproductor
setupButton(play, 'play-pause', 'pressed');
setupButton(back, 'skipL', 'left');
setupButton(forward, 'skipR', 'right');
setupButton(volUp, 'volup', 'up');
setupButton(volDown, 'voldown', 'down');