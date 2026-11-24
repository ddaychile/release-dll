/*
 * g_vote.c -- votacion de los jugadores durante la partida.
 *
 * No es la votacion del final del mapa (mapvoting): esta la abre un jugador
 * cuando quiere, con la misma sintaxis que usaba q2admin en los servers viejos:
 *
 *   vote map <mapa>     propone cambiar a ese mapa en modo normal (el que propone vota si)
 *   vote normal <mapa>  lo mismo que vote map
 *   vote gungame <mapa> propone cambiar a ese mapa en Gun Game (si vote_modes lo permite)
 *   vote ffa <mapa>     idem en Free For All clasico
 *   vote restart        propone reiniciar el mapa actual
 *   vote yes | no       vota la propuesta en curso (tambien "si")
 *   vote maps [texto]   busca entre los mapas del servidor
 *   vote                muestra la votacion en curso o la ayuda
 *
 * Votan solo los jugadores humanos que estan en un equipo (no espectadores ni
 * bots). Se aprueba cuando los "si" superan el porcentaje de esos jugadores; si
 * se acaba el tiempo sin llegar, se rechaza (no votar cuenta como no).
 *
 * Cvars:
 *   vote_enable        0    1 = los jugadores pueden abrir votaciones
 *   vote_time          30   segundos que dura una votacion
 *   vote_percent       50   porcentaje de "si" que hay que superar
 *   vote_cooldown      60   segundos de espera tras una votacion rechazada
 *   vote_announce      600  segundos entre avisos de como votar (0 = sin avisos); ademas
 *                           se avisa al entrar al server, a los 45 s del mapa y al terminar
 *   vote_modes         ""   modos que se pueden votar ademas del normal: "gungame ffa"
 *                           ("" = solo vote map / vote normal)
 *
 * Mapas: cualquier .bsp de dday/maps que sea de D-Day (tiene spawns de equipo
 * en el .bsp o en ents/<mapa>.ent). Sin distinguir mayusculas.
 * vote map / vote normal siempre juega el mapa en modo normal: desde FFA / Gun Game vuelve al
 * modo normal (con mapvoting y sv_maplist del server.cfg, ver Vote_ToNormal). vote gungame / ffa
 * solo acepta mapas preparados para ese modo (ents/<mapa>_ffa.ent) y no se usa en torneo.
 * Despues del mapa votado todo sigue como siempre (votacion de fin de mapa, rotacion del modo).
 * En torneo el aviso de como votar sale solo al inicio y al final del mapa.
 * No se vota en campana ni intermision.
 * El admin puede cortar una votacion con "sv votecancel".
 */

#include "g_local.h"
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

#define VOTE_NONE	0
#define VOTE_YES	1
#define VOTE_NO		2

#define VTYPE_MAP		0
#define VTYPE_RESTART	1

#define VOTE_REMIND		10		// cada cuantos segundos se recuerda la votacion
#define VOTE_MAX_MAPS	2048
#define VOTE_SUGGEST	5

#ifndef MAX_OSPATH
#define MAX_OSPATH		256
#endif

extern qboolean disable_mapvoting;

// strcat que no se pasa del tamano del destino
static void Vote_Cat (char *dst, const char *src, int size)
{
	int len = strlen (dst);

	if (len < size - 1)
		Com_sprintf (dst + len, size - len, "%s", src);
}

static cvar_t *vote_enable;
static cvar_t *vote_time;
static cvar_t *vote_percent;
static cvar_t *vote_cooldown;
static cvar_t *vote_announce;	// segundos entre avisos de como votar (0 = sin avisos)
static cvar_t *vote_modes;		// modos que se pueden votar ademas del normal ("gungame ffa")

// avisos de como votar: al entrar al server (una vez por jugador), al empezar el mapa,
// cada vote_announce segundos y al terminar el mapa
static qboolean	howto_told[MAX_CLIENTS];	// ya se le explico (hasta que se desconecte)
static int		howto_frames[MAX_CLIENTS];	// aviso privado pendiente
static float	next_announce;
static qboolean	end_announced;

static struct
{
	qboolean	active;
	int			type;
	char		map[MAX_QPATH];		// mapa propuesto (el actual si es restart)
	int			vmode;				// modo en que se juega el mapa propuesto (VMODE_*)
	qboolean	to_normal;			// se vota el modo normal estando en FFA / Gun Game
	char		caller[32];
	int			caller_slot;
	float		end_time;
	float		next_remind;
	float		next_allowed;		// level.time desde el que se puede abrir otra
	byte		choice[MAX_CLIENTS];	// por slot de cliente
} vote;

// mapas del servidor (se leen una vez por mapa, al primer uso)
static char	vote_maps[VOTE_MAX_MAPS][MAX_QPATH];
static int	vote_nummaps = -1;

void Vote_Init (void)
{
	vote_enable       = gi.cvar ("vote_enable", "0", 0);
	vote_time         = gi.cvar ("vote_time", "30", 0);
	vote_percent      = gi.cvar ("vote_percent", "50", 0);
	vote_cooldown     = gi.cvar ("vote_cooldown", "60", 0);
	vote_announce     = gi.cvar ("vote_announce", "600", 0);
	vote_modes        = gi.cvar ("vote_modes", "", 0);
}

