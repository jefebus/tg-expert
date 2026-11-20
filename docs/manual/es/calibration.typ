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
  - *cal (done) ±X.X s/d*: terminado. El valor se copia en el campo #ui[cal]
    y se guarda.
  - *cal (fail)*: la señal fue demasiado irregular. Mejore la instalación y
    empiece de nuevo.
5. Cuando aparezca *done*, elija #ui[Detener calibración] en el menú para
  volver a la medición normal.
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
