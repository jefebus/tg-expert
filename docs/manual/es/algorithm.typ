#import "../lib.typ": *

= El algoritmo de medición <algorithm>

TG-Expert utiliza el sonido del escape para medir la marcha y la amplitud
del volante. Compara varios batidos para distinguir tic y toc, cuyos sonidos
deben repetirse con claridad. Un buen contacto con el micrófono suele ayudar
más que cambiar los ajustes del algoritmo.

== Cómo se obtiene la medición

=== La señal que se utiliza

Antes de medir, el programa filtra las vibraciones lentas y forma una
envolvente que destaca los sonidos de cada batido. En modo completo también
descarta tramos de energía inusualmente alta para reducir las interferencias.
Esta supresión no distingue por sí sola entre ruido y un tic muy fuerte:
si desaparecen batidos auténticos, conviene revisar su umbral.

La detección compara la señal con copias desplazadas en el tiempo para
encontrar una repetición regular. Una grabación con más volumen no garantiza
una medición mejor.

=== El ritmo del reloj y la marcha

El período medido corresponde a un ciclo completo, de *tic a tic*:
*7200/BPH segundos*. El intervalo medio entre batidos adyacentes es la mitad,
*3600/BPH segundos*. Por ejemplo, a 14,400 BPH el ciclo dura 500 ms y entre
tic y toc hay, en promedio, 250 ms.

Seleccione los BPH conocidos del movimiento para buscar cerca de su período.
En automático se estima el ritmo y se asocia al BPH predefinido más próximo.
Compruebe resultados inesperados: una estimación estable puede ser equivocada.
Un candidato alternativo en manual no cambia los BPH seleccionados.

La lectura RATE expresa cuánto adelanta o atrasa el reloj respecto a ese
ritmo nominal. También depende de la precisión del reloj de la tarjeta de
sonido; @calibration explica cómo corregir esa base de tiempo.

El modo completo usa bloques de 2, 4, 8 y 16 segundos; el ligero, de
1, 2, 4 y 8. Se eligen resultados por su estabilidad. Tras cambiar ajustes,
espere a que los bloques se llenen: una lectura marcada como antigua no
representa una nueva medición.

=== Las formas de onda y el error de batido

Las formas de onda reúnen muchos ciclos en una sola imagen. Al promediarlos
se descartan los valores más altos de cada posición para reducir el efecto
de golpes aislados. Lo que aparece en pantalla es un sonido representativo,
no la grabación de un único tic.

En esa imagen se localiza el toc cerca de la mitad del ciclo. Si los
intervalos tic–toc y toc–tic no son iguales, aparece error de batido.
Ampliar esta búsqueda puede ayudar con batidos realmente desiguales, pero
no corrige una marcha que adelanta o atrasa de forma constante.

=== La amplitud y los puntos del diagrama

La amplitud se estima con el tiempo de alzamiento acústico y el ángulo
seleccionado, que debe corresponder al movimiento. Ambas estimaciones deben
superar el mínimo, ser inferiores a 360° y no diferir demasiado. Una cifra
plausible no demuestra que se hayan identificado los impulsos correctos.

Cuando se detectan, los tiempos de alzamiento también afinan B.E. y la
posición temporal de los puntos del diagrama. Sin amplitud puede mostrarse
el error de batido preliminar; sin separar tic y toc, la implementación
actual no actualiza RATE. Los puntos se buscan individualmente, por lo que
puede faltar alguno aunque el ritmo esté reconocido, especialmente si aún
no hay suficiente audio alrededor del batido.

== Modos completo y ligero

El modo completo es la opción habitual y predeterminada. El ligero reduce
la carga de cálculo y ofrece lecturas antes, pero suele ser menos estable
y más sensible al ruido: procesa la mitad de las muestras y omite la
supresión de interferencias.

En ligero se deshabilitan los tres controles de supresión de ruido, sin
borrar sus valores ni su ayuda. Volver al modo completo los habilita de nuevo.
El resto de la detección sigue funcionando en ambos modos; los BPH y el
ángulo de alzamiento no cambian.

== Parámetros de configuración <algorithm-configuration>