// al iniciar cada mapa (level se borra, pero esto es static)
void Vote_Reset (void)
{
	memset (&vote, 0, sizeof(vote));
	vote_nummaps = -1;	// por si subieron mapas nuevos
	next_announce = 45;	// primer aviso a los 45 s del mapa
	end_announced = false;
}

/*
==============================================================================
Avisos de como votar
==============================================================================
*/

static qboolean Vote_ModeAllowed (int vmode);
static void Vote_ModeUsage (char *out, int size, const char *sep);
static qboolean Vote_IsTournament (void);

static void Vote_HowTo (edict_t *ent)
{
	char		modes[128], l1[256];

	Vote_ModeUsage (modes, sizeof(modes), ", ");
	Com_sprintf (l1, sizeof(l1), "Votaciones: en la consola escribe vote map <mapa>%s, "
		"vote yes o vote no (vote maps <texto> busca mapas).\n", modes);

	if (ent)
		safe_cprintf (ent, PRINT_HIGH, "%s", l1);
	else
		safe_bprintf (PRINT_HIGH, "%s", l1);
}

// desde ClientBeginDeathmatch (cada mapa): solo la primera vez que entra al server
void Vote_ClientBegin (edict_t *ent)
{
	int slot = ent - g_edicts - 1;

	if (ent->ai || slot < 0 || slot >= MAX_CLIENTS || howto_told[slot])
		return;
	howto_told[slot] = true;
	howto_frames[slot] = 50;	// 5 s, despues de los mensajes de entrada
}

static qboolean Vote_AnyHuman (void)
{
	int i;

	for (i = 1; i <= game.maxclients; i++)
		if (g_edicts[i].inuse && g_edicts[i].client && !g_edicts[i].ai)
			return true;
	return false;
}

static void Vote_AnnounceFrame (void)
{
	int		i;
	edict_t	*ent;
	qboolean on = vote_enable->value && vote_announce->value > 0;

	for (i = 0; i < game.maxclients && i < MAX_CLIENTS; i++)
	{
		if (!howto_frames[i])
			continue;
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if (--howto_frames[i] == 0 && on)
			Vote_HowTo (ent);
	}

	if (!on)
		return;

	if (level.intermissiontime)
	{
		if (!end_announced && level.time > level.intermissiontime + 2)
		{
			end_announced = true;
			Vote_HowTo (NULL);
		}
		return;
	}
	if (vote.active || level.time < next_announce)
		return;
	// torneo: solo el aviso del inicio (45 s) y el del final, nada en medio
	if (Vote_IsTournament ())
		next_announce = 999999;
	else
		next_announce = level.time + (vote_announce->value > 60 ? vote_announce->value : 60);
	if (Vote_AnyHuman ())
		Vote_HowTo (NULL);
}

/*
==============================================================================
Mapas del servidor
==============================================================================
*/

static int Vote_CompareMaps (const void *a, const void *b)
{
	return Q_stricmp ((char *)a, (char *)b);
}

static void Vote_AddMap (const char *file)
{
	char	name[MAX_QPATH];
	int		len = strlen (file), i;

	if (len <= 4 || len - 4 >= MAX_QPATH || Q_stricmp ((char *)file + len - 4, ".bsp"))
		return;
	memcpy (name, file, len - 4);
	name[len - 4] = '\0';

	for (i = 0; i < vote_nummaps; i++)
		if (!Q_stricmp (vote_maps[i], name))
			return;		// ya esta (homedir y basedir)
	if (vote_nummaps < VOTE_MAX_MAPS)
		strcpy (vote_maps[vote_nummaps++], name);
}

static void Vote_ScanDir (const char *base)
{
	char path[MAX_OSPATH];

	if (!base || !base[0])
		return;
	Com_sprintf (path, sizeof(path), "%s/%s/maps", base, GAMEVERSION);

#ifdef _WIN32
	{
		struct _finddata_t	fd;
		intptr_t			h;
		char				pattern[MAX_OSPATH];

		Com_sprintf (pattern, sizeof(pattern), "%s/*.bsp", path);
		h = _findfirst (pattern, &fd);
		if (h == -1)
			return;
		do
			Vote_AddMap (fd.name);
		while (_findnext (h, &fd) == 0);
		_findclose (h);
	}
#else
	{
		DIR				*d = opendir (path);
		struct dirent	*e;

		if (!d)
			return;
		while ((e = readdir (d)) != NULL)
			Vote_AddMap (e->d_name);
		closedir (d);
	}
#endif
}

static void Vote_LoadMaps (void)
{
	if (vote_nummaps >= 0)
		return;
	vote_nummaps = 0;
	Vote_ScanDir (sys_homedir->string);
	if (Q_stricmp (sys_homedir->string, sys_basedir->string))
		Vote_ScanDir (sys_basedir->string);
	qsort (vote_maps, vote_nummaps, sizeof(vote_maps[0]), Vote_CompareMaps);
}

// indice del mapa (sin distinguir mayusculas), -1 si no esta
static int Vote_FindMap (const char *name)
{
	int i;

	Vote_LoadMaps ();
	for (i = 0; i < vote_nummaps; i++)
		if (!Q_stricmp (vote_maps[i], (char *)name))
			return i;
	return -1;
}

