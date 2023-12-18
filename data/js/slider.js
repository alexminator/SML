const slideValue = document.querySelector("span.mark");
const inputSlider = document.querySelector("input");
inputSlider.oninput = (()=>{
  let value = inputSlider.value;
  slideValue.textContent = value;
  slideValue.style.left = (value/2.55) + "%";
  slideValue.classList.add("show");
});

inputSlider.ontouchend = (()=>{
  slideValue.classList.remove("show");
});

inputSlider.onblur = (()=>{
  slideValue.classList.remove("show");
});