#note(color: orange)[*Para usuarios con experiencia:* modifique los parámetros
del algoritmo solo si entiende qué controla cada uno y cómo afecta a la
detección. Si no conoce su significado, mantenga los valores predeterminados.
Cambiarlos por ensayo y error puede producir lecturas incorrectas que parezcan
plausibles.]

=== Diálogo, niveles y aplicación de cambios

El menú #ui[Configuración del algoritmo…] reúne estos ajustes. En
#ui[Básico] puede elegir el modo ligero; en #ui[Avanzado] encontrará los
cinco controles de uso más habitual. Si necesita afinar una etapa concreta,
#ui[Mostrar ajustes de experto] añade otros trece bajo los mismos encabezados.
Cambiar lo que se muestra no modifica ni aplica los valores.

Puede plegar cada grupo y abrir la #ui[Ayuda] de cualquier parámetro con
ratón o teclado. Los valores que se apartan del predeterminado aparecen
resaltados para facilitar su revisión. Los BPH, el ángulo de alzamiento y
la frecuencia de muestreo siguen en sus controles habituales de movimiento
y audio. Los ajustes de #ui[Calibración] tienen su propia pestaña y se
explican en @calibration-parameters.

- #ui[Aplicar] aplica y guarda los cambios, manteniendo abierto el diálogo.
- #ui[Guardar] también aplica y guarda, y después cierra el diálogo.
- #ui[Restablecer valores predeterminados] reinicia los controles de movimiento
  y el modo ligero en el diálogo. Solo reinicia calibración cuando su edición
  está activada. El reinicio no se aplica hasta pulsar Aplicar o Guardar.
- #ui[Cancelar] descarta cambios no aplicados, pero no deshace un Aplicar anterior.

Guarde una captura antes de aplicar si quiere comparar con la medición
actual: los cambios reinician el análisis y borran la tendencia en tiempo
real. Así se reconstruyen los filtros y los búferes con los nuevos valores.
Si no se puede guardar la configuración, se muestra un error y no se aplica.

=== Unidades, rangos y valores guardados

Las tablas sirven de referencia para volver al valor original y conocer
el margen de ajuste. Fíjese especialmente en *± ms*: 20 significa 20 ms a
cada lado, no una búsqueda de 20 ms en total. Sin ±, los milisegundos indican
la duración de una ventana o segmento. *×* es un multiplicador; el significado
de cada porcentaje depende de la magnitud que se compara. La amplitud se
expresa en grados. En calibración también aparecen segundos y s/día.
Todos los valores numéricos utilizan punto decimal.

Un valor dentro del rango no siempre basta: al convertirlo en muestras,
las ventanas deben caber en el audio disponible. El programa comprueba
que quede al menos un segmento de referencia de ruido, que haya espacio para
el suavizado y que las posiciones de calibración estén ordenadas dentro
de la zona central del segundo plegado.

El INI guarda las modificaciones en `[algorithm]`, con las claves de las
tablas. Al volver al valor predeterminado se elimina esa entrada, de modo
que solo quedan las personalizaciones. Se conservan los ajustes antiguos
válidos y las claves desconocidas. Si un valor antiguo no es válido, el
programa avisa y mide con el predeterminado. Guardar ajustes de movimiento
no sobrescribe los de calibración que estén bloqueados y no se hayan editado.

=== Controles avanzados de movimiento

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parámetro], [Clave INI], [Por defecto], [Rango],
  [Frecuencia de corte del audio y la envolvente (Hz)], [`filter`], [3000], [500–5000],
  [Umbral de rechazo de ruido (× referencia)], [`noise_threshold`], [2], [1–3],
  [Semiancho de búsqueda del período (± ms)], [`guess_search`], [20], [10–30],
  [Semiancho de búsqueda del batido opuesto (± ms)], [`be_search`], [20], [10–30],
  [Amplitud mínima aceptada del volante (°)], [`amp_min`], [135], [90–135],
)