// strstr sin distinguir mayusculas
static qboolean Vote_Contains (const char *s, const char *sub)
{
	int i, n = strlen (sub);

	if (!n)
		return true;
	for (; *s; s++)
	{
		for (i = 0; i < n; i++)
			if (tolower ((unsigned char)s[i]) != tolower ((unsigned char)sub[i]))
				break;
		if (i == n)
			return true;
	}
	return false;
}

// distancia de edicion (nombres cortos), sin distinguir mayusculas
static int Vote_Distance (const char *a, const char *b)
{
	int		la = strlen (a), lb = strlen (b), i, j, cost, best;
	int		row[MAX_QPATH + 1], prev, tmp;

	if (la > MAX_QPATH || lb > MAX_QPATH)
		return 99;
	for (j = 0; j <= lb; j++)
		row[j] = j;
	for (i = 1; i <= la; i++)
	{
		prev = row[0];
		row[0] = i;
		for (j = 1; j <= lb; j++)
		{
			cost = tolower ((unsigned char)a[i-1]) != tolower ((unsigned char)b[j-1]);
			best = prev + cost;
			if (row[j] + 1 < best)
				best = row[j] + 1;
			if (row[j-1] + 1 < best)
				best = row[j-1] + 1;
			tmp = row[j];
			row[j] = best;
			prev = tmp;
		}
	}
	return row[lb];
}

// hasta VOTE_SUGGEST mapas parecidos: primero los que contienen el texto, despues por distancia
static void Vote_Suggest (const char *query, char *out, int size)
{
	int		picked[VOTE_SUGGEST], n = 0, i, j, d, maxd;

	out[0] = '\0';
	Vote_LoadMaps ();

	for (i = 0; i < vote_nummaps && n < VOTE_SUGGEST; i++)
		if (Vote_Contains (vote_maps[i], query) || Vote_Contains (query, vote_maps[i]))
			picked[n++] = i;

	maxd = strlen (query) / 3 + 1;
	if (maxd > 3)
		maxd = 3;
	for (d = 1; d <= maxd && n < VOTE_SUGGEST; d++)
		for (i = 0; i < vote_nummaps && n < VOTE_SUGGEST; i++)
		{
			for (j = 0; j < n; j++)
				if (picked[j] == i)
					break;
			if (j == n && Vote_Distance (vote_maps[i], query) == d)
				picked[n++] = i;
		}

	for (i = 0; i < n; i++)
	{
		Vote_Cat (out, i ? ", " : "", size);
		Vote_Cat (out, vote_maps[picked[i]], size);
	}
}

#define MAP_OK			0
#define MAP_NOT_DDAY	1
#define MAP_BROKEN		2

static qboolean Vote_HasTeamSpawns (const char *ents)
{
	return Vote_Contains (ents, "info_team_start") || Vote_Contains (ents, "info_reinforcements_start");
}

// el mapa es de D-Day: spawns de equipo en el .bsp o en ents/<mapa>.ent
static int Vote_CheckMap (const char *map)
{
	char	path[MAX_OSPATH], *ents;
	FILE	*f = NULL;
	byte	header[16];
	int		ofs, len, result = MAP_BROKEN;

	ents = ReadEntFile (va ("ents/%s.ent", map));
	if (ents)
	{
		qboolean ok = Vote_HasTeamSpawns (ents);
		gi.TagFree (ents);
		if (ok)
			return MAP_OK;
	}

	Com_sprintf (path, sizeof(path), "%s/%s/maps/%s.bsp", sys_homedir->string, GAMEVERSION, map);
	f = fopen (path, "rb");
	if (!f)
	{
		Com_sprintf (path, sizeof(path), "%s/%s/maps/%s.bsp", sys_basedir->string, GAMEVERSION, map);
		f = fopen (path, "rb");
	}
	if (!f)
		return MAP_BROKEN;

	// cabecera IBSP: magic, version, y el lump 0 (entidades) = offset, largo
	if (fread (header, 1, sizeof(header), f) == sizeof(header) && !memcmp (header, "IBSP", 4))
	{
		ofs = header[8] | (header[9] << 8) | (header[10] << 16) | (header[11] << 24);
		len = header[12] | (header[13] << 8) | (header[14] << 16) | (header[15] << 24);
		if (ofs > 0 && len > 0 && len < 8 * 1024 * 1024 && !fseek (f, ofs, SEEK_SET))
		{
			char *buf = gi.TagMalloc (len + 1, TAG_LEVEL);

			if (fread (buf, 1, len, f) == (size_t)len)
			{
				buf[len] = '\0';
				result = Vote_HasTeamSpawns (buf) ? MAP_OK : MAP_NOT_DDAY;
			}
			gi.TagFree (buf);
		}
	}
	fclose (f);
	return result;
}

/*
==============================================================================
Mapas de cada modo: FFA / Gun Game (ents/<mapa>_ffa.ent). En esos modos la
rotacion (EndDMLevel) y la votacion de mapa van solo entre los mapas
preparados para el modo.
==============================================================================
*/

// patron del archivo que prepara un mapa para el modo actual; NULL en los demas modos
static const char *Mode_FileFormat (void)
{
	if (G_IsFFA ())
		return "ents/%s_ffa.ent";
	return NULL;
}

