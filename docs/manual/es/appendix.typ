#import "../lib.typ": *

#set heading(numbering: "A.1")
#counter(heading).update(0)
= Límites
#table(
  columns: 2,
  table.header([Elemento], [Intervalo]),
  [Frecuencia de batido], [de 8100 a 72000 bph],
  [Ángulo de alzamiento], [de 10° a 90°, 52° por defecto],
  [cal], [de −100.0 a +100.0 s/d],
  [Frecuencias de muestreo], [22.05, 32, 44.1 y 48 kHz, si el dispositivo las admite],
  [Registro de tendencia], [una lectura cada 2 s, hasta 100 horas],
  [Calibración], [unos 15 minutos (900 tics de una referencia de cuarzo de 1 Hz)],
)
= Glosario
/ Amplitud: el recorrido del volante desde su posición de reposo, en grados.
/ Ángulo de alzamiento: el ángulo durante el cual el volante está en contacto
  con el escape, en grados.
/ Batido: un tic o un toc. El volante da dos batidos por oscilación.
/ bph: batidos por hora.
/ cal: la corrección del reloj de la tarjeta de sonido, en s/d.
/ Captura: una copia congelada de una medición, conservada en su propia
  pestaña.
/ dBFS: decibelios relativos a la escala completa. 0 dBFS es la señal más
  fuerte que puede grabar la entrada.
/ Diagrama de marcha: la representación de los tiempos de batido, que debe su
  nombre a los diagramas en papel de los cronocomparadores antiguos.
/ Error de batido: la asimetría temporal positiva entre los semiperíodos alternos Tic-a-Toc y Toc-a-Tic, en ms.
/ Error posicional: la mayor diferencia de marcha media entre posiciones.
/ Marcha: el adelanto o atraso del reloj, en s/día.
/ .tgj: el formato de archivo de Tg, tg-timer y tg-expert.
= Créditos y licencia
tg-expert se basa en *Tg*, escrito por Marcello Mamino y los colaboradores de
#link("https://github.com/vacaboja/tg")[vacaboja/tg], y en *tg-timer*, la
continuación de Tg realizada por Alejandro Grigera. tg-expert es mantenido
por jefebus en
#link("https://github.com/jefebus/tg-expert")[github.com/jefebus/tg-expert].
tg-expert es software libre, distribuido bajo la Licencia Pública General de
GNU, versión 2. No ofrece ninguna garantía. La licencia completa está en el
archivo `LICENSE` que se distribuye con el programa.
Este manual se distribuye bajo la misma licencia.
