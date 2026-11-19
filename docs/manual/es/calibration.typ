#import "../lib.typ": *

= Calibración <calibration>
== Por qué
tg-expert mide el tiempo de los batidos con el reloj de la tarjeta de sonido.
Este reloj es un cristal barato y puede adelantar o atrasar varios segundos
por día. Ese error se suma directamente a la marcha medida. La calibración lo
mide frente a una referencia de cuarzo, y a partir de entonces el programa lo
resta.
La amplitud y el error de batido no dependen de la calibración.
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
La calibración dura unos 15 minutos (900 tics). Repítala una vez para
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
