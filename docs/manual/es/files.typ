#import "../lib.typ": *
= Archivos <files>
Las mediciones se guardan en archivos con la extensión `.tgj`.
== Guardar
- #ui[Guardar la pantalla actual] guarda la pestaña seleccionada: la medición
  en tiempo real o una captura.
- #ui[Guardar todas las capturas] guarda todas las pestañas de captura en un
  solo archivo.
== Abrir
- #ui[Abrir] en el menú;
- haga doble clic en un archivo `.tgj`, en los sistemas donde el tipo de
  archivo está registrado (el instalador de Windows y `make install` lo
  hacen);
- indique uno o más archivos en la línea de órdenes: `tg-expert file.tgj`.
Cada captura del archivo se abre en su propia pestaña. Las capturas pueden
verse, renombrarse, asignarles una posición y guardarse de nuevo, pero no
continuarse.
== Qué contiene un archivo
Para cada captura:
- el nombre y la posición del reloj;
- los resultados: marcha, amplitud, error de batido, bph, ángulo de alzamiento
  y cal;
- los batidos mostrados en el diagrama de marcha, con el tic y el toc, y el
  historial de amplitud;
- las formas de onda promediadas;
- el zoom y la posición del diagrama de marcha;
- las estadísticas a largo plazo y las gráficas de la página de tendencia.
El archivo no contiene la grabación de audio.
== Compatibilidad
Los archivos guardados por Tg y tg-timer se abren en tg-expert. Las capturas
guardadas sin estadísticas a largo plazo muestran un mensaje en la página de
tendencia en lugar de las estadísticas. Los archivos guardados por tg-expert
pueden perder los datos de tendencia al abrirse en los programas más antiguos.
