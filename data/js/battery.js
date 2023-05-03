// Bubbles charging
//document.querySelectorAll('.bubbles').style.display = 'none';
// Array with size of bubble
sArray = [4, 6, 8, 10];
// Helper function to get a random value from an array
function randomValue(arr) {
    return arr[Math.floor(Math.random() * arr.length)];
  }
  
  // setInterval function used to create a new bubble every 350 milliseconds
  setInterval(function () {
    if (sArray.length > 0) {
      // Get a random size, defined as variable so it can be used for both width and height
      var size = randomValue(sArray);
      var largestSize = Math.max.apply(Math, sArray);
      var offset = largestSize / 2; // half to get the largest bubble radius
      offset += 5; // 5px for border-right
  
      // New bubble appended to div with its size and left position being set inline
      document.querySelectorAll('.bubbles').forEach(function (bubbleContainer) {
        var bArray = new Array(parseInt(bubbleContainer.offsetWidth) - offset)
          .join()
          .split(',')
          .map(function (item, index) {
            return ++index;
          });
  
        var individualBubble = document.createElement('div');
        individualBubble.className = 'individual-bubble';
        individualBubble.style.left = randomValue(bArray) + 'px';
        individualBubble.style.width = size + 'px';
        individualBubble.style.height = size + 'px';
        bubbleContainer.appendChild(individualBubble);
  
        // Animate each bubble to the top (bottom 100%) and reduce opacity as it moves
        // Callback function used to remove finished animations from the page
        var startTime = performance.now();
        var duration = 3000;
        var initialOpacity = parseFloat(getComputedStyle(individualBubble).opacity);
  
        function animateBubble(timestamp) {
          var progress = (timestamp - startTime) / duration;
          individualBubble.style.bottom = (progress * 100) + '%';
          individualBubble.style.opacity = initialOpacity - (0.9 * progress);
  
          if (progress < 1) {
            requestAnimationFrame(animateBubble);
          } else {
            bubbleContainer.removeChild(individualBubble);
          }
        }
  
        requestAnimationFrame(animateBubble);
      });
    }
  }, 350);

/*=============== BATTERY ===============*/
function initBattery(batt) {
  const batteryLiquid = document.querySelector('.battery__liquid'),
        batteryStatus = document.querySelector('.battery__status'),
        batteryPercentage = document.querySelector('.battery__percentage');

  const t = (batt) => {
      updateBattery = () => {
          /* 1. We update the number level of the battery */
          let level = (batt.level);
          batteryPercentage.innerHTML = level+ '%';
          /* 2. We update the background level of the battery */
          batteryLiquid.style.height = `${batt.level}%`;
          
          /* 3. We validate full battery, low battery and if it is charging or not */
          if(level == 100){ /* We validate if battery is full */
              batteryStatus.innerHTML = `Full battery <span class="fa fa-battery-full green-color"></span>`;
              batteryLiquid.style.height = '103%'; /* To hide the ellipse */
          }
          else if (level <= 20 & !batt.charging) { /* We validate if battery is low */
              batteryStatus.innerHTML = `Low battery <span class="fa fa-plug animated-red"></span>`;      
          }
          else if (batt.charging){ /* We validate if battery its charging */
              batteryStatus.innerHTML = `Charging... <span class="fa fa-flash animated-green"></span>`;
              batteryLiquid.style.height = '20%';
              batteryPercentage.style.display = 'none';
              //document.querySelectorAll('.bubbles').style.display = 'inline-block';
          } 
          else { /* If its not loading, dont show anything. */
              batteryStatus.innerHTML = '';
          }
          
          /* 4. We change the colors of the battery and remove the other colors */
          if(level <= 20) {
              batteryLiquid.classList.add('gradient-color-red');
              batteryLiquid.classList.remove('gradient-color-orange','gradient-color-yellow','gradient-color-green');
          }
          else if (level <= 40) {
              batteryLiquid.classList.add('gradient-color-orange');
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-yellow','gradient-color-green');
          }
          else if (level <= 80) {
              batteryLiquid.classList.add('gradient-color-yellow');
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-orange','gradient-color-green');    
          }
          else {
              batteryLiquid.classList.add('gradient-color-green');
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-orange','gradient-color-yellow');
          }
          
      }
      updateBattery();
  }
  t(batt);
}
