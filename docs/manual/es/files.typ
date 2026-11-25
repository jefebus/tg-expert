#import "../lib.typ": *

= Archivos <files>
Las mediciones se guardan en archivos con la extensión `.tgj`.

== Sesiones
Una sesión es un archivo `.tgj` con todo lo medido en un reloj: los datos del proyecto, el reloj, sus tramos y sus capturas (véase @runs). La barra de título muestra el nombre del proyecto y el archivo; `*` indica que hay cambios sin guardar.

- #ui[Nueva sesión] comienza una sesión vacía para el reloj seleccionado en la barra superior.
- #ui[Abrir sesión…] sustituye la sesión actual por la de un archivo.
- #ui[Guardar] escribe la sesión completa en su archivo, sea cual sea la pestaña visible. La primera vez funciona como #ui[Guardar como…].
- #ui[Guardar como…] escribe la sesión en otro archivo, que pasa a ser el de la sesión.

Antes de sustituir la sesión, o de cerrar el programa, con cambios sin guardar, el programa pregunta si desea guardarlos. Son cambios los tramos y capturas nuevos, las capturas renombradas, cerradas o con otra posición y los datos del proyecto; mientras se graba en una sesión que ya tiene tramos, capturas o archivo, también las nuevas lecturas en tiempo real. Observar la pestaña de tiempo real en pausa no lo es.

== Tramos y capturas <runs>
El programa graba por *tramos*. Un tramo es un periodo de grabación continua: empieza con #ui[Grabar] y termina con #ui[Pausar], los dos en el botón situado junto a #ui[Capturar medición]. El título de la pestaña de tiempo real indica el estado: #ui[Tiempo real ● Tramo 2] mientras se graba, #ui[Tiempo real ❚❚ en pausa] cuando no.

- Al iniciar el programa y con #ui[Nueva sesión] empieza a grabarse el tramo 1. Al abrir una sesión, queda en pausa.
- #ui[Pausar] termina el tramo. Pasa a su propia pestaña, #ui[Tramo 1], #ui[Tramo 2]…, con sus páginas Tendencia y Correlaciones y la pestaña de tiempo real tal como estaba al final. Su medición se conserva tal cual: no puede cerrarse ni cambiarse de posición. Puede ponérsele un nombre en el campo #ui[Tramo actual], que se muestra tras su número: #ui[Tramo 2 tras el ajuste].
- #ui[Grabar] empieza un tramo nuevo. Su tendencia empieza desde cero: los tramos son mediciones aparte y nunca se unen, aunque sean seguidos. Tras una pausa de más de 10 minutos, un aviso indica cuánto duró.
- En pausa, la pestaña de tiempo real sigue analizando el sonido, para colocar el reloj y el micrófono, pero no se graba nada ni pueden hacerse capturas.

Una *captura* conserva el momento: lecturas, diagrama de marcha y formas de onda. No detiene la grabación, así que no tiene tendencia propia; la tendencia es de su tramo, que se indica al pasar el ratón por su pestaña. Las capturas guardadas por versiones anteriores con tendencia la conservan.

Las capturas se numeran dentro de su tramo: #ui[2.1], #ui[2.2]… son la primera y la segunda captura del tramo 2, seguidas de la posición si está indicada, como en #ui[2.3 (DU)]. Un nombre escrito en el campo #ui[Captura actual] se muestra tras el número: #ui[2.3 tras el ajuste (DU)]. El número se conserva: cerrar una captura no renumera las demás. Las capturas de archivos de versiones anteriores, que no tienen tramo, conservan su nombre.

Las pestañas van en orden después de #ui[Tiempo real]: primero las capturas del tramo que se está grabando, después el tramo más reciente seguido de sus capturas, luego el tramo anterior, y así sucesivamente. En cada grupo, la captura más reciente va primero. Las pestañas de captura muestran una cámara. Las capturas de archivos de versiones anteriores, que no tienen tramo, van al final.

Cada tramo y cada captura tiene sus propias notas, que se escriben en la página #ui[Medición] del panel de notas (véase @notes-panel). En la pestaña de tiempo real, esa página contiene las notas del tramo que se está grabando: pasan con el tramo a su pestaña al pulsar #ui[Pausar], y al archivo al #ui[Guardar].

