#import "../lib.typ": *

= Archivos <files>
Las mediciones se guardan en archivos con la extensión `.tgj`.

== Guardar
- #ui[Guardar la pantalla actual] guarda la pestaña seleccionada: la medición en tiempo real o una captura.
- #ui[Guardar todas las capturas] guarda todas las pestañas de captura en un solo archivo.

El diálogo de guardado propone un nombre de archivo a partir del contexto de la medición. Si se ha seleccionado un reloj guardado, usa su nombre como base. Si hay una posición definida, añade el código de posición. Al guardar varias capturas juntas, conviene usar el nombre de la sesión o del reloj y no el de una sola posición. El nombre propuesto puede editarse antes de guardar. La extensión `.tgj` se añade cuando es necesario.

#note[Use nombres cortos y válidos como nombres de archivo si va a mover los archivos entre Windows, Linux y macOS. Cambiar el nombre de una captura cambia el nombre almacenado dentro del archivo `.tgj`; el nombre del archivo puede elegirse de forma independiente en el diálogo de guardado.]

== Abrir
- #ui[Abrir] en el menú;
- haga doble clic en un archivo `.tgj` en los sistemas donde el tipo de archivo esté registrado (el instalador de Windows y `make install` lo hacen);
- indique uno o más archivos en la línea de órdenes: `tg-expert file.tgj`.

Cada captura del archivo se abre en su propia pestaña. Las capturas pueden verse, renombrarse, asignarles una posición y guardarse de nuevo, pero no pueden reanudarse como mediciones en tiempo real.

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
