# Escenas: Ajuste y Extendido para 2.3

## Objetivo humano

Las cinemáticas realizadas con el motor del juego deben ofrecer un encuadre
cercano al GBA, equivalente visual al actual x6 / «integer1», para disimular
márgenes de escenarios aún incompletos. Se conservan las extensiones y mejoras
existentes: cambia el zoom, no el arte, los actores, los diálogos o la lógica.

El usuario aclara que no exige x6 físico en todas las resoluciones, sino su
equivalente de escala entera en el área real de juego. Diálogos y HUD conservan
el tamaño asociado al zoom elegido por el jugador.

## Comportamiento propuesto

- **Ajuste**, seleccionado por defecto también al actualizar una configuración
  que aún no tenga esta preferencia: en escenas sin una excepción aprobada,
  usar la mayor escala entera que conserve los240×160nativos en el área de
  presentación. A1080p corresponde x6 y a720p x4; ambos muestran aproximadamente
  180líneas lógicas. Se calcula desde el área realmente disponible, no desde
  el nombre de la resolución ni la etiqueta fija «integer1» del menú actual.
- Las escenas con aprobación explícita **con honores** conservan el encuadre
  general elegido (por ejemplo x4). Una aprobación normal no basta. El registro
  debe vincular escena y versión de arte aprobada; una variante o revisión nueva
  no hereda automáticamente la excepción. Conciliar primero LOGICA_APROBADOS y
  las referencias vigentes de la app; documentar la lista exacta comprobada.
- **Extendido** reproduce el comportamiento actual de la cámara en las escenas,
  usando el zoom general y las extensiones existentes.
- Guardar la preferencia de forma persistente y ofrecerla en el launcher y en
  las opciones nativas. Se puede cambiar durante una escena sin reiniciarla.

Texto propuesto junto al selector:

> **Escenas: Ajuste / Extendido (experimental)**
>
> Ajuste usa un encuadre cercano al original. Extendido muestra más escenario;
> algunas ilustraciones de los márgenes aún están en desarrollo.

«Con honores» es una regla interna de la selección de cámara, no una exigencia
para que el jugador comprenda o utilice la opción.

## Alcance técnico y decisiones del implementador

Separar el zoom de cámara del zoom de interfaz. Reutilizar el canvas y la capa
HUD existentes; no sobrescribir `zoom`, `hudSize` ni el override de combate para
conseguir el acercamiento. Al terminar la escena, restaurar el encuadre previo
sin alterar preferencias. La escala entera debe mantenerse al redimensionar y
en pantallas altas/ultraanchas, respetando los límites del canvas y evitando
estiramiento o recorte de los240×160nativos.

Aplicar a escenas con motor, incluidos sus diálogos y reproducción equivalente
en la galería. No cambiar películas prerenderizadas, exploración, combates,
menús ni la vista Original/CRT cuando el jugador la haya elegido. Los modos
existentes conservan sus propias reglas; el selector nuevo se aplica a la vista
ampliada/Auto que permite mostrar márgenes.

Se estudiaron un x6 fijo y una escala de mundo/interfaz compartida. No encajan:
el primero cambia de encuadre o recorta según resolución; el segundo cambia el
tamaño de diálogos/HUD pese a la indicación del usuario. La escala de cámara
entera calculada por viewport y la capa UI separada son el enfoque recomendado.

Evitar ampliar PortConfig sin revisar su ABI con huéspedes64bits. Mantener el
estado derivado de cámara separado de la preferencia persistida y de los datos
de escena originales. Ningún cambio de arte ni aprobaciones se publica aquí.

## Validación antes de integrar

1. Comparar Ajuste/Extendido en una escena sin excepción y en otra con honores,
   con los mismos frames, arte y configuración. Verificar que Extendido conserva
   la vista actual y Ajuste mantiene personajes, diagonales y extensiones.
2. Probar720p,1080p,1440p,2160p, ventana pequeña y formato alto/ultraancho:
   escala entera de cámara, centro estable y240×160nativos completos.
3. Verificar diálogos, elecciones y HUD con distintos zooms de usuario: legibles,
   completos y con tamaño/posición independientes del acercamiento del mundo.
4. Entrada/salida, transiciones consecutivas, cambio de opción y resize no
   modifican escena, temporización, cámara original, combate ni preferencias.
5. Pruebas de configuración anterior, guardado/reapertura,32/64bits y controles
   Original/CRT. Capturas reales sin control de escritorio mientras siga vigente
   el Escape previo; usar captura interna del juego cuando sea viable.

Estado: diseño escrito para revisión; implementación pendiente de aprobación.
