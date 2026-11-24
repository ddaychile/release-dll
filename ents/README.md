# Archivos `.ent` para Free For All

Estos archivos solo los lee **el servidor** (la DLL del juego, en `LoadEntFile`). Los jugadores que se conectan no necesitan tenerlos.

## Qué son

Con `ffa 1`, el juego busca `ents/<mapa>_ffa.ent` en la carpeta del juego del servidor y lo usa **en lugar de** `ents/<mapa>.ent`. Si no existe, usa el `.ent` normal del mapa. Con `ffa 0` nunca se lee, así que el modo por equipos no cambia.

Permiten agregar entidades que solo existen en FFA, por ejemplo más puntos de spawn (`info_reinforcements_start`), sin modificar el BSP ni el `.ent` normal.

## Instalación

Copia el archivo a la carpeta `ents/` del juego del servidor (junto a los demás `.ent`):

```
<carpeta del juego>/dday/ents/dday2_ffa.ent
```

Al cargar el mapa en FFA, la consola del servidor debe decir `dday2_ffa.ent Loaded (Free For All)`.

## Archivos

| Archivo | Descripción |
|---|---|
| `dday2_ffa.ent` | Copia de `dday2.ent` más 19 puntos de spawn para FFA (`info_reinforcements_start`). |

## Notas

- Cada `_ffa.ent` es una copia completa del `.ent` normal del mapa. Si el `.ent` normal cambia, hay que copiar el cambio al de FFA.
- No se usa si `ent_files` vale 0, ni en mapas con la entidad `misc_civilian` o la palabra `override` en el BSP (igual que el `.ent` normal).
