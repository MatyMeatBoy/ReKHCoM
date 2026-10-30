# Tres encuadres de escenas para 2.3

Propuesta del usuario, 30/10/2026. Aprobado explícitamente por el usuario para implementar las tres opciones.

## Comportamiento

Un único selector en launcher y opciones del juego:

- **Ajuste (predeterminado, valor0):** cámara cercana entera equivalente al GBA;
  sólo las versiones vigentes aprobadas con honores conservan el zoom de la app.
- **Extendido (valor1):** todas las escenas usan el zoom de la app, como antes.
  Conserva el aviso experimental sobre arte de los bordes pendiente.
- **Plano original (valor2):** cámara cercana en todas las escenas, también las
  aprobadas con honores. Conserva las extensiones; no cambia a la vista Original.

Los tres modos mantienen HUD, caja, retratos y texto al zoom de interfaz elegido.
Sólo afectan escenas del motor en vista Auto, nunca campo, combate ni películas.
Se conserva el arreglo de retratos completos y los grants versionados con honores.

## Enfoque

Ampliar el selector existente y su política pura es suficiente: un segundo toggle
permitiría estados redundantes y separar ajustes por escena no responde a lo pedido.
Mantener0/1 evita migraciones destructivas; un INI antiguo sigue en Ajuste.
No crece PortConfig ni cambia la ABI. Nombre propuesto: Plano original, con ayuda
breve que distingue encuadre cercano de la vista Original de toda la aplicación.

## Comprobación

Tabla de los tres modos con escenas ordinarias/con honores y fuera de escena;
INI/default/guardado/límites; selector y navegación en ambos sentidos; pruebas de
consumidores reales. Comparación nativa en una escena con honores: Ajuste igual a
Extendido, Plano original más cercano, con retrato completo. No publicar release.
