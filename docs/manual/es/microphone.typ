#import "../lib.typ": *

= El micrófono <microphone>
La mayoría de las malas lecturas se deben a la instalación de audio y no al
programa: un micrófono débil o ruidoso, un micrófono demasiado lejos del
reloj, o el ruido de la sala.
== Qué micrófono elegir
Mejores opciones:
- un micrófono de contacto o un captador piezoeléctrico;
- el micrófono de unos auriculares o de unos cascos, apoyado en la caja;
- un buen micrófono USB colocado contra el reloj.
Normalmente peores:
- el micrófono integrado de un portátil;
- un micrófono de sobremesa situado a distancia;
- micrófonos con cancelación de ruido o ganancia automática activadas.
El micrófono sencillo de unos auriculares apoyado en la caja suele funcionar
mejor que un micrófono caro situado a unos centímetros.
== Colocación
- Ponga el micrófono en contacto con la caja, la tapa trasera o el portamovimiento.
- Mantenga quietos el reloj y el micrófono. Un pequeño soporte o una pinza
  ayudan.
- Si la señal es débil, pruebe otros puntos de la caja. Pequeños cambios de
  posición pueden suponer una gran diferencia.
- Al medir posiciones (véase @measuring), compruebe de nuevo la señal después
  de girar el reloj.
#shot("microphone-setup.jpg", [El micrófono integrado en el cable de unos
auriculares, apoyado directamente sobre el cristal del reloj.], width: 60%)
== Ganancia
Suba la ganancia hasta que los batidos se vean claramente en los paneles de
forma de onda y el medidor del nivel de entrada se mueva claramente por encima
del ruido, sin llegar a 0 dBFS. Demasiada ganancia amplifica el ruido de la
sala y puede recortar los batidos.
Desactive las «mejoras» de audio del sistema operativo: la supresión de ruido,
el control automático de ganancia y la cancelación de eco modifican la señal y
perturban la medición.
== Ruido
- Trabaje en una sala silenciosa. Apague los ventiladores, los altavoces y los
  aparatos ruidosos cercanos.
- No toque la mesa ni los cables durante la medición.
- Una almohadilla blanda bajo el soporte aísla el reloj de las vibraciones de
  la mesa.
- El #ui[Algoritmo ligero] (en #ui[Configuración del algoritmo…], #ui[Básico]) omite el supresor de ruido. Déjelo
  desactivado salvo que el ordenador sea demasiado lento.
