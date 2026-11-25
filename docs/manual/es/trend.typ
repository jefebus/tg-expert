#import "../lib.typ": *

= Medición a largo plazo <trend>
La marcha de un reloj cambia a medida que se desarma el muelle real y cuando
el reloj cambia de posición. La página #ui[Tendencia] registra la medición a
lo largo de horas, para que estos cambios se hagan visibles.
#shot("trend.png", [La página Tendencia con estadísticas y gráficas de las
lecturas registradas.])
== Registro
Cada 2 segundos, mientras la señal es buena, el programa registra la marcha, la
amplitud y el error de batido, junto con la posición seleccionada. Los
períodos sin una buena señal se omiten.
El registro vuelve a empezar cuando cambia un ajuste que modifica los
resultados: los bph, el ángulo de alzamiento, cal, la entrada de audio, la
frecuencia de muestreo o el algoritmo ligero. #ui[Reiniciar] lo reinicia a
mano. El registro se detiene a las 100 horas.
Con #ui[BEAT] ajustado a #ui[auto], el programa puede reconocer brevemente otra
frecuencia de batido mientras se gira el reloj o cuando la señal es débil.
Esas lecturas se omiten y el registro continúa. Solo una frecuencia de batido
detectada que se mantenga distinta durante un minuto reinicia el registro.
Para registros largos, ajuste la frecuencia de batido a mano.
Durante la calibración no se registra nada.

== Estabilización al inicio y tras cambiar de posición

En la página de tendencia en tiempo real, ajusta #ui[Tiempo de estabilización
(s)] (0 por defecto, lo que la desactiva). El ajuste se guarda entre
sesiones y los cambios se aplican a la siguiente cuenta atrás.

La cuenta atrás comienza al iniciar la aplicación y continúa incluso sin
buena señal, para excluir las primeras lecturas afectadas por la ventana de
suavizado.
Selecciona la nueva posición en el programa *antes* de mover físicamente el
reloj. Al cambiar desde una posición definida a otra, incluida #ui[sin
definir], la cuenta atrás empieza inmediatamente y continúa incluso sin buena
señal. Volver a una posición usada anteriormente la inicia de nuevo. La
posición seleccionada al iniciar el registro no necesita cambiarse para que
se aplique la cuenta atrás inicial. #ui[Reiniciar] no inicia ni reinicia la
cuenta atrás; si hay una activa, continúa.

Las lecturas con buena señal durante la cuenta atrás se conservan, se
resaltan en las gráficas y se marcan como #ui[Estabilización] en el CSV. Se
excluyen de las estadísticas generales y por posición, incluidos el error
posicional y las medias de las gráficas. La columna *actual* sigue mostrando
la última lectura. Las capturas guardan estas marcas; las versiones antiguas
del programa pueden ignorarlas.

== Estadísticas desde el inicio
La tabla de la parte superior muestra, para la marcha (s/día), la amplitud (°)
y el error de batido (ms):
- *actual*: la última lectura;
- *media*: el promedio;
- *mín.* y *máx.*;
- *rango*: máx. menos mín.;
- *σ*: la desviación típica.
Debajo, la página muestra cuánto tiempo lleva el registro, el porcentaje de
ese tiempo con buena señal, y los bph, el ángulo de alzamiento y el valor cal
usados.
== Estadísticas por posición
Cuando se ha indicado la posición durante el registro, una segunda tabla
muestra, para cada posición: el tiempo que se ha pasado en ella, y la marcha,
la amplitud y el error de batido medios. Las lecturas tomadas sin posición
indicada aparecen como #ui[sin definir].
Con dos o más posiciones, la página muestra el *error posicional*: la mayor
diferencia entre las marchas medias de las posiciones. Un error posicional
pequeño indica que el reloj está bien ajustado.
== Gráficas
En la parte inferior, tres gráficas comparten el eje de tiempo: marcha,
amplitud y error de batido. Cada una muestra una línea discontinua con su
media cuando esta cabe en la escala visible. Una línea vertical con el código
de posición (DU, DD, ...) marca cada cambio de posición. Los controles
situados bajo la página establecen:
/ Marcha: el intervalo vertical de la gráfica de marcha: #ui[Auto], ±10 s/día
  o ±50 s/día.
