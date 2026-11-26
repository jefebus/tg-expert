#import "../lib.typ": *

= La ventana principal <main-window>
#shot("main-window.png", [La ventana principal midiendo un reloj de 28800 bph.])
La ventana tiene una barra superior, una fila de pestañas, la tabla de
resultados, el diagrama de marcha a la izquierda y, a la derecha, los paneles
de forma de onda, tendencia o correlaciones.
== La barra superior
De izquierda a derecha:
/ Reloj: una lista de relojes guardados. Al seleccionar uno se establecen sus
  bph y su ángulo de alzamiento. El botón de guardar almacena los bph y el
  ángulo de alzamiento actuales con un nombre, y el botón de eliminar borra
  el reloj seleccionado.
/ BEAT: la frecuencia de batido en bph. Elija un valor predefinido, escriba un
  valor, o elija #ui[auto] para que el programa la encuentre.
/ L.A.: el ángulo de alzamiento en grados, de 10 a 90.
/ Botón de audio: muestra la entrada de audio en uso. Haga clic en él para
  abrir los ajustes de audio.
/ cal: el valor de calibración en s/día, de −100 a +100 s/día. Lo rellena la
  calibración, y también puede escribirse a mano.
/ Posición: la posición del reloj (esfera arriba, esfera abajo, corona
  arriba, corona abajo, corona a la izquierda, corona a la derecha, o sin
  definir).
/ Grabar / Pausar: empieza y termina un tramo, un periodo de grabación
  continua (véase @runs).
/ Capturar medición: conserva la medición actual en una pestaña nueva, sin
  detener la grabación. Solo mientras se graba. El atajo es Ctrl+Espacio.
/ Captura actual: el nombre de la pestaña de captura seleccionada, que puede
  editarse.
La barra de menús, en la parte superior de la ventana, tiene cuatro menús:

/ Archivo: #ui[Nueva sesión], #ui[Abrir sesión…], #ui[Guardar], #ui[Guardar
  como…], #ui[Exportar datos…], #ui[Exportar informe…] (véase @files) y
  #ui[Salir].
/ Ajustes: #ui[Ajustes de audio…] y #ui[Configuración del algoritmo…].
/ Herramientas: #ui[Iniciar calibración] (#ui[Detener calibración] durante la
  calibración) y #ui[Panel de proyecto y notas], que muestra u oculta ese
  panel.
/ Ayuda: #ui[Manual de usuario] y #ui[Acerca de tg-expert], con la versión,
  la licencia y un enlace al código fuente en GitHub.

Atajos de teclado, indicados junto a las entradas del menú: Ctrl+N nueva
sesión, Ctrl+O abrir sesión, Ctrl+S guardar, Ctrl+Mayús+S guardar como,
Ctrl+E exportar datos, Ctrl+P exportar informe, Ctrl+Q salir, F1 manual de usuario y Ctrl+Espacio para
hacer una captura. Un atajo solo funciona cuando su entrada está disponible
(por ejemplo, no durante la calibración).

== Panel de proyecto y notas <notes-panel>
Flota sobre la parte derecha de las pestañas y solo las tapa en parte, para
que la mayor parte de las gráficas siga a la vista mientras escribe. Al
principio está oculto: #ui[Panel de proyecto y notas] en el menú
#ui[Herramientas] lo muestra u oculta, y su botón de cerrar lo oculta.
Arrastre la franja de su borde izquierdo para cambiar su anchura. El programa
recuerda si estaba visible y su anchura. Tiene dos páginas:

/ Proyecto: #ui[Nombre del proyecto], #ui[Referencia del trabajo],
  #ui[Propietario] y #ui[Último servicio] (un año, un mes o un día: `AAAA`,
  `AAAA-MM` o `AAAA-MM-DD`, que se conserva tal como se escribe; una fecha no
  válida se muestra en rojo y no se guarda hasta que sea válida), en el grupo
  plegable #ui[Identificación]; el reloj de la sesión, que es el elegido en la
  lista de relojes guardados y no se edita aquí; y las #ui[Notas generales].
/ Medición: las notas del tramo o de la captura de la pestaña seleccionada,
  con su título (#ui[Notas de: Tramo 2], #ui[Notas de: 2.3 (DU)]). Cada tramo
  y cada captura tiene sus propias notas, vacías al principio. En la pestaña
  de tiempo real, mientras se graba, son las notas del tramo en curso
  (#ui[Notas de: Tramo 3 (en curso)]); pasan con el tramo a su pestaña al
  pulsar #ui[Pausar]. En pausa no se graba ningún tramo, y la página lo indica.

#shot("notes_project.png", [La página Proyecto, con los datos de identificación
y las notas generales.], width: 60%)

#shot("notes_measurement.png", [La página Medición, con las notas de una
captura.], width: 65%)

Lo que escribe cambia la sesión al momento (no hay botón Aplicar) y se guarda
con ella. Las notas son texto sin formato, de hasta 64 KiB cada una.

El proceso de detección y el diálogo #ui[Configuración del algoritmo…] se
describen en @algorithm; la referencia completa de parámetros está en
@algorithm-configuration.

