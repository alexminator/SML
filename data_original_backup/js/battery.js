/*=============== BUBBLES ===============*/
const sArray = [4, 6, 8, 10]; // Array con tamaños de burbujas
let bubbleInterval;

const randomValue = (arr) => arr[Math.floor(Math.random() * arr.length)];

const animateBubble = (bubble, startTime, duration, initialOpacity) => {
  const animate = (timestamp) => {
    const progress = (timestamp - startTime) / duration;
    bubble.style.bottom = `${progress * 100}%`;
    bubble.style.opacity = initialOpacity - 0.9 * progress;

    if (progress < 1) {
      requestAnimationFrame(animate);
    } else {
      bubble.remove();
    }
  };
  requestAnimationFrame(animate);
};

const createBubble = () => {
  const bubbleContainers = document.querySelectorAll('.bubbles');
  if (bubbleContainers.length === 0) return;

  const size = randomValue(sArray);
  const largestSize = Math.max(...sArray);
  const offset = largestSize / 2 + 5;

  bubbleContainers.forEach((bubbleContainer) => {
    const bArray = Array.from(
      { length: bubbleContainer.offsetWidth - offset },
      (_, i) => i + 1
    );

    const individualBubble = document.createElement('div');
    individualBubble.className = 'individual-bubble';
    individualBubble.style.left = `${randomValue(bArray)}px`;
    individualBubble.style.width = `${size}px`;
    individualBubble.style.height = `${size}px`;
    bubbleContainer.appendChild(individualBubble);

    const startTime = performance.now();
    const duration = 3000;
    const initialOpacity = parseFloat(getComputedStyle(individualBubble).opacity);

    animateBubble(individualBubble, startTime, duration, initialOpacity);
  });
};

const startBubbles = () => {
  bubbleInterval = setInterval(createBubble, 350);
};

const stopBubbles = () => {
  clearInterval(bubbleInterval);
};

/*=============== BATTERY ===============*/
const initBattery = (batt) => {
  const batteryLiquid = document.querySelector('.battery__liquid');
  const batteryStatus = document.querySelector('.battery__status');
  const batteryPercentage = document.querySelector('.battery__percentage');

  stopBubbles();

  const updateBattery = () => {
    const level = batt.level;
    batteryPercentage.textContent = `${level}%`;
    batteryLiquid.style.height = `${level}%`;

    if (level === 100 && !batt.charging) {
      batteryStatus.innerHTML = 'Full battery <span class="fas fa-battery-full green-color"></span>';
      batteryLiquid.style.height = '103%';
    } else if (level <= 30) {
      batteryStatus.innerHTML = 'Low battery <span class="fas fa-plug animated-red"></span>';
    } else if (batt.fullbatt) {
      batteryStatus.innerHTML = 'Power in use... <span class="fas fa-plug animated-green"></span>';
    } else if (batt.charging) {
      batteryStatus.innerHTML = 'Charging... <span class="fas fa-bolt-lightning animated-green"></span>';
      startBubbles();
    } else {
      batteryStatus.innerHTML = 'In use... <span class="fas fa-battery-quarter animated-green"></span>';
    }

    const colors = ['gradient-color-red', 'gradient-color-orange', 'gradient-color-yellow', 'gradient-color-green'];
    const colorIndex = level <= 20 ? 0 : level <= 40 ? 1 : level <= 80 ? 2 : 3;
    batteryLiquid.classList.remove(...colors);
    batteryLiquid.classList.add(colors[colorIndex]);
  };

  updateBattery();
};