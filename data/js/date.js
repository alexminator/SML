document.addEventListener("DOMContentLoaded", function() {
  var actualizarHora = function() {
    var fecha = new Date(),
      hora = fecha.getHours(),
      minutos = fecha.getMinutes(),
      segundos = fecha.getSeconds(),
      diaSemana = fecha.getDay(),
      dia = fecha.getDate(),
      mes = fecha.getMonth(),
      anio = fecha.getFullYear(),
      ampm;

    var pHoras = document.querySelector("#horas"),
      pSegundos = document.querySelector("#segundos"),
      pMinutos = document.querySelector("#minutos"),
      pAMPM = document.querySelector("#ampm"),
      pDiaSemana = document.querySelector("#diaSemana"),
      pDia = document.querySelector("#dia"),
      pMes = document.querySelector("#mes"),
      pAnio = document.querySelector("#anio");

    var semana = [
      "Domingo",
      "Lunes",
      "Martes",
      "Miércoles",
      "Jueves",
      "Viernes",
      "Sábado"
    ];
    var meses = [
      "Enero",
      "Febrero",
      "Marzo",
      "Abril",
      "Mayo",
      "Junio",
      "Julio",
      "Agosto",
      "Septiembre",
      "Octubre",
      "Noviembre",
      "Diciembre"
    ];

    pDiaSemana.textContent = semana[diaSemana];
    pDia.textContent = dia;
    pMes.textContent = meses[mes];
    pAnio.textContent = anio;

    if (hora >= 12) {
      hora = hora - 12;
      ampm = "PM";
    } else {
      ampm = "AM";
    }

    if (hora == 0) {
      hora = 12;
    }

    if (hora < 10) {
      pHoras.textContent = "0" + hora;
    } else {
      pHoras.textContent = hora;
    }

    if (minutos < 10) {
      pMinutos.textContent = "0" + minutos;
    } else {
      pMinutos.textContent = minutos;
    }

    if (segundos < 10) {
      pSegundos.textContent = "0" + segundos;
    } else {
      pSegundos.textContent = segundos;
    }

    pAMPM.textContent = ampm;
  };

  actualizarHora();
  var intervalo = setInterval(actualizarHora, 1000);
});
