# Imágenes para Free For All

Estas imágenes las dibuja el **cliente** en el scoreboard de Free For All. Hay que copiarlas a la carpeta `pics/` del juego del servidor y de los jugadores (`<carpeta del juego>/dday/pics/`), o incluirlas en el pak de distribución. Si el servidor tiene `allow_download_pics` activado, el cliente puede pedirlas al conectarse (el servidor las precarga con `gi.imageindex`; la descarga automática de PNG hay que verificarla en cada motor).

Si falta alguna, el juego funciona igual: el texto se dibuja sin fondo.

| Archivo | Tamaño | Uso |
|---|---|---|
| `ffa_score.png` | 320 x 332 px, PNG 32 bits con transparencia | Fondo del scoreboard (las dos páginas). Zona oscura útil: x 23..297, y 28..270. Título en y 30, encabezado en y 44, 20 filas de 10 px desde y 58. |
| `ffa_winner.png` | 320 x 112 px, PNG 32 bits con transparencia | Placa del ganador sobre el scoreboard final. Zona oscura útil: y 30..82. Título en y 34, número grande en y 44, "FRAGS" en y 72. |

## Ruleta de armas (`ffa_w_*.png`)

Durante la pausa inicial de cada mapa de Free For All, el HUD muestra una ruleta con los iconos de las armas del sorteo (`FFA_RollImage`, `src/p_hud.c`). Hay un icono por arma de fuego del juego (41 en total, uno por cada `w_*` de las 8 facciones): `ffa_w_m1.png`, `ffa_w_thompson.png`, `ffa_w_colt45.png`, etc. Son de **320 x 112 px**: el marco de `ffa_winner.png` con el icono original del juego centrado sobre una placa clara opaca (y 43..71), para que se vea cualquier arma. El título "WEAPON OF THE MATCH" lo dibuja el HUD en y 40 y el nombre del arma cae en y 78, bajo la placa (el banner va en `yv 6`). El nombre es siempre `ffa_` + el nombre del icono del arma (`item->icon`).

Se generan a partir de los iconos originales del juego (`pics/w_*.png` o `.pcx`). Si se agrega un arma nueva, hay que crear su `ffa_<icono>.png`; si falta, el cliente dibuja el cuadro rojo de "imagen no encontrada" en ese paso de la ruleta.

Las posiciones del texto están en las constantes `FFA_PANEL_*`, `FFA_TEXT_X`, `FFA_ROWS` y `FFA_BANNER_*` de `src/p_hud.c`. Si se cambian las imágenes por otras de otro tamaño, hay que ajustar esas constantes.

## Notas

- Se usan como PNG (el cliente debe soportarlo, como Q2PRO). En motores sin soporte PNG el fondo no se verá.
- Cada píxel de la imagen equivale a 1 unidad del layout (320 x 240); el motor lo escala a la pantalla.
- `ffa_score.png` incluye el logo de la Comunidad D-Day Normandy Chile en la franja inferior.