== La tabla de resultados
La tabla situada en la parte superior de cada pestaña muestra:
/ RATE: la marcha en s/d, corregida con el valor cal.
/ AMP.: la amplitud en grados, cuando puede medirse.
/ B.E.: el error de batido en ms.
/ L.A.: el ángulo de alzamiento usado.
/ BEAT: la frecuencia de batido usada, en bph. Un valor marcado con \~ es una
  estimación libre, que se muestra cuando la señal no coincide con los bph
  seleccionados.
A la izquierda de la tabla, un icono de reloj y cuatro cuadrados indican el
estado de la señal. Cada cuadrado es una ventana de análisis con un período
válido. El reloj muestra:
- *0*: no hay entrada de audio;
- *?*: hay audio, pero todavía no se ha reconocido ninguna frecuencia de
  batido;
- *!*: no hay un ritmo válido a la frecuencia de batido seleccionada;
- la esfera del reloj sin símbolo: se detectan audio y un período de batido
  válido.
Sitúe el cursor sobre el icono para ver esta explicación en el programa.
== El diagrama de marcha
El diagrama de marcha dibuja cada batido como un punto. El tiempo avanza hacia
abajo. La posición horizontal de un punto es el instante del batido dentro de
la ventana, de modo que un reloj que marca la hora exacta dibuja líneas
verticales. Un reloj que adelanta dibuja líneas inclinadas hacia un lado, y
uno que atrasa dibuja líneas inclinadas hacia el otro. El tic y el toc se
dibujan en colores distintos. Cuando hay error de batido, forman dos líneas
separadas.
El diagrama de marcha tiene un ancho fijo. En la parte superior hay dos
escalas que comparten las mismas líneas verticales de la cuadrícula: la
amplitud en grados, y el tiempo dentro de la ventana en ms. Las etiquetas de
tiempo de la izquierda son minutos y segundos.
Los puntos que cruzan un margen vertical reaparecen en el margen opuesto.
El intervalo de repetición sigue la escala actual en ms y el zoom. Cerca de
un margen, un punto puede aparecer en ambos lados; Centrar sigue situando el
batido más reciente en el centro.
Las demás líneas, tal como figuran en la leyenda de la parte inferior:
- *amplitud*: la línea rosa, que se lee en la escala de grados de la parte
  superior;
- *guía de marcha*: las líneas azules discontinuas se inclinan según la marcha
  medida, suavizada durante unos 10 segundos. Cuando los puntos corren
  paralelos a ellas, la marcha es constante.
#shot("paper-strip.png", [El diagrama de marcha con sus controles. El tic y el
toc forman dos líneas próximas, junto al historial de amplitud.],
width: 45%)
Los botones situados bajo el diagrama:
/ < y >: desplazan el diagrama una división de la cuadrícula.
/ Borrar: borra el diagrama y lo reinicia (solo en la pestaña en tiempo real).
/ Centrar: sitúa el batido más reciente en el centro del diagrama.
/ −, 1x, +: zoom. Los niveles de zoom son 0.25×, 0.5×, 1×, 2×, 4× y 8×. Con
  1× el diagrama tiene el ancho de un batido. 1x vuelve a ese nivel.
Los ajustes situados bajo los botones:
/ Marca Y (s): el tiempo entre marcas del eje vertical, en segundos. + y −
  lo cambian en 1 s. Pueden escribirse valores más pequeños, como 0.5.
/ Ventana Y (s): el intervalo de tiempo que se muestra en vertical, en
  segundos. Es fijo y no cambia por sí solo.
== Los paneles de forma de onda
La página #ui[Formas de onda] de la derecha muestra el sonido promediado de
los batidos:
/ Tic: el sonido promediado del primer batido.
/ Toc: el sonido promediado del segundo batido.
/ Período completo: el tic y el toc juntos, con los dos resaltados.
/ Debug: la señal procesada que usa el análisis.
#shot("waveform-panels.png", [Los paneles de forma de onda. Cada batido muestra
los clics separados del escape.])
Los paneles de tic y toc tienen una escala en ms en la parte inferior y una
escala de amplitud en grados en la parte superior. La línea vertical azul
marca la amplitud medida a partir de la forma de onda. La vista se amplía
hacia la izquierda cuando es necesario para incluir la amplitud mínima
configurada; las ventanas resaltadas del
panel de período completo se amplían igual. Esto solo cambia la presentación,
no la detección de amplitud.
Una forma de onda limpia, con clics nítidos y separados, indica una buena
señal. Una forma de onda plana o borrosa indica que el micrófono no capta bien
el reloj.
La línea de estado bajo las formas de onda conserva su espacio cuando está
vacía. Pase el cursor sobre un mensaje truncado para ver el texto completo.
Cuando se detecta el batido pero no se puede medir la amplitud, el estado bajo
las formas de onda indica "Amplitud no disponible". La marcha y el error de
batido siguen disponibles; el error de batido no se ajusta con los tiempos de
los impulsos de elevación. La versión de depuración también muestra el motivo
del rechazo de la amplitud.
Si se encuentra un período estable pero no se pueden separar el tic y el toc,
la pantalla cambia automáticamente a "Solo marcha": el error de batido y la
amplitud aparecen como no disponibles, y no se añaden formas de onda, eventos
al registro ni lecturas de tendencia hasta que se detecten de nuevo el tic y
el toc. El historial anterior sigue visible.
Las páginas #ui[Tendencia] y #ui[Correlaciones] se describen en @trend.
