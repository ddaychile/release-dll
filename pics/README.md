# Imágenes para Free For All

Estas imágenes las dibuja el **cliente** en el scoreboard de Free For All. Hay que copiarlas a la carpeta `pics/` del juego del servidor y de los jugadores (`<carpeta del juego>/dday/pics/`), o incluirlas en el pak de distribución. Si el servidor tiene `allow_download_pics` activado, el cliente puede pedirlas al conectarse (el servidor las precarga con `gi.imageindex`; la descarga automática de PNG hay que verificarla en cada motor).

Si falta alguna, el juego funciona igual: el texto se dibuja sin fondo.

| Archivo | Tamaño | Uso |
|---|---|---|
| `ffa_score.png` | 320 x 332 px, PNG 32 bits con transparencia | Fondo del scoreboard (las dos páginas). Zona oscura útil: x 23..297, y 28..270. Título en y 30, encabezado en y 44, 20 filas de 10 px desde y 58. |
| `ffa_winner.png` | 320 x 112 px, PNG 32 bits con transparencia | Placa del ganador sobre el scoreboard final. Zona oscura útil: y 30..82. Título en y 34, número grande en y 44, "FRAGS" en y 72. |

Las posiciones del texto están en las constantes `FFA_PANEL_*`, `FFA_TEXT_X`, `FFA_ROWS` y `FFA_BANNER_*` de `src/p_hud.c`. Si se cambian las imágenes por otras de otro tamaño, hay que ajustar esas constantes.

## Notas

- Se usan como PNG (el cliente debe soportarlo, como Q2PRO). En motores sin soporte PNG el fondo no se verá.
- Cada píxel de la imagen equivale a 1 unidad del layout (320 x 240); el motor lo escala a la pantalla.
- `ffa_score.png` incluye el logo de la Comunidad D-Day Normandy Chile en la franja inferior.
