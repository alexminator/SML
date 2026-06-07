/*=============== BUBBLES ===============*/
const sArray = [4, 6, 8, 10];
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
    const containerWidth = bubbleContainer.offsetWidth - offset;
    if (containerWidth <= 0) return; // no space for bubbles

    const bArray = Array.from(
      { length: containerWidth },
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

  stopBubbles();

  const updateBattery = () => {
    const level = batt.level;

    // Liquid fill
    batteryLiquid.style.height = `${level}%`;

    // Color by level
    const colors = ['gradient-color-red', 'gradient-color-orange', 'gradient-color-yellow', 'gradient-color-green'];
    const colorIndex = level <= 20 ? 0 : level <= 40 ? 1 : level <= 80 ? 2 : 3;
    batteryLiquid.classList.remove(...colors);
    batteryLiquid.classList.add(colors[colorIndex]);

    // Bubbles when charging
    if (batt.charging) {
      startBubbles();
    }
  };

  updateBattery();
};