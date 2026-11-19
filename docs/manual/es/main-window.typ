#import "../lib.typ": *

= La ventana principal <main-window>
#shot("main-window.png", [La ventana principal midiendo un reloj de 28800 bph.])
La ventana tiene una barra superior, una fila de pestañas, la tabla de
resultados, el diagrama de marcha a la izquierda y, a la derecha, los paneles
de forma de onda o la página de tendencia.
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
/ Capturar medición: conserva la medición actual en una pestaña nueva. El
  atajo es Ctrl+Espacio.
/ Captura actual: el nombre de la pestaña de captura seleccionada, que puede
  editarse.
/ Menú: #ui[Abrir], #ui[Guardar la pantalla actual], #ui[Guardar todas las
  capturas], #ui[Algoritmo ligero], #ui[Ajustes de audio…], #ui[Iniciar
  calibración], #ui[Manual de usuario], #ui[Cerrar todas las capturas] y
  #ui[Salir].
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
Las demás líneas, tal como figuran en la leyenda de la parte inferior:
- *amplitud*: la línea rosa, que se lee en la escala de grados de la parte
  superior;
- *guía de marcha*: las líneas azules discontinuas se inclinan según la marcha
  medida, suavizada durante unos 10 segundos. Cuando los puntos corren
  paralelos a ellas, la marcha es constante.
#shot("paper-strip.png", [El diagrama de marcha con sus controles. El tic y el
toc forman dos líneas próximas, y la amplitud baja de unos 300° a 250°.],
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
marca la amplitud medida a partir de la forma de onda.
Una forma de onda limpia, con clics nítidos y separados, indica una buena
señal. Una forma de onda plana o borrosa indica que el micrófono no capta bien
el reloj.
La página #ui[Tendencia] se describe en @trend.