/ Frecuencia de corte del audio y la envolvente:
  Si la grabación recoge vibraciones lentas, una frecuencia mayor puede
  ayudar a rechazarlas y conservar un detalle más fino en la envolvente.
  El mismo valor se usa para el paso alto del audio y el paso bajo de la
  envolvente; no es un control de volumen. Use Aplicar para reconstruir
  los filtros con el nuevo corte.
/ Umbral de rechazo de ruido:
  En modo completo se silencian los tramos cuya energía supera este
  múltiplo de la referencia. Si desaparecen tics auténticos e intensos,
  pruebe a subirlo: *un valor mayor silencia menos*. Bajarlo refuerza el
  rechazo. No tiene una opción de desactivación independiente y no actúa
  en modo ligero.
/ Semiancho de búsqueda del período:
  Da margen para encontrar un reloj cuyo período se aparta del nominal.
  En manual, el centro es 7200/BPH segundos: a 14,400 BPH y ±20 ms se
  busca entre 480 y 520 ms, no entre 230 y 270. Amplíelo si el ciclo real
  queda fuera de esa ventana. También se usa al comprobar candidatos
  en automático.
/ Semiancho de búsqueda del batido opuesto:
  El toc se busca cerca de la mitad del ciclo medido. Darle más margen
  puede ayudar si tic y toc están realmente descompensados, pero también
  permite escoger un pico ajeno y falsear el error de batido. Una marcha
  que adelanta o atrasa de forma constante no es motivo para ampliarlo.
/ Amplitud mínima aceptada del volante:
  Puede bajarlo para medir un movimiento de amplitud realmente baja.
  Tanto la estimación del tic como la del toc deben superar este valor;
  reducirlo no ayuda a encontrar mejor los impulsos. Las vistas tic/toc
  se amplían a la izquierda cuando lo requiere el mínimo elegido, y los
  resaltados del período completo las acompañan. Esa ampliación solo
  cambia la presentación, no confirma una detección correcta.

=== Controles expertos de movimiento

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parámetro], [Clave INI], [Por defecto], [Rango],
  [Ventana de energía del ruido (ms)], [`noise_window`], [20], [10–30],
  [Longitud del segmento de referencia del ruido (ms)], [`noise_segment`], [500], [300–700],
  [Semiancho de ajuste del período (± ms)], [`period_refine`], [20], [10–30],
  [Pico mínimo de período más corto (%)], [`harmonic`], [90], [80–100],
  [Umbral de comprobación de doble detección (%)], [`double_trigger`], [20], [10–30],
  [Suavizado de alineación tic/toc (ms)], [`tic_smoothing`], [0.5], [0.1–1],
  [Muestras altas descartadas por intervalo de fase (%)], [`trim`], [20], [10–30],
  [Semiancho de búsqueda de eventos de la traza (± ms)], [`event_search`], [20], [10–30],
  [Suavizado de impulsos de elevación (ms)], [`amp_smoothing`], [1], [0.5–2],
  [Umbral inicial del impulso de elevación (% del pico)], [`amp_initial`], [1], [0.1–5],
  [Multiplicador del fondo del impulso de elevación (×)], [`amp_noise`], [1.4], [1.2–2],
  [Límite del umbral del impulso de elevación (% del pico)], [`amp_limit`], [20], [10–30],
  [Diferencia máxima de amplitud tic/toc (°)], [`amp_difference`], [60], [60–100],
)

/ Ventana de energía del ruido:
  Indica cuánto audio entra en cada medida de energía local. Una ventana
  más larga hace que un impacto breve influya durante más tiempo. Úsela
  junto al umbral de ruido para entender qué tramos se silencian; solo
  interviene en modo completo.
/ Longitud del segmento de referencia del ruido:
  La referencia no es el volumen medio del micrófono: se toma la mayor
  energía local de cada segmento y después la mediana de esos máximos.
  Al alargar los segmentos habrá menos medidas de referencia en cada
  bloque. Como los otros controles de ruido, no actúa en modo ligero.
/ Semiancho de ajuste del período:
  Una vez encontrado el período, el programa lo afina buscando los siguientes
  picos de correlación cerca de sus posiciones previstas. Este margen
  pertenece a esa segunda búsqueda. Ampliarlo tiene sentido si el pico
  correcto queda fuera; si no, puede hacer que se elija un vecino.
