#import "../lib.typ": *
= Introducción
tg-expert escucha un reloj mecánico a través de la entrada de audio del
ordenador y mide cómo funciona el mecanismo. Hace el trabajo de un
cronocomparador, pero con un micrófono en lugar de un sensor específico.
== Qué mide un cronocomparador
Un mecanismo mecánico produce dos sonidos por oscilación del volante,
llamados *tic* y *toc*. Cada batido es un grupo de clics breves del escape. A
partir de la temporización de estos batidos, el programa obtiene:
/ Marcha: cuánto adelanta o atrasa el reloj, en segundos por día (s/día). Una
  marcha de +5 s/día significa que el reloj adelanta 5 segundos cada día.
/ Amplitud: cuánto se separa el volante de su posición de reposo, en grados.
  Un mecanismo en buen estado y con la cuerda completa muestra normalmente de
  270° a 310° con la esfera arriba. Una amplitud baja suele indicar un muelle
  real débil, aceite sucio o desgaste.
/ Error de batido: la diferencia de tiempo entre el tic y el toc, en
  milisegundos. Cero significa que el volante está centrado («en batido»).
/ Frecuencia de batido (bph): el número de batidos por hora para el que está
  diseñado el mecanismo, por ejemplo 18000, 21600, 28800 o 36000. La marcha se
  calcula respecto a este valor, por lo que debe ser correcto.
/ Ángulo de alzamiento: el ángulo, en grados, durante el cual el volante está
  en contacto con el escape. Es una propiedad del mecanismo, publicada por el
  fabricante. La amplitud se calcula a partir de él, de modo que un ángulo de
  alzamiento incorrecto da una amplitud incorrecta. La marcha y el error de
  batido no dependen de él.
== Qué puede y qué no puede hacer tg-expert
tg-expert puede:
- medir en tiempo real la marcha, la amplitud y el error de batido, en
  cualquier posición;
- detectar automáticamente la frecuencia de batido de un mecanismo
  desconocido;
- mostrar los batidos en un diagrama de marcha, para que se vean las
  irregularidades;
- registrar la marcha, la amplitud y el error de batido durante un máximo de
  100 horas, con estadísticas por posición del reloj;
- conservar capturas de las mediciones y guardarlas en archivos.
En comparación con un cronocomparador de hardware:
- *El sensor importa.* Un cronocomparador de hardware tiene un sensor
  piezoeléctrico sujeto al reloj. Un micrófono capta además el ruido de la
  sala, por lo que la instalación requiere más cuidado (véase @microphone).
- *El reloj debe calibrarse.* Un cronocomparador de hardware tiene su propio
  oscilador de referencia. tg-expert se basa en el reloj de la tarjeta de
  sonido, que puede desviarse varios segundos por día. La calibración (véase
  @calibration) mide este error y lo corrige.
- *Sin soporte de posicionamiento.* Usted coloca el reloj en cada posición e
  indica al programa en cuál está.
- *La precisión depende de la instalación.* Con un buen micrófono apoyado en
  la caja, una sala silenciosa y una tarjeta de sonido calibrada, las
  lecturas se repiten con una diferencia de una fracción de segundo por día.
  Con una instalación deficiente, los números pueden inducir a error.

== Lecturas de un vistazo
#table(columns: (24%, 76%),
  table.header([Dato], [Interpretación práctica]),
  [RATE (s/d)], [Adelanto o atraso diario; un signo positivo indica adelanto. La calibración corrige esta lectura.],
  [AMP. (°)], [Amplitud del volante. Su cálculo depende del ángulo de alzamiento configurado.],
  [B.E. (ms)], [Error de batido, también denominado error de alternancia.],
  [BEAT (bph)], [Frecuencia de batido o alternancias por hora del movimiento.],
)
#note(color: orange)[*Antes de interpretar los resultados:* calibra la entrada de audio para confiar en RATE y utiliza el ángulo de alzamiento del calibre para interpretar AMP. La calibración no modifica la amplitud ni el error de batido.]