// nombre del modo para los mensajes
const char *Mode_Label (void)
{
	return (ffa_gungame && ffa_gungame->value) ? "Gun Game" : "Free For All";
}

static qboolean Mode_FileExists (const char *fmt, const char *map)
{
	char	name[MAX_QPATH + 16];
	FILE	*f;
	int		i;

	Com_sprintf (name, sizeof(name), (char *)fmt, map);
	for (i = 0; name[i]; i++)
		name[i] = tolower ((unsigned char)name[i]);	// los archivos de ents/ van en minusculas (Linux)

	f = DDay_OpenFullPathFile (sys_homedir->string, GAMEVERSION, name, "r");
	if (!f)
		f = DDay_OpenFullPathFile (sys_basedir->string, GAMEVERSION, name, "r");
	if (!f)
		f = DDay_OpenFullPathFile (".", GAMEVERSION, name, "r");
	if (!f)
		return false;
	fclose (f);
	return true;
}

// mapas del servidor preparados para el modo actual (en orden alfabetico); 0 fuera de esos modos
static int Mode_Maps (int *idx, int max)
{
	const char	*fmt = Mode_FileFormat ();
	int			i, n = 0;

	if (!fmt)
		return 0;
	Vote_LoadMaps ();
	for (i = 0; i < vote_nummaps && n < max; i++)
		if (Mode_FileExists (fmt, vote_maps[i]))
			idx[n++] = i;
	return n;
}

// proximo mapa de la rotacion del modo: el siguiente al actual en orden alfabetico (o el primero si
// el actual no esta preparado). NULL fuera de FFA / Gun Game o si no hay ningun mapa preparado.
const char *Mode_NextMap (const char *current)
{
	int idx[VOTE_MAX_MAPS], n = Mode_Maps (idx, VOTE_MAX_MAPS), i;

	if (!n)
		return NULL;
	for (i = 0; i < n; i++)
		if (!Q_stricmp (vote_maps[idx[i]], (char *)current))
			return vote_maps[idx[(i + 1) % n]];
	return vote_maps[idx[0]];
}

// el mapa carga y es de D-Day (para la seccion normal de la votacion de fin de mapa)
qboolean Vote_MapPlayable (const char *map)
{
	return Vote_CheckMap (map) == MAP_OK;
}

// patron del archivo que prepara un mapa para un modo cualquiera (no el actual): VMODE_GUNGAME / VMODE_FFA
// con _ffa.ent; NULL para el normal
static const char *Mode_FormatFor (int vmode)
{
	if (vmode == VMODE_GUNGAME || vmode == VMODE_FFA)
		return "ents/%s_ffa.ent";
	return NULL;
}

// mapas del servidor preparados para un modo cualquiera. Lo usa la votacion de fin de mapa por secciones.
int Mode_PreparedMaps (int vmode, const char **out, int max)
{
	const char	*fmt = Mode_FormatFor (vmode);
	int			i, n = 0;

	if (!fmt)
		return 0;
	Vote_LoadMaps ();
	for (i = 0; i < vote_nummaps && n < max; i++)
		if (Mode_FileExists (fmt, vote_maps[i]))
			out[n++] = vote_maps[i];
	return n;
}

// lista "dday2, gg_rats, tebessa2" de los mapas preparados para un modo
static void Mode_MapListFor (int vmode, char *out, int size)
{
	const char	*maps[VOTE_MAX_MAPS];
	int			n = Mode_PreparedMaps (vmode, maps, VOTE_MAX_MAPS), i;

	out[0] = '\0';
	for (i = 0; i < n; i++)
	{
		Vote_Cat (out, i ? ", " : "", size);
		Vote_Cat (out, maps[i], size);
	}
}

/*
==============================================================================
Modos que se votan: vote map / normal / gungame / ffa <mapa>
==============================================================================
*/

static const struct
{
	const char	*word;
	int			vmode;
} vote_mode_words[] =
{
	{"map",		VMODE_NORMAL},
	{"normal",	VMODE_NORMAL},
	{"gungame",	VMODE_GUNGAME},
	{"ffa",		VMODE_FFA},
	{NULL,		0}
};

// VMODE_* del subcomando (map, normal, gungame, ffa); -1 si no es uno de esos
static int Vote_ModeOfWord (const char *word)
{
	int i;

	for (i = 0; vote_mode_words[i].word; i++)
		if (!Q_stricmp ((char *)word, (char *)vote_mode_words[i].word))
			return vote_mode_words[i].vmode;
	return -1;
}

// palabra del comando para un modo especial ("gungame", "ffa")
static const char *Vote_WordOfMode (int vmode)
{
	int i;

	for (i = 2; vote_mode_words[i].word; i++)
		if (vote_mode_words[i].vmode == vmode)
			return vote_mode_words[i].word;
	return "map";
}

// el modo se puede votar: el normal siempre, los otros si estan en vote_modes ("gungame ffa")
static qboolean Vote_ModeAllowed (int vmode)
{
	const char	*word = Vote_WordOfMode (vmode), *p = vote_modes->string;
	char		tok[32];
	int			n;

	if (vmode == VMODE_NORMAL)
		return true;
	while (*p)
	{
		while (*p == ' ' || *p == ',' || *p == '\t')
			p++;
		for (n = 0; *p && *p != ' ' && *p != ',' && *p != '\t'; p++)
			if (n < (int)sizeof(tok) - 1)
				tok[n++] = *p;
		tok[n] = 0;
		if (n && !Q_stricmp (tok, (char *)word))
			return true;
	}
	return false;
}

