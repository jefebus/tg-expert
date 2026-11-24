#import "../lib.typ": *

= Calibración <calibration>
== Por qué
Un ordenador contiene varios relojes electrónicos internos. 
La placa principal y el procesador utilizan sus propias 
referencias de reloj, pero para tg-expert la más importante 
es la de la tarjeta o interfaz de sonido: es la que determina 
la frecuencia real con la que se toman las muestras de audio 
y, por tanto, la escala de tiempo con la que el programa mide 
los batidos del reloj.

Estos relojes suelen derivarse de cristales de cuarzo económicos. 
Su error puede ser de varias decenas de partes por millón (ppm)
y varía ligeramente con la temperatura. Una ppm significa una 
parte por millón: en una medida de tiempo equivale a 0,0864 
segundos por día. Por ejemplo, un error de 50 ppm equivalen a 4,32 s/d. 
Aunque parezca pequeño, ese error se suma directamente al RATE calculado por tg-expert.

La calibración mide el error efectivo del reloj de la tarjeta de sonido 
frente a una referencia de cuarzo y lo expresa en s/d. A partir de entonces, 
el programa lo resta de las medidas para que el RATE corresponda al reloj 
analizado y no al error de la base de tiempos del ordenador.
== Cómo
Necesita un reloj de cuarzo que haga tic una vez por segundo, por ejemplo un
reloj de cuarzo analógico con segundero. Su precisión pasa a ser la precisión
de la calibración, normalmente mejor que 0.5 s/día.
1. Coloque el reloj de cuarzo muy cerca del micrófono, a ser posible tocándolo.
2. Trabaje en una sala silenciosa. Apague los ventiladores y los altavoces
  cercanos, y no toque la mesa.
3. Elija #ui[Iniciar calibración] en el menú. El lado derecho de la ventana
  muestra las instrucciones de calibración.
4. Observe el estado en la tabla de resultados:
  - *cal (wait)*: todavía no se ha encontrado un tic estable una vez por
    segundo. Revise la instalación.
  - *cal (acq.) NN %*: recopilando tics. No toque nada.
  - *Estimación provisional*: aparece por separado cuando hay suficientes
    observaciones válidas. Se marca con `~`, puede cambiar y no modifica la
    corrección actual.
  - *cal (done) ±X.X s/d*: terminado. El valor se copia en el campo #ui[cal]
    y se guarda.
  - *cal (fail)*: la señal fue demasiado irregular. Mejore la instalación y
    empiece de nuevo.
5. Cuando aparezca *done*, elija #ui[Detener calibración] en el menú para
  volver a la medición normal.

Si detiene la calibración antes de terminar y hay una estimación utilizable,
tg-expert pregunta si quiere aplicarla. La corrección anterior se conserva
salvo que elija explícitamente aplicar la estimación. Al aceptarla, pasa a ser
el valor normal de calibración guardado y deja de ser provisional. Si no hay
suficientes observaciones válidas, detener simplemente conserva la corrección
anterior.
Con los ajustes predeterminados del algoritmo, la calibración dura unos
15 minutos (900 tics); otros ajustes pueden cambiar la duración. Repítala una vez para
confirmar el resultado. Dos mediciones deberían coincidir con una diferencia
de unas décimas de segundo por día.
#shot("calibration.png", [Calibración en curso, al 5 %. El diagrama de marcha
muestra los tics de referencia, y el lado derecho muestra las instrucciones.])
== Cuánto dura
El valor cal pertenece a la tarjeta de sonido y a su reloj, no al reloj de
pulsera. Se guarda y se usa en cada inicio. Vuelva a calibrar:
- cuando cambie la entrada de audio o la frecuencia de muestreo;
- cuando use otro ordenador u otra tarjeta de sonido;
- si la temperatura de la sala es muy distinta, ya que el cristal se desvía un
  poco con la temperatura;
- de vez en cuando, para comprobarlo. Una vez al año es una costumbre
  razonable.
El valor cal también puede escribirse en el campo #ui[cal], si se conoce de una
calibración anterior.

== Controles expertos de calibración <calibration-parameters>

En condiciones normales no necesita cambiar estos valores. Si conoce su
efecto y necesita hacerlo, detenga primero la calibración y abra
#ui[Configuración del algoritmo…]. En la pestaña #ui[Calibración],
#ui[Editar ajustes de calibración (Experto)] habilita los seis controles.
Desmarcarla los bloquea, pero no descarta las modificaciones ni las aplica.
Aplicar y Guardar funcionan como en @algorithm-configuration: reinician el
análisis y el INI conserva solo los valores distintos del predeterminado.
Para restablecer estos controles debe mantener activada su edición.

Estos ajustes pertenecen a la referencia de cuarzo, no al movimiento que
va a medir después. La referencia sigue dando un tic por segundo; el programa
reúne esos sonidos en una onda de un segundo y sigue su desplazamiento
temporal para calcular la corrección. Antes de ajustar la detección,
compruebe la colocación y el silencio siguiendo el procedimiento anterior.

#table(
  columns: (2.8fr, 1.2fr, 0.65fr, 0.95fr),
  [Parámetro], [Clave INI], [Por defecto], [Rango],
  [Número de observaciones], [`cal_count`], [900], [300–3600],
  [Umbral de flanco de calibración (%)], [`cal_edge`], [5], [3–10],
  [Intervalo mínimo entre observaciones (s)], [`cal_interval`], [0.9], [0.5–2],
  [Precisión de aceptación del ajuste (s/día)], [`cal_precision`], [0.1], [0.1–0.5],
  [Posición inferior en el segundo plegado (%)], [`cal_lock_low`], [40], [35–45],
  [Posición superior en el segundo plegado (%)], [`cal_lock_high`], [60], [55–65],
)

/ Número de observaciones:
  Indica cuántas observaciones válidas se reúnen antes de calcular el
  ajuste. No es una duración en segundos: aumentar la cantidad obliga
  normalmente a mantener la referencia estable durante más tiempo.
/ Umbral de flanco de calibración:
  Sirve para localizar el comienzo del impulso de referencia. El umbral
  se sitúa por encima del máximo del fondo circundante, añadiendo este
  porcentaje de la distancia hasta el pico del impulso plegado. No expresa
  un volumen absoluto del micrófono ni una posición dentro del segundo.
/ Intervalo mínimo entre observaciones:
  Evita aceptar observaciones demasiado próximas. Debe transcurrir más
  de este tiempo desde la anterior para admitir otra. No garantiza ese
  ritmo de actualización: los impulsos que no se detectan o un intervalo
  mayor alargan la recogida.
/ Precisión de aceptación del ajuste:
  Es el margen de incertidumbre que se admite para dar por buena la
  calibración. La incertidumbre calculada debe quedar por debajo de él,
  en s/día. Aumentarlo permite aceptar un ajuste menos preciso, pero no
  mejora la referencia ni fija la corrección que se guardará.
/ Posición inferior en el segundo plegado:
  Marca el comienzo de la zona donde se admite el pico de referencia;
  el flanco ascendente elegido debe quedar después de esa posición.
  El 40% habitual corresponde a 400 ms dentro de la onda de un segundo,
  no al tiempo entre observaciones.
/ Posición superior en el segundo plegado:
  Cierra la zona permitida para el pico de referencia. Con el 60%
  predeterminado, el límite son 600 ms dentro de la onda de un segundo.
  Junto al límite inferior delimita dónde se espera el pico; no añade
  otro umbral para detectar el flanco.
