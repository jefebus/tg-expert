#import "../lib.typ": *

= Archivos <files>
Las mediciones se guardan en archivos con la extensión `.tgj`.

== Sesiones
Una sesión es un archivo `.tgj` con todo lo medido en un reloj: los datos del proyecto, el reloj, todas las capturas y el estado de la pestaña de tiempo real al guardar. La barra de título muestra el nombre del proyecto y el archivo; `*` indica que hay cambios sin guardar.

- #ui[Nueva sesión] comienza una sesión vacía para el reloj seleccionado en la barra superior.
- #ui[Abrir sesión…] sustituye la sesión actual por la de un archivo.
- #ui[Guardar] escribe la sesión completa en su archivo, sea cual sea la pestaña visible. La primera vez funciona como #ui[Guardar como…].
- #ui[Guardar como…] escribe la sesión en otro archivo, que pasa a ser el de la sesión.

Antes de sustituir la sesión, o de cerrar el programa, con cambios sin guardar, el programa pregunta si desea guardarlos. Son cambios las capturas nuevas, renombradas, cerradas o con otra posición y los datos del proyecto; cuando la sesión ya tiene capturas o archivo, también las nuevas lecturas en tiempo real. Observar solo la pestaña de tiempo real no lo es.

== Guardar
#ui[Guardar] escribe las capturas y, como #ui[Estado al guardar], lo que muestra en ese momento la pestaña de tiempo real, con sus estadísticas a largo plazo. Si se vuelve a guardar en la misma ejecución, ese estado se sustituye por el actual. Durante la calibración no se guarda la pestaña de tiempo real.

El diálogo de guardado propone `AAAA-MM-DD nombre del proyecto.tgj` (con la fecha de hoy), o el nombre del reloj si no hay nombre de proyecto. Los caracteres que no admiten los nombres de archivo se sustituyen por `_`; el nombre del proyecto guardado en la sesión no cambia. La extensión `.tgj` se añade cuando es necesario.

El archivo se escribe primero en un archivo temporal de la misma carpeta y solo sustituye al anterior cuando se ha escrito por completo. Si falla el guardado, el archivo anterior queda como estaba y la sesión conserva sus cambios sin guardar.

#note[Use nombres cortos y válidos como nombres de archivo si va a mover los archivos entre Windows, Linux y macOS.]

== Abrir
- #ui[Abrir sesión…] en el menú #ui[Archivo];
- haga doble clic en un archivo `.tgj` en los sistemas donde el tipo de archivo esté registrado (el instalador de Windows y `make install` lo hacen);
- indique un archivo en la línea de órdenes: `tg-expert file.tgj`. Una sesión es un único archivo: si se indican varios, solo se abre el primero.

Cada captura se abre en su propia pestaña, y el estado guardado en una pestaña llamada #ui[Estado al guardar]. Las mediciones del archivo se conservan tal como están: no pueden cambiarse de posición ni cerrarse. Las capturas sí pueden renombrarse; el nombre no forma parte de la medición. Tampoco pueden reanudarse como mediciones en tiempo real, porque los búferes internos no se guardan.

La sesión puede continuar después de abrirla, por ejemplo tras un cierre inesperado del programa. La pestaña de tiempo real mide como siempre, sus estadísticas a largo plazo empiezan desde cero y las capturas nuevas se añaden a la sesión. El siguiente #ui[Guardar] conserva todo lo del archivo, incluido su #ui[Estado al guardar], y añade las capturas nuevas y un nuevo estado guardado. Así, una sesión puede tener varios estados guardados, uno por cada tramo de la grabación; cada uno conserva sus propias estadísticas y ajustes, y no se unen.

Abrir una sesión no cambia el reloj seleccionado en la barra superior, sus bph ni su ángulo de alzamiento, ni la calibración. Un aviso indica cómo se relaciona el reloj de la sesión con los relojes guardados: #ui[Usar este reloj] lo selecciona para seguir midiendo con sus ajustes. Si el reloj guardado tiene ahora otros ajustes que la sesión, o no existe, el aviso lo indica y no se cambia nada.

== El reloj de una sesión
El reloj de una sesión es el seleccionado en la barra superior. Si se elige otro reloj cuando la sesión ya tiene capturas o archivo, el programa pregunta si se inicia una #ui[Sesión nueva] para él o se #ui[Asocia] esta sesión con él. En ambos casos las capturas conservan sus nombres y sus propios bph y ángulo de alzamiento.

== Qué contiene un archivo
Para cada captura:
- el nombre de la captura y la posición del reloj;
- los resultados: marcha, amplitud, error de batido, bph, ángulo de alzamiento y cal;
- los batidos mostrados en el diagrama de marcha, incluida la identificación tic/toc y el historial de amplitud;
- las formas de onda promediadas;
- el zoom y el desplazamiento del diagrama de marcha;
- las estadísticas a largo plazo y las gráficas de la página Tendencia.

El archivo no contiene la grabación de audio.

== Compatibilidad
Los archivos guardados por Tg y tg-timer se abren en tg-expert. Las capturas guardadas sin estadísticas a largo plazo muestran un mensaje en la página Tendencia en lugar de las estadísticas. Si se abre un archivo de tg-expert en un programa antiguo y se vuelve a guardar, pueden perderse los datos de Tendencia.

Un archivo también puede contener los datos del proyecto (nombre del proyecto, referencia del trabajo, etiqueta del reloj, propietario, fecha del último servicio y notas generales) y notas de cada captura. Estos campos son opcionales. Los archivos que no los tienen se abren como antes, y los programas antiguos los ignoran al leer, pero los pierden si vuelven a guardar el archivo. Los programas antiguos muestran un estado guardado como una captura más, llamada #ui[Estado al guardar] en el idioma en que se guardó. Los archivos guardados con versiones anteriores se abren como una sesión sin datos del proyecto; el nombre de sus capturas nunca se toma como nombre de un reloj. La etiqueta del reloj se guarda como texto en el archivo: abrir un archivo nunca añade, cambia ni aplica un reloj guardado, ni modifica la calibración.
