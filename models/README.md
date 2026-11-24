# Healthpack assets

Files used by the Medic healthpack (see `Weapon_Healthpack` in `src/p_weapon.c`). They are not inside the DLL: copy them
to the `dday` folder of the **server and of every player** (or add them to a pak), keeping these paths:

| File | What it is |
|---|---|
| `models/items/healthpack/tris.md2` + `skin.png` | The pack on the ground (a 14 unit crate, one skin with its 6 faces). |
| `models/weapons/v_healthpack/tris.md2` + `skin.png` | The crate in the hands of the Medic (50 frames: 0-3 raise, 4-8 throw, 9-45 idle, 46-49 lower). |
| `pics/w_healthpack.png` | HUD icon (48x24) of the selected item. |

The skins are PNG (512x256, the 6 faces of the crate in one image). Like the other models of the game, the `.md2` files
name their skin as `skin.pcx` and the client loads the `skin.png` next to them, so the client must be Q2PRO. A skin named
`.png` inside the `.md2` or a bigger skin (1024x512) made Q2PRO r1504 reject the model with "Invalid file format".
The server also needs `models/items/healthpack/tris.md2` and `weapons/tnt/toss.wav` (already in the game) to precache them.

How to use it in game: bind a key to `use special` (the Medic has 2 healthpacks per life), then press fire to throw one.

**Automatic download:** the `.md2` files name `skin.pcx` and the automatic download of the server asks for `.pcx`, so
next to each `skin.png` (and to `pics/w_healthpack.png`) there is an 8 bit `.pcx` with the **Quake 2 palette**
(`pics/colormap.pcx`): Q2PRO draws 8 bit `.pcx` files with that palette and ignores the one inside the file, so a `.pcx`
with its own palette shows broken colors. Index 255 is kept for the transparent pixels of the icon. This way a player
who does not have the files downloads them when he connects (`allow_download 1` on the server); whoever has the
`.png` keeps seeing that one. The server needs the 8 files: the 2 `.md2`, the 2 skins in `.png` and `.pcx`, and the
icon in both formats.

The packs stay on the ground for 3 minutes (`HEALTHPACK_LIFE`), also after the Medic dies, with at most 6 per Medic
(`HEALTHPACK_MAX_PER_MEDIC`, a new one removes his oldest); they are removed when he leaves. The Medic can take his own
packs one second after throwing them.
