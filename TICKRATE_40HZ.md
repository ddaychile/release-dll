# Tickrate de 40 Hz (rama `netcode`)

El servidor puede correr a 40 Hz en vez de 10 Hz usando el fps variable de Q2PRO.
Si el engine no lo soporta, la DLL arranca a 10 Hz y se comporta como antes.

## Requisitos

- Engine Q2PRO compilado con `-Dvariable-fps=true` (el cvar `sv_fps` tiene que existir).
  Para comprobarlo, escribir `sv_fps` en la consola del servidor: si dice
  que no existe, ese engine se queda en 10 Hz aunque se use esta DLL.
- Sin Q2Admin (su reloj interno sigue a 10 Hz y no esta adaptado).

## Compilar la DLL (Linux)

```
git fetch && git checkout netcode
cd src
make
```

El Makefile genera `game<arch>.real.so` (por ejemplo `gamex86_64.real.so`).
Sin Q2Admin hay que renombrarlo al nombre que carga el engine
(en x86_64 `gamex86_64.so`; en otras arquitecturas, el mismo nombre que ya use
su DLL actual) y dejarlo en la carpeta `dday/`.

## Configuracion a mano en el servidor

En el `server.cfg`, antes de la linea que carga el mapa (`map ...`):

```
set sv_fps 40
```

- `sv_fps` es `LATCH`: se aplica al iniciar o al cambiar de mapa, no en caliente.
- Tiene que ser multiplo de 10 y como maximo 60. La DLL lo toma tal cual
  (40 = 4 ticks por frame de juego).
- Si el cfg ya tenia otro `sv_fps` (por ejemplo `seta sv_fps 30`), dejar solo uno.
  Con engines sin fps variable ese valor se ignoraba; con esta DLL y un engine
  con fps variable si se aplica.
- No hay que tocar ningun otro cvar del juego: los tiempos de partida,
  respawn, cuentas atras, etc. siguen en segundos reales.
- `sv_maxrate`: si el servidor lo tiene limitado, subirlo (con 40 snapshots
  por segundo hace falta mas ancho de banda por cliente).

## Verificar

Al cargar el mapa, el log debe mostrar:

```
==== Tickrate: 40 Hz (4 ticks por frame de juego) ====
```

y `sv_fps` debe valer 40. El cvar `g_features` debe incluir el bit 2048
(lo pone la DLL, no hay que configurarlo).

## Clientes

- Cada cliente entra recibiendo 10 snapshots por segundo. Solo un cliente
  Q2PRO con fps variable puede subir, segun su `cl_updaterate`
  (0 = lo mas alto que ofrezca el servidor).
- Se recomienda `rate` de 25000 o mas en el cliente.
- Los clientes sin Q2PRO (vanilla, Yamagi, R1Q2, etc.) siguen jugando a 10 Hz.

## Volver a 10 Hz

`set sv_fps 10` y reiniciar el mapa, o poner la DLL anterior.

## Pendiente

- Monsters y actores (`m_*`) siguen avanzando a 10 Hz.
- Adaptar Q2Admin-kmod si se quiere usar con 40 Hz.
