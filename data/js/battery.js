/*=============== BATTERY ===============*/
//initBattery()

function initBattery( batt ) {
  const batteryLiquid = document.querySelector('.battery__liquid'),
        batteryStatus = document.querySelector('.battery__status'),
        batteryPercentage = document.querySelector('.battery__percentage')
  
  const t = (batt) => {
      updateBattery = () => {
          /* 1. We update the number level of the battery */
          let level = (batt.level)
          batteryPercentage.innerHTML = level+ '%'
          
          /* 2. We update the background level of the battery */
          batteryLiquid.style.height = `${batt.level}%`
          
          /* 3. We validate full battery, low battery and if it is charging or not */
          if(level == 100){ /* We validate if battery is full */
              batteryStatus.innerHTML = `Full battery <span class="fa fa-battery-full green-color"></span>`
              batteryLiquid.style.height = '103%' /* To hide the ellipse */
          }
          else if (level <= 20 &! batt.charging) { /* We validate if battery is low */
              batteryStatus.innerHTML = `Low battery <span class="fa fa-plug animated-red"></span>`      
          }
          else if (batt.charging){ /* We validate if battery its charging */
              batteryStatus.innerHTML = `Charging... <span class="fa fa-flash animated-green"></span>`
          }
          else { /* If its not loading, dont show anything. */
              batteryStatus.innerHTML = ''
          }
          
          /* 4. We change the colors of the battery and remove the other colors */
          if(level <= 20) {
              batteryLiquid.classList.add('gradient-color-red')
              batteryLiquid.classList.remove('gradient-color-orange','gradient-color-yellow','gradient-color-green')
          }
          else if (level <= 40) {
              batteryLiquid.classList.add('gradient-color-orange')
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-yellow','gradient-color-green')
          }
          else if (level <= 80) {
              batteryLiquid.classList.add('gradient-color-yellow')
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-orange','gradient-color-green')    
          }
          else {
              batteryLiquid.classList.add('gradient-color-green')
              batteryLiquid.classList.remove('gradient-color-red','gradient-color-orange','gradient-color-yellow')
          }
          
      }
      updateBattery()
  }
  t(batt);
}