== Guardar
#ui[Guardar] escribe los tramos, las capturas y el tramo que se está grabando, hasta ese momento, con la pestaña de tiempo real tal como está entonces. Si se vuelve a guardar, el tramo en curso se escribe de nuevo: no se duplica. Durante la calibración no se guarda la pestaña de tiempo real.

El diálogo de guardado propone `AAAA-MM-DD nombre del proyecto.tgj` (con la fecha de hoy), o el nombre del reloj si no hay nombre de proyecto. Los caracteres que no admiten los nombres de archivo se sustituyen por `_`; el nombre del proyecto guardado en la sesión no cambia. La extensión `.tgj` se añade cuando es necesario.

El archivo se escribe primero en un archivo temporal de la misma carpeta y solo sustituye al anterior cuando se ha escrito por completo. Si falla el guardado, el archivo anterior queda como estaba y la sesión conserva sus cambios sin guardar.

#note[Use nombres cortos y válidos como nombres de archivo si va a mover los archivos entre Windows, Linux y macOS.]

== Abrir
- #ui[Abrir sesión…] en el menú #ui[Archivo];
- haga doble clic en un archivo `.tgj` en los sistemas donde el tipo de archivo esté registrado (el instalador de Windows y `make install` lo hacen);
- indique un archivo en la línea de órdenes: `tg-expert file.tgj`. Una sesión es un único archivo: si se indican varios, solo se abre el primero.

Cada tramo y cada captura se abre en su propia pestaña, y la sesión queda en pausa. Las mediciones del archivo se conservan tal como están: no pueden cambiarse de posición ni cerrarse. Los tramos y las capturas sí pueden renombrarse; el nombre no forma parte de la medición. Un tramo no puede reanudarse, porque los búferes internos no se guardan.

Para seguir midiendo, por ejemplo tras un cierre inesperado del programa, pulse #ui[Grabar]: empieza un tramo nuevo, con el número siguiente, y las capturas nuevas pertenecen a él. El siguiente #ui[Guardar] conserva todo lo del archivo y añade el tramo y las capturas nuevos.

Abrir una sesión no cambia el reloj seleccionado en la barra superior, sus bph ni su ángulo de alzamiento, ni la calibración. Un aviso indica cómo se relaciona el reloj de la sesión con los relojes guardados: #ui[Usar este reloj] lo selecciona para seguir midiendo con sus ajustes. Si el reloj guardado tiene ahora otros ajustes que la sesión, o no existe, el aviso lo indica y no se cambia nada.

== El reloj de una sesión
El reloj de una sesión es el seleccionado en la barra superior. Si se elige otro reloj cuando la sesión ya tiene capturas o archivo, el programa pregunta si se inicia una #ui[Sesión nueva] para él o se #ui[Asocia] esta sesión con él. En ambos casos las capturas conservan sus nombres y sus propios bph y ángulo de alzamiento.

== Qué contiene un archivo
Para cada tramo y captura:
- el nombre de la captura y la posición del reloj;
- la fecha y hora en que se tomó, o en que empezó y terminó el tramo (los archivos de versiones anteriores no las tienen);
- los resultados: marcha, amplitud, error de batido, bph, ángulo de alzamiento y cal;
- los batidos mostrados en el diagrama de marcha, incluida la identificación tic/toc y el historial de amplitud;
- las formas de onda promediadas;
- el zoom y el desplazamiento del diagrama de marcha;
- para un tramo, su número, sus estadísticas a largo plazo y las gráficas de la página Tendencia; para una captura, el tramo en que se tomó y su número dentro de ese tramo;
- sus notas.

El archivo no contiene la grabación de audio.

== Informe (PDF)
#ui[Exportar informe…] en el menú #ui[Archivo] (Ctrl+P) escribe un informe PDF de la sesión, en páginas A4. El diálogo muestra los tramos como las pestañas, el más reciente primero, incluido el que se está grabando (#ui[Tramo 3 (en curso)]). Bajo cada tramo:

- #ui[Tendencias]: lo que muestran las páginas Tendencia y Correlaciones para el tramo: las estadísticas, las gráficas de marcha, amplitud y error de batido y las tres gráficas de correlación, de todo el tramo.
- #ui[Estado final]: la pestaña de tiempo real tal como estaba al terminar el tramo, o al guardar la sesión. Para el tramo que se está grabando es el #ui[Estado al exportar]. Por defecto solo está marcado en los tramos sin capturas.
- sus capturas, con una casilla cada una.

