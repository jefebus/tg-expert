#import "../lib.typ": *
= Instalación
== Windows
Descargue `tg-expert_0.9.0.msi` de la
#link("https://github.com/jefebus/tg-expert/releases/latest")[página de versiones]
y ejecútelo. El instalador coloca tg-expert en su propia carpeta dentro de
`Program Files`, añade entradas al menú Inicio para el programa y para este
manual, y asocia los archivos `.tgj` con el programa.
En cualquier sistema, #ui[Manual de usuario] en el menú abre este manual. Si
el manual no está instalado, abre la página de versiones, desde donde puede
descargarse el PDF.
tg-expert se instala junto a tg-timer y no lo sustituye. Los dos programas
mantienen ajustes independientes.
Para desinstalarlo, use *Configuración > Aplicaciones* en Windows.
#note[Windows puede preguntar si se permite al programa usar el micrófono. Si
se denegó el acceso al micrófono, véase @troubleshooting.]
== Linux
Todavía no hay un paquete binario para Linux. Compile desde el código fuente
como se describe en `BUILDING.md`, incluido en el archivo tar del código
fuente, y después ejecute `make install`. Así se instalan el programa, la
página de manual, una entrada de escritorio y el tipo de archivo `.tgj`, y
este manual si Typst estaba disponible durante la compilación.
== macOS
No hay paquete para macOS. Compilar desde el código fuente con Homebrew
debería funcionar siguiendo `BUILDING.md`, pero no se ha verificado para esta
versión. En el primer inicio, macOS solicita permiso de micrófono para el
programa o para la terminal desde la que se inició. Permítalo en *Ajustes del
Sistema > Privacidad y seguridad > Micrófono*.
== Archivo de ajustes
tg-expert guarda sus ajustes en `tg-expert.ini`, en la carpeta de
configuración del usuario (`%LOCALAPPDATA%` en Windows, `~/.config` en Linux).
Almacena el tamaño de la ventana, los ajustes de audio, el valor cal, los
relojes guardados y los ajustes del diagrama de marcha. Si lo elimina, el
programa vuelve a sus valores predeterminados.
== Idioma
El programa muestra sus textos en el idioma del sistema operativo: inglés,
español o alemán. Para cualquier otro idioma usa el inglés. #ui[Manual de
usuario] en el menú abre el manual en el mismo idioma.
Algunas cosas son iguales en todos los idiomas, para que las lecturas puedan
compararse con las de cualquier cronocomparador:
- los números usan siempre un punto como separador decimal, como en
  +2.5 s/d;
- las cabeceras de la tabla de resultados RATE, AMP., B.E., L.A. y BEAT, y
  sus unidades;
- los códigos de posición DU, DD, CU, CD, CL y CR;
- el estado de la calibración: cal (wait), cal (acq.), cal (done),
  cal (fail).
En Linux, el idioma puede elegirse al iniciar el programa, por ejemplo
`LANGUAGE=es tg-expert`.
