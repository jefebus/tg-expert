#import "../lib.typ": *

// Local presentation: each individual problem has its own heading.
#show heading.where(level: 3): set text(
  size: 10pt,
  weight: "bold",
  fill: navy,
)
#show heading.where(level: 3): set block(
  fill: pale,
  inset: 7pt,
  radius: 2pt,
  above: 1.5em,
  below: 0.6em,
  sticky: true,
)

= Solución de problemas <troubleshooting>
== Sin entrada
=== Al iniciar aparece el mensaje «No se pudo abrir ninguna entrada de audio.»

- Abra los ajustes de audio y elija otro sistema de audio u otro dispositivo
  de entrada.
- Compruebe que el sistema operativo ve el micrófono y que no está silenciado
  ni desactivado.
- Compruebe que ningún otro programa usa la entrada de forma exclusiva.
=== El medidor de los ajustes de audio no se mueve

- Está seleccionado el dispositivo equivocado, o su ganancia está a cero. Dé
  unos golpecitos al micrófono y vea si el medidor reacciona.
=== Windows: acceso al micrófono
 Windows 10 y 11 pueden bloquear el acceso de
los programas de escritorio al micrófono. Abra *Configuración > Privacidad y
seguridad > Micrófono* y active tanto *Acceso al micrófono* como *Permitir que
las aplicaciones de escritorio accedan al micrófono*. Después reinicie
tg-expert.
=== Windows: entradas WDM-KS
 El sistema de audio WDM-KS muestra muchas
entradas, y algunas no pueden abrirse, o se abren pero entregan silencio. Si
una entrada de WDM-KS falla, elija el mismo dispositivo en WASAPI o MME. MME
es la opción más compatible.
=== Windows: «mejoras» de audio
 Algunos controladores aplican supresión de
ruido o ganancia automática al micrófono. Desactívelas en las propiedades del
dispositivo, dentro de los ajustes de sonido de Windows.
=== Linux: dispositivo ocupado
 Si JACK o PipeWire-JACK se está ejecutando, los
dispositivos ALSA pueden estar ocupados. Elija JACK como sistema de audio, o
detenga JACK.
=== macOS: sin entrada
 Permita el acceso al micrófono en *Ajustes del Sistema >
Privacidad y seguridad > Micrófono* para tg-expert o para la terminal que lo
inició.
== bph incorrectos
=== La columna BEAT muestra un valor marcado con \~, o la marcha es absurda (cientos de s/día)

- Con #ui[auto], el programa puede fijarse en una frecuencia de batido
  equivocada cuando la señal es débil. Ajuste los bph a mano si se conocen.
- Si los bph están ajustados a mano, compárelos con los datos del calibre.
  21600 y 28800 son los valores más comunes. Un mecanismo de 18000 bph
  ajustado a 21600 muestra una marcha de miles de s/día.
- El icono de estado muestra *!* cuando no hay un ritmo válido a los bph
  seleccionados.
== Lecturas inestables
=== La marcha salta de un valor a otro, o el diagrama de marcha está disperso

- Mejore primero la señal: apoye el micrófono en la caja, reduzca el ruido de
  la sala y ajuste la ganancia. Véase @microphone.
- Revise los paneles de forma de onda. Unos clics nítidos y separados indican
  una buena señal.
- Desactive el #ui[Algoritmo ligero] si está activado.
- No toque el reloj ni la mesa durante la medición.
- Parte de la inestabilidad se debe al propio reloj. Un mecanismo desgastado o
  sucio varía de un batido a otro. La σ de la página de tendencia indica
  cuánto.
=== La amplitud falta o no es verosímil

- Compruebe el ángulo de alzamiento.
- La amplitud necesita una medida fiable del tiempo de alzamiento acústico: deben detectarse el inicio del desbloqueo y la referencia final fuerte de bloqueo/tope. Una señal débil o ruidosa puede dar una marcha estable pero una amplitud poco fiable.
=== La marcha se desvía de forma constante respecto al comportamiento real del reloj

- Calibre la tarjeta de sonido (véase @calibration), y vuelva a calibrar
  después de cambiar la entrada de audio o la frecuencia de muestreo.
