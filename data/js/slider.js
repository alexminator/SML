const slideValue = document.querySelector('span.mark');
const inputSlider = document.querySelector('input');

// Función para actualizar el valor y la posición del indicador
const updateSliderValue = () => {
  const value = inputSlider.value;
  slideValue.textContent = value;
  slideValue.style.left = `${value / 2.55}%`;
  slideValue.classList.add('show');
};

// Función para ocultar el indicador
const hideSliderValue = () => {
  slideValue.classList.remove('show');
};

// Configurar eventos del control deslizante
inputSlider.addEventListener('input', updateSliderValue);
inputSlider.addEventListener('touchend', hideSliderValue);
inputSlider.addEventListener('blur', hideSliderValue);