document.addEventListener('DOMContentLoaded', () => {
  const pHoras = document.querySelector('#horas');
  const pMinutos = document.querySelector('#minutos');
  const pSegundos = document.querySelector('#segundos');
  const pAMPM = document.querySelector('#ampm');
  const pDiaSemana = document.querySelector('#diaSemana');
  const pDia = document.querySelector('#dia');
  const pMes = document.querySelector('#mes');
  const pAnio = document.querySelector('#anio');

  const semana = ['Domingo', 'Lunes', 'Martes', 'Miércoles', 'Jueves', 'Viernes', 'Sábado'];
  const meses = [
    'Enero', 'Febrero', 'Marzo', 'Abril', 'Mayo', 'Junio',
    'Julio', 'Agosto', 'Septiembre', 'Octubre', 'Noviembre', 'Diciembre'
  ];

  const formatNumber = (number) => (number < 10 ? `0${number}` : number);

  const actualizarHora = () => {
    const fecha = new Date();
    let hora = fecha.getHours();
    const minutos = fecha.getMinutes();
    const segundos = fecha.getSeconds();
    const ampm = hora >= 12 ? 'PM' : 'AM';

    // Convertir a formato de 12 horas
    hora = hora % 12 || 12;

    // Actualizar el DOM
    pDiaSemana.textContent = semana[fecha.getDay()];
    pDia.textContent = fecha.getDate();
    pMes.textContent = meses[fecha.getMonth()];
    pAnio.textContent = fecha.getFullYear();

    pHoras.textContent = formatNumber(hora);
    pMinutos.textContent = formatNumber(minutos);
    pSegundos.textContent = formatNumber(segundos);
    pAMPM.textContent = ampm;

    // Llamar a la función nuevamente después de 1 segundo
    setTimeout(actualizarHora, 1000);
  };

  // Iniciar la actualización de la hora
  actualizarHora();
});