/ B.E.: #ui[Todo] ajusta automáticamente la escala del error de batido;
  < 1 ms y < 2 ms fijan su límite superior. Los valores mayores aparecen en
  el borde, sin cambiar los datos ni las estadísticas. El ajuste también se
  aplica a Correlaciones.
/ Mostrar: el tiempo mostrado: #ui[Todo], o la última 1, 6 o 24 horas.

== Correlaciones

La página #ui[Correlaciones] usa las mismas lecturas registradas. Sus tres
diagramas de dispersión muestran amplitud → marcha, error de batido → marcha
y error de batido → amplitud. Los puntos no se unen y una relación no
demuestra causalidad. El selector muestra todas las posiciones como series
separadas o una sola posición. Los colores y símbolos son iguales en las tres
gráficas. #ui[(sin definir)] indica una posición desconocida, no otra posición
física.

#shot("correlations.png", [La página Correlaciones con sus tres diagramas de
dispersión y el filtro de posición.])

El selector #ui[Mostrar] y los rangos de los ejes se comparten con Tendencia,
independientemente del filtro de posición. Los valores fuera de esos rangos
se muestran en el borde. Al pasar sobre un punto se muestran el tiempo
transcurrido, la posición registrada y los valores disponibles. Una lectura
sin amplitud puede aparecer en error de batido → marcha; cero es válido para
la marcha y el error de batido. Se excluyen las lecturas marcadas como en
estabilización, aunque siguen influyendo en los rangos de Tendencia. Los
archivos antiguos sin marcas de estabilización no permiten identificarlas.
Las lecturas de solo marcha no se registran en Tendencia.

Cada lectura conserva la posición seleccionada al registrarla. El historial
no guarda el inicio de la ventana de audio del detector, por lo que no se
pueden identificar ni excluir de forma fiable las ventanas que cruzan un
cambio de posición. Los archivos antiguos sin posiciones registradas se
muestran como #ui[(sin definir)]; los que no tienen historial de Tendencia
no pueden proporcionar puntos.

== Exportar datos

Elige #ui[Exportar datos…] en el menú #ui[Archivo] para guardar
los datos de tendencia de la pestaña activa. Los valores de tiempo son los
segundos transcurridos desde el inicio del registro, no la hora del reloj. El
archivo incluye los códigos de posición y los ajustes de medición. Solo se
registran las lecturas con buena señal, por lo que los períodos omitidos
aparecen como saltos; las filas se marcan como válidas o en estabilización. La
amplitud queda vacía cuando no está disponible.

El algoritmo aparece como #ui[Ligero (personalizado)] o #ui[Completo
(personalizado)] si algún parámetro de medición utilizado en ese modo difiere
del predeterminado. No cuentan los parámetros de calibración ni los de
supresión de ruido en modo ligero. Las capturas conservan esta información;
las capturas antiguas que no la contienen se exportan como #ui[Ligero] o
#ui[Completo].

En el diálogo para guardar, elige CSV portable (separador coma y punto
decimal) o, si la interfaz está en español o alemán, CSV localizado
(separador punto y coma y coma decimal). La opción elegida se guarda y se
reutiliza en las siguientes exportaciones, incluso después de reiniciar el
programa. Los nombres de las columnas y los valores descriptivos siguen el
idioma de la interfaz; los códigos de posición se mantienen sin cambios.
Ambos formatos usan UTF-8 con marca de orden de bytes para que Excel reconozca
los caracteres acentuados.

== Medir la reserva de marcha
1. Dé cuerda completa al reloj y ajuste la posición.
2. Inicie la medición y deje el reloj sin tocarlo.
3. Vuelva al cabo de unas horas. La gráfica de amplitud muestra cómo cae la
  amplitud a medida que se desarma el muelle real, y la gráfica de marcha
  muestra cómo la sigue la marcha.
4. Haga una captura y guárdela. Los datos de tendencia se guardan con ella.

#note[*Antes de cambiar ajustes:* haz una captura y guárdala si necesitas conservar el registro actual. Cambiar bph, ángulo de alzamiento, cal, entrada de audio, frecuencia de muestreo o ajustes del algoritmo reinicia la tendencia. Para registros largos, introduce los bph conocidos en lugar de depender de la detección automática.]