/ Pico mínimo de período más corto:
  Hace más o menos exigente la elección de un período automático más corto.
  Su pico de correlación debe superar este porcentaje del pico original.
  Al subirlo pasan menos candidatos cortos. Se comparan picos de correlación,
  no el volumen de los sonidos captados por el micrófono.
/ Umbral de comprobación de doble detección:
  Ayuda a decidir si conviene probar un período doble. Se compara la
  correlación cerca de 1.5 veces el período candidato con la de ese período;
  si la relación queda por debajo del umbral, se comprueba el doble.
  Subirlo hace esa prueba más probable, no establece otro umbral de volumen.
  Este control y el anterior también intervienen en las estimaciones
  alternativas de modo manual, pero no en su búsqueda principal alrededor
  de los BPH seleccionados.
/ Suavizado de alineación tic/toc:
  Antes de fijar la referencia temporal se suaviza la envolvente combinada
  de tic y toc. Un suavizado mayor reduce detalles finos, pero puede desplazar
  el punto elegido para alinearlos. No conviene aumentarlo solo para que
  la imagen parezca más limpia.
/ Muestras altas descartadas por intervalo de fase:
  Al formar la onda representativa, se eliminan las muestras más altas de
  cada posición antes de promediar los ciclos. Subir el porcentaje reduce
  golpes aislados, aunque también resta peso a batidos excepcionalmente
  intensos. La cantidad descartada se redondea hacia arriba; incluso con
  pocas muestras se conserva al menos una.
/ Semiancho de búsqueda de eventos de la traza:
  Si faltan puntos, un margen mayor alrededor de cada batido previsto puede
  recuperarlos. También puede elegir otro rasgo acústico del mismo tic.
  Este ajuste afecta a los puntos individuales del diagrama, no a la
  búsqueda inicial del período.
/ Suavizado de impulsos de elevación:
  El detector busca los impulsos en una forma de onda suavizada. Aumentar
  el suavizado puede quitar ondulaciones, pero, si dos rasgos están muy
  próximos, también puede unirlos y dificultar su identificación.
/ Umbral inicial del impulso de elevación:
  Es el punto de partida de la búsqueda, expresado como porcentaje del
  pico más fuerte de la onda suavizada. No siempre será el umbral usado:
  si el calculado a partir del fondo es mayor, la búsqueda empieza con este último.
/ Multiplicador del fondo del impulso de elevación:
  El fondo estimado se multiplica por este valor para obtener un umbral
  de detección. Subirlo hace más difícil aceptar rasgos débiles como
  impulsos de elevación; no aumenta la señal captada.
/ Límite del umbral del impulso de elevación:
  La búsqueda prueba umbrales crecientes hasta llegar a este porcentaje
  del pico. Un límite mayor permite más pruebas a umbrales altos, no una
  amplitud máxima mayor. Si el umbral de partida ya alcanza el límite,
  no se hacen pruebas con un umbral más bajo.
/ Diferencia máxima de amplitud tic/toc:
  Para aceptar el par, la diferencia entre las dos amplitudes debe ser
  menor que este límite. Subirlo permite que discrepen más; no las hace
  más precisas. Aunque superen esta comprobación, aún pueden proceder
  de impulsos mal identificados.

== Pruebe un cambio cada vez

1. Parta de los valores predeterminados, BPH y ángulo correctos, y audio limpio.
  Mejore el contacto del micrófono antes de relajar la detección. Guarde una captura.
2. Identifique la etapa que falla y toque un solo control pertinente. Aplique,
  espere a que se llenen los bloques y compare lecturas estables, no solo
  el primer número que parezca bueno.
3. Conserve el cambio si ayuda; si no, restablezca y aplique de nuevo.
  Para reiniciar calibración debe activar su casilla de edición.

Para medir solo marcha, deje los controles de impulsos como están. Si la
amplitud es realmente baja, pruebe primero el mínimo avanzado. Obtener
un resultado aceptado no demuestra que sea preciso.