// " | vote gungame <mapa> | vote ffa <mapa>" con los modos especiales que se pueden votar
static void Vote_ModeUsage (char *out, int size, const char *sep)
{
	int i;

	out[0] = '\0';
	for (i = 2; vote_mode_words[i].word; i++)
		if (Vote_ModeAllowed (vote_mode_words[i].vmode))
		{
			Vote_Cat (out, sep, size);
			Vote_Cat (out, "vote ", size);
			Vote_Cat (out, vote_mode_words[i].word, size);
			Vote_Cat (out, " <mapa>", size);
		}
}

static qboolean Vote_IsTournament (void)
{
	return tournament->value;
}

static void Vote_PrintMaps (edict_t *ent, const char *filter)
{
	char	line[1024];
	int		i, shown = 0, total = 0, vmode, cur;

	Vote_LoadMaps ();
	// vote maps gungame / ffa: los mapas de ese modo
	vmode = Vote_ModeOfWord (filter);
	if (vmode > VMODE_NORMAL)
	{
		Mode_MapListFor (vmode, line, sizeof(line));
		safe_cprintf (ent, PRINT_HIGH, "Mapas de %s: %s.%s\n", MapVote_ModeLabel (vmode), line[0] ? line : "(ninguno)",
			Vote_ModeAllowed (vmode) ? va (" Vota con: vote %s <mapa>", filter) : " Ese modo no se vota en este servidor.");
		return;
	}
	cur = MapVote_CurrentMode ();
	if (cur != VMODE_NORMAL)
	{
		Mode_MapListFor (cur, line, sizeof(line));
		if (Vote_ModeAllowed (cur))
			safe_cprintf (ent, PRINT_HIGH, "En %s se juegan: %s. Para seguir en %s: vote %s <mapa>. "
				"vote map <mapa> vuelve al modo normal.\n", MapVote_ModeLabel (cur), line[0] ? line : "(ninguno)",
				MapVote_ModeLabel (cur), Vote_WordOfMode (cur));
		else
			safe_cprintf (ent, PRINT_HIGH, "En %s se juegan: %s. vote map <mapa> vuelve al modo normal.\n",
				MapVote_ModeLabel (cur), line[0] ? line : "(ninguno)");
		if (!filter[0])
			return;
	}
	if (!filter[0])
	{
		safe_cprintf (ent, PRINT_HIGH, "Hay %d mapas en el servidor. Busca con: vote maps <texto> (ej: vote maps dday)\n",
			vote_nummaps);
		return;
	}

	line[0] = '\0';
	for (i = 0; i < vote_nummaps; i++)
	{
		if (!Vote_Contains (vote_maps[i], filter))
			continue;
		total++;
		if (shown < 60 && strlen (line) + strlen (vote_maps[i]) + 3 < sizeof(line))
		{
			Vote_Cat (line, shown ? ", " : "", sizeof(line));
			Vote_Cat (line, vote_maps[i], sizeof(line));
			shown++;
		}
	}
	if (!total)
	{
		Vote_Suggest (filter, line, sizeof(line));
		safe_cprintf (ent, PRINT_HIGH, "Ningun mapa contiene \"%s\".%s%s\n", filter,
			line[0] ? " Parecidos: " : "", line);
		return;
	}
	safe_cprintf (ent, PRINT_HIGH, "%d mapas con \"%s\": %s%s\n", total, filter, line,
		total > shown ? " ..." : "");
}

/*
==============================================================================
Votacion
==============================================================================
*/

// jugador que puede votar: humano y en un equipo
static qboolean Vote_CanVote (edict_t *ent)
{
	return ent->inuse && ent->client && ent->client->resp.team_on && !ent->ai;
}

static void Vote_Count (int *yes, int *no, int *total)
{
	int		i;
	edict_t	*ent;

	*yes = *no = *total = 0;
	for (i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!Vote_CanVote (ent))
			continue;
		(*total)++;
		if (vote.choice[i] == VOTE_YES)
			(*yes)++;
		else if (vote.choice[i] == VOTE_NO)
			(*no)++;
	}
}

// "si" necesarios para superar el porcentaje de total jugadores
static int Vote_Needed (int total)
{
	int pct = (int)vote_percent->value;

	if (pct < 0)
		pct = 0;
	if (pct > 99)
		pct = 99;
	return (total * pct) / 100 + 1;
}

static const char *Vote_What (void)
{
	static char buf[MAX_QPATH + 64];

	if (vote.type == VTYPE_RESTART)
		Com_sprintf (buf, sizeof(buf), "reiniciar %s", vote.map);
	else if (vote.vmode != VMODE_NORMAL)
		Com_sprintf (buf, sizeof(buf), "cambiar a %s en %s", vote.map, MapVote_ModeLabel (vote.vmode));
	else if (vote.to_normal)
		Com_sprintf (buf, sizeof(buf), "cambiar a %s (vuelve al modo normal)", vote.map);
	else
		Com_sprintf (buf, sizeof(buf), "cambiar a %s", vote.map);
	return buf;
}

