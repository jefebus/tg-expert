#import "../lib.typ": *

= Primer inicio
Al iniciarse, tg-expert abre el dispositivo de entrada predeterminado del
sistema de audio predeterminado. Si no puede abrirse ninguna entrada, muestra
el mensaje #ui[No se pudo abrir ninguna entrada de audio.] con un botón que
lleva a los ajustes de audio.
== El diálogo de ajustes de audio
Ábralo con #ui[Ajustes de audio…] en el menú #ui[Ajustes], o haciendo clic en el botón de audio de la barra
superior, que muestra la entrada que se está usando.
#shot("audio-settings.png", [El diálogo de ajustes de audio. La línea situada
bajo el medidor de nivel muestra la entrada en uso.], width: 55%)
/ Sistema de audio: el sistema de sonido usado para grabar y escuchar en
  directo. En Windows son MME, DirectSound, WASAPI y WDM-KS. En Linux, ALSA y
  JACK si se está ejecutando. Al elegir otro sistema de audio se selecciona
  su entrada y frecuencia predeterminadas. La lista de salidas también
  corresponde a este sistema.
/ Dispositivo de entrada: el micrófono o la entrada de línea desde la que se
  graba.
/ Dispositivo de salida: el dispositivo para la escucha en directo. Solo
  aparecen dispositivos del sistema de audio seleccionado.
/ Frecuencia de muestreo: la frecuencia a la que se graba la entrada. Solo
  aparecen las frecuencias que admite el dispositivo, entre 22.05, 32, 44.1 y
  48 kHz. 44.1 kHz es un buen valor predeterminado. Una frecuencia más alta no
  siempre es mejor, sobre todo con un micrófono ruidoso: use la frecuencia más
  baja que dé una lectura estable.
/ Nivel de entrada: un medidor que muestra el nivel de la señal entrante en
  dBFS, y que se actualiza mientras el diálogo está abierto.
Los cambios surten efecto de inmediato: la entrada de audio se reinicia y la
medición vuelve a empezar. La elección se guarda y se usa en el siguiente
inicio.
Los ajustes de audio no pueden cambiarse durante la calibración ni mientras
está seleccionada una pestaña de captura. El diálogo lo indica cuando ocurre.
== Comprobar el nivel de entrada
Acerque el reloj al micrófono y observe el medidor.
- Si el medidor apenas se mueve, está seleccionado el dispositivo equivocado o
  la ganancia es demasiado baja. Suba la ganancia del micrófono en los ajustes
  de sonido del sistema operativo.
- Si el medidor se mantiene cerca de 0 dBFS, la entrada está saturada. Baje la
  ganancia.
- Al golpear suavemente el micrófono, el medidor debería saltar. Si no lo
  hace, el programa está escuchando otro dispositivo.
El indicador de estado situado junto al reloj de la tabla de resultados indica
si el programa reconoce un batido. Véase @main-window.
#note[Calibre la tarjeta de sonido antes de fiarse de la marcha (véase
@calibration). El valor cal se conserva, por lo que no es necesario repetirlo en cada inicio. Vuelve a calibrar si cambias
la entrada o la frecuencia de muestreo; consulta @calibration para los demás casos.]
