# Sonidos para Free For All

Hay que copiarlos a la carpeta `sound/` del juego del servidor y de los jugadores (`<carpeta del juego>/dday/sound/ffa/`), o incluirlos en el pak de distribución. Si el servidor tiene `allow_download_sounds` activado, el cliente puede pedirlos al conectarse (el servidor los precarga con `gi.soundindex`).

Si falta alguno, el juego funciona igual: ese aviso sonará en silencio.

| Archivo | Duración | Uso |
|---|---|---|
| `ffa/gg_up2.wav` | 2,0 s | Subir de escalón en el Gun Game (por defecto). |
| `ffa/gg_up1.wav` | 1,0 s | Alternativa más corta para subir de escalón. |

Son WAV mono, 16 bit, 22050 Hz, normalizados al máximo (el motor de Quake II no admite estéreo). Se envían solo al jugador que sube, a volumen completo y sin atenuación.

## Probar otro sonido sin recompilar

Las cvars `ffa_gg_snd_up` y `ffa_gg_snd_down` (se leen al cargar el mapa, escribirlas antes del `map`) indican la ruta relativa a `sound/`:

```
set ffa_gg_snd_up ffa/gg_up1.wav
set ffa_gg_snd_down misc/menu3.wav
map dday1
```