// valor de "set <name> <valor>" en el server.cfg (el que ejecuta el server al arrancar); false si no esta
qboolean Cfg_Value (const char *name, char *out, int size);
#define Vote_CfgValue Cfg_Value
qboolean Cfg_Value (const char *name, char *out, int size)
{
	char		line[1024], key[64];
	const char	*p;
	FILE		*f;
	qboolean	found = false;
	int			n;

	f = DDay_OpenFullPathFile (sys_homedir->string, GAMEVERSION, "server.cfg", "r");
	if (!f)
		f = DDay_OpenFullPathFile (sys_basedir->string, GAMEVERSION, "server.cfg", "r");
	if (!f)
		f = DDay_OpenFullPathFile (".", GAMEVERSION, "server.cfg", "r");
	if (!f)
		return false;

	while (fgets (line, sizeof(line), f))
	{
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (Q_strncasecmp ((char *)p, "set ", 4) && Q_strncasecmp ((char *)p, "set\t", 4))
			continue;
		for (p += 4; *p == ' ' || *p == '\t'; p++)
			;
		for (n = 0; *p && *p != ' ' && *p != '\t' && n < (int)sizeof(key) - 1; p++)
			key[n++] = *p;
		key[n] = 0;
		if (Q_stricmp (key, (char *)name))
			continue;
		for (; *p == ' ' || *p == '\t'; p++)
			;
		n = 0;
		if (*p == '"')		// entre comillas: hasta la otra comilla
		{
			for (p++; *p && *p != '"' && n < size - 1; p++)
				out[n++] = *p;
		}
		else				// una palabra (sin el comentario // que sigue)
		{
			for (; *p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && n < size - 1; p++)
			{
				if (p[0] == '/' && p[1] == '/')
					break;
				out[n++] = *p;
			}
		}
		out[n] = 0;
		found = true;		// sigue: vale la ultima linea, como al ejecutarlo
	}
	fclose (f);
	return found;
}

// vote map / vote normal desde FFA / Gun Game: el proximo mapa es normal (MapVote_ApplyMode)
// y vuelven la votacion de fin de mapa y la rotacion del server.cfg, igual que "normal" del bot del chat (al entrar
// al modo, el bot deja mapvoting 0 y en sv_maplist solo el mapa del modo)
static void Vote_ToNormal (void)
{
	char	value[1024];

	MapVote_SetPendingMode (VMODE_NORMAL);
	if (Vote_CfgValue ("mapvoting", value, sizeof(value)) && value[0])
		gi.cvar_set ("mapvoting", value);
	if (Vote_CfgValue ("sv_maplist", value, sizeof(value)) && value[0])
		gi.cvar_set ("sv_maplist", value);
	gi.dprintf ("vote: %s vuelve al modo normal (mapvoting %s, sv_maplist \"%s\")\n", vote.map,
		gi.cvar ("mapvoting", "0", 0)->string, gi.cvar ("sv_maplist", "", 0)->string);
}

// por que no se puede votar ahora (NULL = se puede)
static const char *Vote_Blocked (void)
{
	if (!vote_enable->value)
		return "La votacion esta desactivada en este servidor.";
	if (level.intermissiontime)
		return "No se puede votar durante la intermision.";
	if (level.campaign)
		return "No se puede votar en una campana.";
	return NULL;
}

// mensaje al medio de la pantalla de cada humano (como q2admin). En q2pro un centerprint borra
// las kills de arriba, por eso solo se usa al abrir y al cerrar la votacion, no en los recordatorios
static void Vote_Center (const char *fmt, ...)
{
	char	msg[256];
	va_list	ap;
	int		i;
	edict_t	*ent;

	va_start (ap, fmt);
	vsnprintf (msg, sizeof(msg), fmt, ap);
	va_end (ap);

	for (i = 1; i <= game.maxclients; i++)
	{
		ent = g_edicts + i;
		if (ent->inuse && ent->client && !ent->ai)
			safe_centerprintf (ent, "%s", msg);
	}
}

static void Vote_End (qboolean passed, const char *why)
{
	vote.active = false;

	if (!passed)
	{
		safe_bprintf (PRINT_HIGH, "Votacion rechazada (%s): %s.\n", why, Vote_What ());
		Vote_Center ("Votacion RECHAZADA\n\n%s\n(%s)", Vote_What (), why);
		gi.dprintf ("vote: rechazado %s (propuso %s): %s\n", Vote_What (), vote.caller, why);
		vote.next_allowed = level.time + vote_cooldown->value;
		return;
	}

	safe_bprintf (PRINT_HIGH, "Votacion aprobada: %s.\n", Vote_What ());
	Vote_Center ("Votacion APROBADA\n\n%s", Vote_What ());
	gi.dprintf ("vote: aprobado %s (propuso %s)\n", Vote_What (), vote.caller);

	if (vote.type == VTYPE_MAP)
	{
		if (vote.to_normal)
			Vote_ToNormal ();
		else
			MapVote_SetPendingMode (vote.vmode);	// el mapa votado se juega en ese modo
	}

	// igual que "sv nextmap": la partida queda como cortada y sin votacion final
	disable_mapvoting = true;
	BeginIntermission (CreateTargetChangeLevel (vote.map));
}