Si se desmarca un tramo, se omite todo lo que hay bajo él. Debajo de los tramos:

- #ui[Estado actual]: la pestaña de tiempo real tal como está al exportar: lecturas, diagrama de marcha y formas de onda. Por defecto está desmarcado, y no está disponible sin una medición válida ni durante la calibración.
- #ui[Otras capturas (archivos antiguos)]: las capturas de archivos guardados antes de que existieran los tramos, si las hay.
- #ui[Tabla de los tramos en la primera página].

Las gráficas van en color o en negro y grises (#ui[Monocromo]), para imprimir. El propietario solo se incluye si se marca #ui[Incluir el propietario].

El informe se organiza así:

- Cada página lleva una cabecera con el logotipo del programa, el título del informe (el nombre del proyecto, o el reloj), la referencia del trabajo y la fecha.
- La primera página tiene un recuadro con el nombre del proyecto, la referencia del trabajo, el reloj, la fecha del último servicio y la fecha del informe, si existen, las notas generales y la tabla de los tramos: para cada tramo, su nombre, periodo, duración, posiciones medidas, marcha media, amplitud media, error de batido medio y número de capturas, y debajo una línea por cada una de sus capturas incluidas en el informe, con su posición, fecha y hora y lecturas.
- Después, cada tramo elegido, en el orden en que se grabó (primero el tramo 1), desde una página nueva: su número y nombre, periodo y duración, sus notas, sus tendencias, su estado final y sus capturas. El estado final y cada captura van en su propia página: primero sus notas, después las lecturas, el diagrama de marcha y las formas de onda, y cuándo se tomó.
- Al final, el estado actual y las otras capturas.

#shot("pdf-export.png", [Ejemplo de la primera página de un informe PDF exportado.],
width: 85%)

Los datos se toman al pulsar #ui[Exportar], así que el informe muestra un único momento aunque la medición continúe. El informe se escribe primero en un archivo temporal y solo sustituye a un archivo existente cuando está completo. Exportar nunca modifica la sesión ni sus notas.

Los bph, el ángulo de alzamiento y el cal que se muestran son aquellos con los que se tomó cada medición. Las formas de onda de una captura son el sonido promediado de su propia ventana de análisis, no de toda la prueba. El informe no es un certificado de conformidad con ninguna norma.

=== Notas en el informe
Las notas se escriben como texto sin formato y se muestran en el informe con esta parte de Markdown:

- una línea en blanco empieza un párrafo nuevo; un salto de línea simple se conserva;
- `#`, `##` y `###` al principio de una línea crean títulos;
- `**negrita**`, `*cursiva*` y `_cursiva_`;
- las líneas que empiezan por `-`, `*` o `+` y un espacio, o por un número y un punto (`1.`), crean listas.

Todo lo demás (enlaces, tablas, código…) se muestra tal como se escribió: no se omite ningún texto.

== Compatibilidad
Los archivos guardados por Tg y tg-timer se abren en tg-expert. Las capturas guardadas sin estadísticas a largo plazo muestran un mensaje en la página Tendencia en lugar de las estadísticas. Si se abre un archivo de tg-expert en un programa antiguo y se vuelve a guardar, pueden perderse los datos de Tendencia.

Un archivo también puede contener los datos del proyecto (nombre del proyecto, referencia del trabajo, etiqueta del reloj, propietario, fecha del último servicio y notas generales) y notas de cada captura. Estos campos son opcionales. Los archivos que no los tienen se abren como antes, y los programas antiguos los ignoran al leer, pero los pierden si vuelven a guardar el archivo. Los programas antiguos muestran un tramo como una captura más, llamada #ui[Tramo 1], #ui[Tramo 2]… en el idioma en que se guardó. En los archivos guardados por tg-expert antes de que existieran los tramos, cada #ui[Estado al guardar] se abre como un tramo. Las capturas de un tramo guardadas antes de que existiera su número se numeran, al abrir el archivo, en el orden en que se tomaron. Los archivos guardados con versiones anteriores se abren como una sesión sin datos del proyecto; el nombre de sus capturas nunca se toma como nombre de un reloj. La etiqueta del reloj se guarda como texto en el archivo: abrir un archivo nunca añade, cambia ni aplica un reloj guardado, ni modifica la calibración.
