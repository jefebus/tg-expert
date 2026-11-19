#import "../lib.typ": *

= Interpretación del sonido del escape suizo de áncora
Este capítulo explica qué representa la forma de onda de tg-expert, cómo se relaciona con el escape suizo de áncora y qué referencias acústicas usa el programa para obtener la marcha, el error de batido y la amplitud.

Un tic o un toc no es un único golpe. Cada batido contiene varias acciones mecánicas muy próximas. En el sonido suelen distinguirse tres pulsos principales. tg-expert procesa el conjunto y extrae referencias temporales repetibles.

== Funcionamiento del escape y formación del tic-toc
El escape suizo conecta el tren de rodaje, que entrega energía desde el muelle real, con el órgano regulador formado por el volante y la espiral. En cada batido, la elipse mueve el áncora, la rueda de escape se desbloquea, se transmite el impulso y la rueda vuelve a quedar bloqueada. En el batido siguiente la secuencia se repite sobre la paleta opuesta.

=== Las cinco fases mecánicas
#table(
  columns: (25mm, 50mm, 50mm, 50mm),
  table.header([*Fase*], [*Qué ocurre*], [*Manifestación acústica probable*], [*Diagrama*]),
  [*1. Desbloqueo*], [La elipse entra en contacto con la horquilla. El áncora comienza a girar y el diente abandona la cara de bloqueo de la piedra de paleta.], [Primer sonido, normalmente preciso pero más débil que el final.], [#shot("fase_1.png", [], width: 50%)],
  [*2. Impulso*], [El diente recorre la cara de impulso. La energía pasa de la rueda de escape a la paleta, al áncora, a la elipse y finalmente al volante.], [Pulso distribuido e irregular que puede mezclarse con el desbloqueo.], [#shot("fase_2.png", [], width: 50%)],
  [*3. Caída*], [Al terminar el impulso, la rueda de escape gira libremente un pequeño ángulo hasta que el siguiente diente alcanza la paleta opuesta.], [El movimiento libre produce poco sonido; el sonido asociado a la caída aparece principalmente al final.], [#shot("fase_3.png", [], width: 50%)],
  [*4. Bloqueo*], [El siguiente diente golpea la cara de bloqueo de la paleta opuesta y detiene la rueda de escape.], [Impacto fuerte y abrupto.], [#shot("fase_4.png", [], width: 50%)],
  [*5. Recorrido hasta el tope*], [Después del primer contacto de bloqueo, el diente se asienta y el áncora recorre todavía un pequeño ángulo hasta su tope.], [Ocurre muy cerca del bloqueo; ambos suelen formar un mismo paquete acústico fuerte.], [#shot("fase_5.png", [], width: 50%)],
)

El orden es *desbloqueo -> impulso -> caída -> contacto de bloqueo -> recorrido hasta el tope*. El bloqueo comienza antes de que el áncora alcance el tope, pero ambos fenómenos están tan próximos que sus vibraciones se solapan en una grabación normal.

=== Los tres pulsos acústicos
+ *Desbloqueo.* La elipse entra en la horquilla e inicia el movimiento del áncora. Es una referencia temporal precisa, aunque puede ser relativamente débil.
+ *Impulso.* El diente recorre la cara de impulso y el áncora transmite energía al volante. Es menos regular y no siempre aparece como un pico aislado.
+ *Bloqueo y tope.* La caída termina cuando el siguiente diente alcanza la cara de bloqueo; inmediatamente después, el áncora completa su recorrido hasta el tope. Este tercer paquete suele ser el más fuerte.

#shot("waveform_3_pulses.png", [Representación conceptual de la envolvente de un tic o un toc.], width: 50%)
La onda real contiene oscilaciones bipolares, rebotes y colas de resonancia. La altura de cada pico depende del contacto mecánico, pero también de la caja, el punto de apoyo, la posición del reloj, la presión del micrófono y la respuesta del sistema de captura.
#note[No interprete cada pico como una pieza concreta. La forma de onda es la respuesta conjunta del escape, el movimiento, la caja, el soporte y el micrófono.]

=== Diferencias entre tic y toc
Tic y toc corresponden a acciones alternas sobre las paletas de entrada y salida. Sus tiempos deberían ser simétricos alrededor de la posición de equilibrio del volante, pero su forma y nivel acústico pueden diferir. Una diferencia de altura entre tic y toc no implica por sí sola un defecto; son más importantes la estabilidad temporal y la repetibilidad.

=== Parámetros obtenidos del sonido
#shot("parameters_from_sound.png", [Cómo se obtienen los parámetros a partir de la forma de onda.], width: 100%)

== Cómo procesa el sonido tg-expert
El algoritmo procede de Tg. No reconoce piezas ni etiqueta las fases mecánicas. Convierte el audio en una envolvente, busca la repetición periódica del patrón y mide referencias temporales.

=== Cadena de procesamiento
#table(columns: (12mm, 45mm, 99mm),
  table.header([*N.º*], [*Operación*], [*Finalidad*]),
  [1], [Filtro paso alto], [Reduce deriva, vibraciones lentas y ruido de manipulación.],
  [2], [Supresión de ruido], [Descarta intervalos mantenidos con energía anormalmente alta.],
  [3], [Rectificación], [`fabs()` convierte la señal bipolar en una magnitud positiva.],
  [4], [Filtro paso bajo], [Extrae la envolvente temporal del paquete acústico.],
  [5], [Autocorrelación mediante FFT], [Busca periodicidad sin necesitar una forma de tic fija.],
  [6], [Plegado de ciclos], [Alinea múltiples tic y toc para crear una forma de onda representativa.],
  [7], [Media recortada], [Descarta el 20 % superior para reducir golpes accidentales.],
  [8], [Separación Tic-Toc], [Busca el batido opuesto cerca de la mitad del período.],
  [9], [Tiempo de alzamiento], [Localiza el inicio detectable del paquete acústico antes de la referencia final.],
  [10], [Correlación con plantilla], [Localiza los batidos individuales usados en el diagrama de marcha.],
)

=== Acondicionamiento y envolvente
El filtro paso alto conserva los impactos rápidos y reduce movimientos lentos. La rectificación elimina el signo de la vibración y el filtro paso bajo produce una envolvente. La detección posterior trabaja principalmente con esta envolvente, no con cada oscilación rápida del audio original.

=== Marcha y período
La autocorrelación busca cuánto tarda en repetirse un batido equivalente. A 28800 bph hay ocho batidos por segundo: un Tic y el Toc siguiente están separados aproximadamente 0.125 s, mientras que dos Tic consecutivos están separados aproximadamente 0.25 s. Este período Tic-a-Tic es la referencia principal para la marcha.

#note[Una buena medida de marcha no valida automáticamente la amplitud. La periodicidad puede ser estable aunque el inicio débil del desbloqueo no se detecte bien.]

=== Error de batido
El error de batido describe la asimetría temporal entre los dos semiperíodos alternos: del Tic al Toc siguiente y del Toc al Tic siguiente. tg-expert compara ambos intervalos y muestra B.E. como un valor positivo en milisegundos.

=== Cálculo de amplitud
La amplitud no se deduce de la altura del pico. tg-expert estima el tiempo de alzamiento a partir de la onda y lo combina con el ángulo de alzamiento configurado. El modelo supone un movimiento aproximadamente sinusoidal del volante: para el mismo ángulo de alzamiento, un tiempo de alzamiento más corto corresponde a una amplitud mayor.

El algoritmo suaviza la forma de onda con una ventana de aproximadamente 1 ms y calcula un umbral a partir del máximo global y del nivel de fondo. Para cada Tic y Toc busca hacia atrás desde la referencia principal hasta encontrar el inicio detectable del paquete, normalmente próximo al desbloqueo. Ese inicio se combina con la referencia fuerte final de bloqueo/tope para obtener el tiempo acústico de alzamiento mostrado en la forma de onda.

#note[Si el desbloqueo queda bajo el umbral, tg-expert comienza demasiado tarde. El tiempo de alzamiento se acorta y la amplitud puede resultar artificialmente alta.]

El ángulo de alzamiento también es crítico. Un valor incorrecto produce una amplitud incorrecta, aunque no afecta a RATE ni B.E.

=== Qué distingue el algoritmo
#table(columns: (70mm, 86mm),
  table.header([*Elemento*], [*Tratamiento en tg-expert*]),
  [Tic frente a Toc], [Sí. Ambos sentidos se localizan y se mantienen separados.],
  [Tres pulsos acústicos], [No se clasifican individualmente; forman una envolvente compuesta.],
  [Altura del pico], [Influye en la detección, pero no determina la amplitud.],
  [Tiempo de alzamiento], [Sí. Se estima a partir de las referencias acústicas detectables.],
  [Amplitud], [Se infiere del tiempo de alzamiento, el período y el ángulo de alzamiento configurado.],
)

== Evaluación de la calidad de la medición
#table(columns: (35mm, 61mm, 60mm),
  table.header([*Indicador*], [*Buena medición*], [*Medición dudosa*]),
  [Fondo], [Bajo y estable entre batidos.], [Ruido continuo, golpes o nivel variable.],
  [Desbloqueo], [Inicio visible, limpio y repetible.], [Inicio enterrado, intermitente o variable.],
  [Pulso final], [Fuerte pero sin recorte.], [Techo plano, saturación o resonancia excesiva.],
  [Tic y Toc], [Ambos se detectan de forma estable.], [Uno desaparece o cambia continuamente.],
  [Traza], [Línea definida, sin eventos duplicados.], [Nubes, saltos o doble disparo.],
  [Marcha], [Estable durante varias ventanas.], [Cambios bruscos sin cambio físico del reloj.],
  [Amplitud], [Plausible y coherente.], [Saltos grandes mientras la marcha permanece estable.],
)

== Límites de interpretación
La forma de onda permite evaluar periodicidad, estabilidad y calidad de detección. Puede relacionarse de forma general el inicio con el desbloqueo, la zona intermedia con el impulso y el paquete final con el bloqueo y el tope. No permite diagnosticar una pieza concreta sólo por una resonancia ni sustituye la inspección directa del escape cuando aparecen anomalías persistentes.

== Referencias
1. Witschi Electronic AG. _Testing Methods for Mechanical Watches_. Büren an der Aare, 2025. Secciones “Acoustic Measurement”, “The Beat Noise of the Swiss Lever Escapement”, “Rate Measurement”, “Beat Error” y “Amplitude”.
2. Israel, Robin. “The Swiss lever escapement - Part 1: For everyone”. _R.I.O. Uhrenwerkstätte_, 28 de octubre de 2026.
3. Velásquez Enriquez, Guido Alfonso. “The Beat Noise Of The Swiss Lever Escapement”. Vídeo, 21 de octubre de 2013.