// revisa si la votacion ya se decidio
static void Vote_Check (void)
{
	int yes, no, total, needed;

	if (!vote.active)
		return;

	Vote_Count (&yes, &no, &total);
	if (!total)
	{
		Vote_End (false, "no quedan jugadores");
		return;
	}

	needed = Vote_Needed (total);
	if (yes >= needed)
		Vote_End (true, NULL);
	else if (total - no < needed)
		Vote_End (false, "no alcanzan los votos");
}

static void Vote_Status (edict_t *ent)
{
	int yes, no, total;

	Vote_Count (&yes, &no, &total);
	safe_cprintf (ent, PRINT_HIGH, "Votacion de %s: %s. Si %d, No %d, faltan %d si (%d s).\n",
		vote.caller, Vote_What (), yes, no, Vote_Needed (total) - yes,
		(int)(vote.end_time - level.time + 0.9));
}

static void Vote_Help (edict_t *ent)
{
	char modes[128];

	Vote_ModeUsage (modes, sizeof(modes), " | ");
	safe_cprintf (ent, PRINT_HIGH,
		"Uso: vote map <mapa>%s | vote restart | vote yes | vote no\n"
		"\"vote maps <texto>\" busca mapas%s.\n",
		modes, modes[0] ? " (\"vote maps gungame\": los de ese modo)" : "");
}

static void Vote_Start (edict_t *ent, int type)
{
	vote.active = true;
	vote.type = type;
	vote.caller_slot = ent - g_edicts - 1;
	Com_sprintf (vote.caller, sizeof(vote.caller), "%s", ent->client->pers.netname);
	memset (vote.choice, 0, sizeof(vote.choice));
	vote.choice[ent - g_edicts - 1] = VOTE_YES;
	vote.end_time = level.time + (vote_time->value > 5 ? vote_time->value : 5);
	vote.next_remind = level.time + VOTE_REMIND;

	safe_bprintf (PRINT_HIGH, "%s propone %s. Escribe \"vote yes\" o \"vote no\" en la consola.\n",
		vote.caller, Vote_What ());
	gi.dprintf ("vote: %s propone %s\n", vote.caller, Vote_What ());
	Vote_Center ("VOTACION\n\n%s propone\n%s\n\nvote yes  o  vote no\nen la consola (%d s)",
		vote.caller, Vote_What (), (int)(vote.end_time - level.time + 0.5));

	Vote_Check ();	// con un solo jugador se aprueba al tiro
}

// vote map / normal / gungame / ffa <mapa>: el mapa se juega en el modo vmode
static void Vote_StartMap (edict_t *ent, const char *arg, int vmode)
{
	char	sug[256], list[1024];
	int		idx;

	if (!Vote_ModeAllowed (vmode))
	{
		safe_cprintf (ent, PRINT_HIGH, "En este servidor no se vota %s.\n", MapVote_ModeLabel (vmode));
		return;
	}
	if (vmode != VMODE_NORMAL && Vote_IsTournament ())
	{
		safe_cprintf (ent, PRINT_HIGH, "En torneo no se vota %s.\n", MapVote_ModeLabel (vmode));
		return;
	}
	if (!arg[0])
	{
		if (vmode == VMODE_NORMAL)
			safe_cprintf (ent, PRINT_HIGH, "Uso: vote map <mapa>. \"vote maps <texto>\" busca mapas.\n");
		else
		{
			Mode_MapListFor (vmode, list, sizeof(list));
			safe_cprintf (ent, PRINT_HIGH, "Uso: vote %s <mapa>. Mapas de %s: %s.\n", Vote_WordOfMode (vmode),
				MapVote_ModeLabel (vmode), list[0] ? list : "(ninguno)");
		}
		return;
	}
	if (strlen (arg) >= MAX_QPATH || strchr (arg, '/') || strchr (arg, '\\') || strchr (arg, '.')
		|| strchr (arg, '"') || strchr (arg, ';'))
	{
		safe_cprintf (ent, PRINT_HIGH, "Nombre de mapa no valido.\n");
		return;
	}

	idx = Vote_FindMap (arg);
	if (idx < 0)
	{
		Vote_Suggest (arg, sug, sizeof(sug));
		if (sug[0])
			safe_cprintf (ent, PRINT_HIGH, "El mapa \"%s\" no esta en el servidor. Quisiste decir: %s?\n", arg, sug);
		else
			safe_cprintf (ent, PRINT_HIGH, "El mapa \"%s\" no esta en el servidor. \"vote maps <texto>\" busca mapas.\n", arg);
		return;
	}

	switch (Vote_CheckMap (vote_maps[idx]))
	{
	case MAP_NOT_DDAY:
		safe_cprintf (ent, PRINT_HIGH, "\"%s\" esta en el servidor pero no es un mapa de D-Day.\n", vote_maps[idx]);
		return;
	case MAP_BROKEN:
		safe_cprintf (ent, PRINT_HIGH, "El archivo de \"%s\" esta danado, no se puede votar.\n", vote_maps[idx]);
		return;
	}

	// Gun Game / FFA: solo mapas preparados para ese modo
	if (vmode != VMODE_NORMAL && !Mode_FileExists (Mode_FormatFor (vmode), vote_maps[idx]))
	{
		Mode_MapListFor (vmode, list, sizeof(list));
		safe_cprintf (ent, PRINT_HIGH, "\"%s\" no esta preparado para %s. Mapas de %s: %s.\n", vote_maps[idx],
			MapVote_ModeLabel (vmode), MapVote_ModeLabel (vmode), list[0] ? list : "(ninguno)");
		return;
	}

	Com_sprintf (vote.map, sizeof(vote.map), "%s", vote_maps[idx]);	// nombre real del archivo
	vote.vmode = vmode;
	// vote map / vote normal desde FFA / Gun Game: vuelve al modo normal
	vote.to_normal = vmode == VMODE_NORMAL && MapVote_CurrentMode () != VMODE_NORMAL;
	Vote_Start (ent, VTYPE_MAP);
}

