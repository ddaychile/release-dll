# Imágenes para Free For All

Estas imágenes las dibuja el **cliente** en el scoreboard de Free For All. Hay que copiarlas a la carpeta `pics/` del juego del servidor y de los jugadores (`<carpeta del juego>/dday/pics/`), o incluirlas en el pak de distribución. Si el servidor tiene `allow_download_pics` activado, el cliente puede pedirlas al conectarse (el servidor las precarga con `gi.imageindex`; la descarga automática de PNG hay que verificarla en cada motor).

Si falta alguna, el juego funciona igual: el texto se dibuja sin fondo.

| Archivo | Tamaño | Uso |
|---|---|---|
| `ffa_score.png` | 320 x 332 px, PNG 32 bits con transparencia | Fondo del scoreboard (las dos páginas). Zona oscura útil: x 23..297, y 28..270. Título en y 30, encabezado en y 44, 20 filas de 10 px desde y 58. |
| `ffa_winner.png` | 320 x 112 px, PNG 32 bits con transparencia | Placa del ganador sobre el scoreboard final. Zona oscura útil: y 30..82. Título en y 34, número grande en y 44, "FRAGS" en y 72. |

## Banners del Gun Game (`ffa_gungame.png`, `ffa_gungame_win.png`)

Con `ffa_gungame 1`:

- `ffa_gungame.png`: durante la pausa inicial del mapa (la misma de la ruleta, 10 s) el HUD muestra este banner en lugar de la ruleta (`stat 2`, `src/p_hud.c`). Se dibuja en x 0, y 6 del layout de 320 x 240. El HUD no escribe ningún texto encima: el texto va dentro de la imagen.
- `ffa_gungame_win.png`: la misma imagen con la placa central vacía (sin "GUN GAME"). La usa el scoreboard final en lugar de `ffa_winner.png`, justo sobre el panel, y el juego escribe encima el nombre del ganador ("NOMBRE WINS!", `string2`, centrado en x 160 e y 68 de la imagen: `FFA_BANNER_GG_NAME_Y`). Si se rehace el arte, la placa debe quedar despejada en la zona oscura y 56..88 (x 73..247).

Las dos son de **320 x 112 px**, PNG 32 bits (el arte original, de 2120 x 742, se redujo a este tamaño: tiene exactamente la misma proporción). Basta reemplazar los archivos, sin recompilar.

## Ruleta de armas (`ffa_w_*.png`)

Durante la pausa inicial de cada mapa de Free For All, el HUD muestra una ruleta con los iconos de las armas del sorteo (`FFA_RollImage`, `src/p_hud.c`). Hay un icono por arma de fuego del juego (41 en total, uno por cada `w_*` de las 8 facciones): `ffa_w_m1.png`, `ffa_w_thompson.png`, `ffa_w_colt45.png`, etc. Son de **320 x 112 px**: el marco de `ffa_winner.png` con el icono original del juego centrado sobre una placa clara opaca (y 43..71), para que se vea cualquier arma. El título "WEAPON OF THE MATCH" lo dibuja el HUD en y 40 y el nombre del arma cae en y 78, bajo la placa (el banner va en `yv 6`). El nombre es siempre `ffa_` + el nombre del icono del arma (`item->icon`).

Se generan a partir de los iconos originales del juego (`pics/w_*.png` o `.pcx`). Si se agrega un arma nueva, hay que crear su `ffa_<icono>.png`; si falta, el cliente dibuja el cuadro rojo de "imagen no encontrada" en ese paso de la ruleta.

Las posiciones del texto están en las constantes `FFA_PANEL_*`, `FFA_TEXT_X`, `FFA_ROWS` y `FFA_BANNER_*` de `src/p_hud.c`. Si se cambian las imágenes por otras de otro tamaño, hay que ajustar esas constantes.

## Killfeed (`kf_*.png`)

El killfeed (`src/p_killfeed.c`) funciona en todos los modos de juego y se dibuja arriba a la izquierda, dentro del statusbar. Cada fila usa imágenes que carga el **cliente**: hay que copiarlas a `<carpeta del juego>/dday/pics/` del servidor y de los jugadores, o incluirlas en el pak. A diferencia de las de Free For All, el servidor **no** las registra con `gi.imageindex` (son 186 y el límite de imágenes del nivel es 256), así que el cliente no las descarga solo. Si faltan, el comportamiento depende del motor del cliente (sin verificar): lo esperable es que no dibuje ese fondo o ese icono y que el texto igual se vea.

Una fila es `[bandera] killer  arma  [headshot]  [bandera] víctima`, sobre una placa. Sin killer (suicidio, entorno, granada propia) es `calavera  [bandera] víctima`.

| Archivo | Tamaño | Uso |
|---|---|---|
| `kf_bg_<equipo>_<ancho>.png` | `<ancho>` de 48 a 344 en pasos de 8, x 14 px, PNG 32 bits con transparencia | Placa de la fila: el juego usa la más angosta en la que cabe. Equipo `a` (equipo 0, verde oliva), `x` (equipo 1, rojo ladrillo), `n` (sin equipo y Free For All, caqui). |
| `kf_w_<icono>.png` | ancho variable x 14 px | Icono del arma a la mitad del tamaño del original (alto máximo 12 px, centrado en 14), en tono hueso. `<icono>` es el `item->icon` del arma. Hay uno por cada `w_*` salvo `w_sabre`, que mide 256 x 256. |
| `kf_skull`, `kf_nade`, `kf_plane`, `kf_boom`, `kf_tank` | 12, 12, 30, 12 y 30 x 14 px | Calavera (suicidios, entorno y muertes sin icono propio), granadas, airstrike, explosivos/barriles/bombas y tanque. Dibujados a mano (formas vectoriales). |
| `kf_hs.png` | 14 x 14 px | Marca de headshot, a la derecha del arma (la misma condición que el cue "Headshot!"). |
| `kf_<facción>.png` | 12 x 14 px | Bandera de la facción del jugador (`usa usm gbr grm ita jpn pol rus`), a partir de `pics/teams/<facción>.pcx`. En Free For All no se dibujan: todos están en el mismo equipo. |

El ancho de cada icono está en la tabla `kf_icons` de `src/p_killfeed.c`: el statusbar no puede medir imágenes, así que la posición de lo que va a su derecha se calcula con ese ancho. Si se cambia un icono por otro de otro ancho, o se agrega un arma, hay que actualizar la tabla. Un arma que no esté en la tabla se dibuja con la calavera.

Los iconos de armas se generan a partir de los `pics/w_*.pcx` del juego (reducidos con promedio de área y pasados a tono hueso). El statusbar del engine tiene un límite de 1535 bytes: el killfeed va delante del statusbar del nivel y, si no cabe, se descartan las filas más viejas. Con nombres normales caben 3 filas.

Los nombres van siempre en blanco (`string`): el color del equipo lo da la placa.

## Notas

- Se usan como PNG (el cliente debe soportarlo, como Q2PRO). En motores sin soporte PNG el fondo no se verá.
- Cada píxel de la imagen equivale a 1 unidad del layout (320 x 240); el motor lo escala a la pantalla.
- `ffa_score.png` incluye el logo de la Comunidad D-Day Normandy Chile en la franja inferior.
