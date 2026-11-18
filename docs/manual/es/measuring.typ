#import "../lib.typ": *
= Medir un reloj <measuring>
== Ajustar los bph y el ángulo de alzamiento
1. Busque la frecuencia de batido y el ángulo de alzamiento del mecanismo. Las
  hojas de datos de los calibres y las obras de referencia de relojería
  indican ambos. Muchos calibres comunes usan un ángulo de alzamiento entre
  50° y 53°. El valor predeterminado de 52° es una estimación razonable cuando
  se desconoce el valor, pero entonces la amplitud es aproximada.
2. Ajuste #ui[BEAT] a la frecuencia de batido, o elija #ui[auto]. La detección
  automática funciona con la mayoría de los relojes, pero con una señal débil
  es posible que acierte mal. Si se conoce la frecuencia de batido, indíquela.
3. Ajuste #ui[L.A.] al ángulo de alzamiento.
Al cambiar los bph, el ángulo de alzamiento o cal se reinician las
estadísticas a largo plazo.
== Relojes guardados
Para un reloj que mide a menudo, ajuste sus bph y su ángulo de alzamiento y
pulse el botón de guardar situado junto a la lista de relojes. Póngale un
nombre. Al seleccionar el reloj más adelante se establecen ambos valores. El
botón de eliminar borra de la lista el reloj seleccionado.
== Posiciones
Un reloj marcha de forma distinta en cada posición, porque la gravedad actúa
de manera diferente sobre el volante. Los cronocomparadores usan estas
posiciones estándar:
#table(
  columns: 2,
  table.header([Código], [Posición]),
  [DU], [Esfera arriba],
  [DD], [Esfera abajo],
  [CU], [Corona arriba],
  [CD], [Corona abajo],
  [CL], [Corona a la izquierda],
  [CR], [Corona a la derecha],
)
Seleccione la posición en la barra superior antes de colocar el reloj en ella.
La posición se registra junto con las lecturas y las capturas. Espere
alrededor de un minuto después de girar el reloj, para que las lecturas se
estabilicen.
== Capturas
#ui[Capturar medición] (Ctrl+Espacio) conserva la medición actual en una
pestaña nueva. La pestaña recibe el nombre del reloj seleccionado y muestra la
posición. Una captura conserva tal como estaban la tabla de resultados, el
diagrama de marcha, las formas de onda y la página de tendencia.
En una pestaña de captura:
- el nombre puede cambiarse en el campo #ui[Captura actual];
- la posición puede cambiarse en la barra superior;
- la pestaña se cierra con la × de su título.
Una sesión típica: mida con la esfera arriba, haga una captura, gire el reloj
con la corona abajo, espere, haga otra captura, y así sucesivamente. Después
guarde todas las capturas en un solo archivo (véase @files).