void Cmd_Vote_f (edict_t *ent)
{
	char		*arg = gi.argv (1);
	const char	*why;
	int			slot = ent - g_edicts - 1;

	if (!arg[0])
	{
		if (vote.active)
			Vote_Status (ent);
		else
			Vote_Help (ent);
		return;
	}

	if (!Q_stricmp (arg, "maps") || !Q_stricmp (arg, "mapas"))
	{
		Vote_PrintMaps (ent, gi.argv (2));
		return;
	}

	if (!Q_stricmp (arg, "yes") || !Q_stricmp (arg, "si") || !Q_stricmp (arg, "no"))
	{
		if (!vote.active)
		{
			safe_cprintf (ent, PRINT_HIGH, "No hay ninguna votacion en curso.\n");
			return;
		}
		if (!Vote_CanVote (ent))
		{
			safe_cprintf (ent, PRINT_HIGH, "Solo votan los jugadores que estan en un equipo.\n");
			return;
		}
		vote.choice[slot] = Q_stricmp (arg, "no") ? VOTE_YES : VOTE_NO;
		safe_cprintf (ent, PRINT_HIGH, "Votaste %s.\n", vote.choice[slot] == VOTE_YES ? "si" : "no");
		Vote_Check ();
		if (vote.active)
			Vote_Status (ent);
		return;
	}

	if (Vote_ModeOfWord (arg) < 0 && Q_stricmp (arg, "restart"))
	{
		Vote_Help (ent);
		return;
	}

	// abrir una votacion nueva
	if ((why = Vote_Blocked ()) != NULL)
	{
		safe_cprintf (ent, PRINT_HIGH, "%s\n", why);
		return;
	}
	if (vote.active)
	{
		Vote_Status (ent);
		return;
	}
	if (!Vote_CanVote (ent))
	{
		safe_cprintf (ent, PRINT_HIGH, "Tienes que estar en un equipo para proponer una votacion.\n");
		return;
	}
	if (level.time < vote.next_allowed)
	{
		safe_cprintf (ent, PRINT_HIGH, "Espera %d s para proponer otra votacion.\n",
			(int)(vote.next_allowed - level.time + 0.9));
		return;
	}

	if (!Q_stricmp (arg, "restart"))
	{
		Com_sprintf (vote.map, sizeof(vote.map), "%s", level.mapname);
		vote.vmode = VMODE_NORMAL;		// sin cambio de modo (Vote_End no lo usa en restart)
		vote.to_normal = false;
		Vote_Start (ent, VTYPE_RESTART);
	}
	else
		Vote_StartMap (ent, gi.argv (2), Vote_ModeOfWord (arg));
}

// cada frame desde G_RunFrame
void Vote_RunFrame (void)
{
	int yes, no, total;

	Vote_AnnounceFrame ();
	if (!vote.active)
		return;

	if (level.intermissiontime)
	{
		vote.active = false;	// el mapa termino solo
		return;
	}
	if (!vote_enable->value)
	{
		Vote_End (false, "votacion desactivada");
		return;
	}
	// alguien se fue o paso a espectador: la cuenta puede haber cambiado
	Vote_Check ();
	if (!vote.active)
		return;
	if (level.time >= vote.end_time)
	{
		Vote_End (false, "se acabo el tiempo");
		return;
	}
	if (level.time >= vote.next_remind)
	{
		vote.next_remind = level.time + VOTE_REMIND;
		Vote_Count (&yes, &no, &total);
		safe_bprintf (PRINT_HIGH, "Votacion: %s. Si %d, No %d, faltan %d si (%d s). \"vote yes\" o \"vote no\".\n",
			Vote_What (), yes, no, Vote_Needed (total) - yes, (int)(vote.end_time - level.time + 0.9));
	}
}

// el que se va deja de contar; el proximo que use ese slot empieza sin voto
void Vote_ClientDisconnect (edict_t *ent)
{
	int slot = ent - g_edicts - 1;

	if (slot < 0 || slot >= MAX_CLIENTS)
		return;
	vote.choice[slot] = VOTE_NONE;
	howto_told[slot] = false;
	howto_frames[slot] = 0;
}

// sv votecancel
void Svcmd_VoteCancel_f (void)
{
	if (!vote.active)
	{
		safe_cprintf (NULL, PRINT_HIGH, "No hay ninguna votacion en curso.\n");
		return;
	}
	vote.active = false;
	safe_bprintf (PRINT_HIGH, "El admin cancelo la votacion: %s.\n", Vote_What ());
}
