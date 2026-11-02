// p_killfeed.c: graphical killfeed, independent of the game mode.
//
// The deaths are drawn at the top left by the statusbar program itself (CS_STATUSBAR): there are no free stats left,
// so the lines are written straight into the program and the configstring is sent again when the feed changes (a new
// death or an expired line), which is a rare event. The statusbar of the level (team, Free For All...) is given once
// by Killfeed_SetBase and goes after the lines, so the feed does not depend on it.
//
// Each row is a plate (pics/kf_bg_<team>_<size>.png, olive for the team 0, red for the team 1, khaki for no team),
// the weapon icon (pics/kf_<icon>.png, half the size of the icon of the game) and the names. The pictures are made
// by the clients, see pics/README.md.
//
// The statusbar of the engine has a limit (MAX_QPATH * (CS_AIRACCEL - CS_STATUSBAR)): the base is stored with its
// white space collapsed and the oldest lines are dropped when they do not fit.

#include "g_local.h"
#include "x_fire.h"
#include <stdio.h>
#include <string.h>

#define KF_MAX_LINES	4
#define KF_LIFETIME		6.0		// seconds a line stays on the screen
#define KF_NAME_LEN		12		// the names are cut to this length
#define KF_BAR_MAX		(MAX_QPATH * (CS_AIRACCEL - CS_STATUSBAR) - 1)
#define KF_X			2
#define KF_Y			30		// under the lines of the console that the client prints at the top ("You've been hit...")
#define KF_ROW_H		16		// plate (14 pixels) and a gap
#define KF_PAD_L		5		// stripe of the team of the plate
#define KF_PAD_R		4
#define KF_TEXT_DY		3		// text (8 pixels high) centered on the plate
#define KF_CHAR_W		8
#define KF_GAP			6		// between the names and the weapon icon
#define KF_FLAG_W		12		// faction icon (pics/kf_<faction>.png) and the space after it
#define KF_FLAG_GAP		2
#define KF_HS_GAP		4		// between the weapon and the headshot icon
#define KF_PLATE_MIN	48		// the plates are pics/kf_bg_<team>_<width>.png, in steps of KF_PLATE_STEP
#define KF_PLATE_MAX	344
#define KF_PLATE_STEP	8
#define KF_ICON_GENERIC	"skull"	// deaths without killer (suicides, environment) and with no icon of their own

typedef struct
{
	char	*name;		// the picture is pics/kf_<name>.png
	int		width;		// pixels: the statusbar cannot scale or measure them
} kf_icon_t;

// width of the icons of pics/kf_*.png; an icon that is not here is drawn as the generic one
static kf_icon_t kf_icons[] =
{
	{ "w_303", 68 },
	{ "w_303s", 55 },
	{ "w_arisaka", 59 },
	{ "w_arisakas", 59 },
	{ "w_b34", 16 },
	{ "w_b38", 45 },
	{ "w_b3842", 37 },
	{ "w_bar", 48 },
	{ "w_bazooka", 48 },
	{ "w_bhmg", 48 },
	{ "w_binoc", 24 },
	{ "w_breda", 45 },
	{ "w_bren", 31 },
	{ "w_carcano", 81 },
	{ "w_colt45", 24 },
	{ "w_dpm", 57 },
	{ "w_fists", 12 },
	{ "w_flame", 48 },
	{ "w_healthpack", 24 },
	{ "w_helmet", 12 },
	{ "w_k43", 70 },
	{ "w_katana", 54 },
	{ "w_knife", 48 },
	{ "w_m1", 48 },
	{ "w_m1903", 48 },
	{ "w_m1carb", 12 },
	{ "w_m9130", 48 },
	{ "w_m9130s", 48 },
	{ "w_m98k", 48 },
	{ "w_m98ks", 48 },
	{ "w_masher", 24 },
	{ "w_mg34", 47 },
	{ "w_mg42", 48 },
	{ "w_molotov", 13 },
	{ "w_morphine", 12 },
	{ "w_mors", 47 },
	{ "w_mp40", 48 },
	{ "w_mp43", 48 },
	{ "w_nambu", 24 },
	{ "w_p38", 24 },
	{ "w_panzer", 48 },
	{ "w_panzerf", 72 },
	{ "w_piat", 42 },
	{ "w_pps43", 47 },
	{ "w_ppsh41", 40 },
	{ "w_rkm", 48 },
	{ "w_sandbags", 15 },
	{ "w_shotgun", 46 },
	{ "w_sten", 48 },
	{ "w_svt", 48 },
	{ "w_thompson", 48 },
	{ "w_tnt", 24 },
	{ "w_tt33", 14 },
	{ "w_type_100", 50 },
	{ "w_type_99", 28 },
	{ "w_vickers", 46 },
	{ "w_vis", 25 },
	{ "w_webley", 25 },
	{ "skull", 12 },
	{ "plane", 30 },
	{ "nade", 12 },
	{ "boom", 12 },
	{ "tank", 30 },
	{ "hs", 14 }
};

typedef struct
{
	char		killer[KF_NAME_LEN + 1];	// empty: nobody (suicide, environment)
	char		victim[KF_NAME_LEN + 1];
	kf_icon_t	*icon;
	char		*killer_fac;				// faction (pics/kf_<faction>.png), NULL: no icon
	char		*victim_fac;
	qboolean	headshot;
	char		team;						// color of the plate: 'a' team 0, 'x' team 1, 'n' none
	float		expire;
} kf_entry_t;

// factions that have an icon (pics/teams/<faction>.pcx of the game)
static char *kf_factions[] = { "usa", "usm", "gbr", "grm", "ita", "jpn", "pol", "rus" };

static kf_entry_t	kf_entries[KF_MAX_LINES];	// [0] is the oldest, it is drawn on top
static int			kf_count;
static char			kf_base[4096];
static qboolean		kf_base_set;


// copy of the statusbar program without the extra white space (the quoted text is kept as it is)
static void KF_Compact (char *dst, int size, const char *src)
{
	int			n = 0;
	qboolean	quoted = false;
	qboolean	space = true;	// true: the last char is a space, so a new one is not written
	char		c;

	while (*src && n < size - 2)
	{
		c = *src++;

		if (c == '"')
		{
			quoted = !quoted;
			space = false;
			dst[n++] = c;
		}
		else if (!quoted && (c == ' ' || c == '\t' || c == '\r' || c == '\n'))
		{
			if (!space)
			{
				dst[n++] = ' ';
				space = true;
			}
		}
		else
		{
			space = false;
			dst[n++] = c;
		}
	}

	if (n > 0 && dst[n - 1] != ' ')
		dst[n++] = ' ';
	dst[n] = 0;
}


// the name goes inside quotes in the program: no quotes or control chars
static void KF_CopyName (char *dst, const char *name)
{
	int		n = 0;

	while (*name && n < KF_NAME_LEN)
	{
		if (*name != '"' && (unsigned char)*name >= 32)
			dst[n++] = *name;
		name++;
	}
	dst[n] = 0;
}


static kf_icon_t *KF_FindIcon (char *name)
{
	int		i;

	for (i = 0; i < sizeof (kf_icons) / sizeof (kf_icons[0]); i++)
		if (!Q_stricmp (kf_icons[i].name, name))
			return &kf_icons[i];

	return NULL;
}


// the weapon in hand of the killer for the deaths that come from it, a fixed icon for the weapons that are
// not held when they kill (fire, rockets, TNT, bottles, grenades, airstrikes...) and the skull for the rest
static kf_icon_t *KF_ChooseIcon (edict_t *killer, edict_t *victim, int mod)
{
	kf_icon_t	*icon = NULL;
	char		*held = NULL;

	// suicides and the environment: always the skull
	if (!killer || killer == victim || !killer->client)
		return KF_FindIcon (KF_ICON_GENERIC);

	if (killer->client->pers.weapon)
		held = killer->client->pers.weapon->icon;

	switch (mod)
	{
	case MOD_PISTOL:
	case MOD_SHOTGUN:
	case MOD_SHOTGUN2:
	case MOD_RIFLE:
	case MOD_LMG:
	case MOD_HMG:
	case MOD_SUBMG:
	case MOD_SNIPER:
	case MOD_KNIFE:
	case MOD_FISTS:
	case MOD_BAYONET:
		if (held)
			icon = KF_FindIcon (held);
		break;

	case MOD_ROCKET:
	case MOD_R_SPLASH:
		// the weapon in hand can be another one by the time the rocket explodes
		if (held && (!Q_stricmp (held, "w_bazooka") || !Q_stricmp (held, "w_panzer") ||
			!Q_stricmp (held, "w_panzerf") || !Q_stricmp (held, "w_piat")))
			icon = KF_FindIcon (held);
		else
			icon = KF_FindIcon ("w_bazooka");
		break;

	case MOD_FIRE_SPLASH:
	case MOD_ON_FIRE:
	case MOD_FIREBALL:
		icon = KF_FindIcon ("w_flame");
		break;

	case MOD_BOTTLE:
		icon = KF_FindIcon ("w_molotov");
		break;

	case MOD_TNT:
	case MOD_TNT1:
	case MOD_HELD_TNT:
	case MOD_TNT_SPLASH:
	case MOD_TNT1_SPLASH:
		icon = KF_FindIcon ("w_tnt");
		break;

	case MOD_GRENADE:
	case MOD_G_SPLASH:
	case MOD_HANDGRENADE:
	case MOD_HG_SPLASH:
	case MOD_HELD_GRENADE:
		icon = KF_FindIcon ("nade");
		break;

	case MOD_AIRSTRIKE:
	case MOD_AIRSTRIKE_SPLASH:
		icon = KF_FindIcon ("plane");
		break;

	case MOD_EXPLOSIVE:
	case MOD_BARREL:
	case MOD_BOMB:
		icon = KF_FindIcon ("boom");
		break;

	case MOD_TANKHIT:
		icon = KF_FindIcon ("tank");
		break;

	case MOD_HELMET:
		icon = KF_FindIcon ("w_helmet");
		break;
	}

	if (!icon)
		icon = KF_FindIcon (KF_ICON_GENERIC);
	return icon;
}


// color of the plate: the team of the killer (of the victim when nobody killed him); Free For All has no teams
static char KF_ChooseTeam (edict_t *killer, edict_t *victim)
{
	edict_t		*who = victim;

	if (G_IsFFA ())
		return 'n';

	if (killer && killer != victim && killer->client)
		who = killer;

	if (who->client && who->client->resp.team_on)
	{
		if (who->client->resp.team_on->index == 0)
			return 'a';
		if (who->client->resp.team_on->index == 1)
			return 'x';
	}
	return 'n';
}


// faction icon of the player; none in Free For All (everybody is on the same team) or when the faction has no icon
static char *KF_ChooseFaction (edict_t *ent)
{
	int		i;

	if (!ent || !ent->client || !ent->client->resp.team_on || G_IsFFA ())
		return NULL;

	for (i = 0; i < sizeof (kf_factions) / sizeof (kf_factions[0]); i++)
		if (!strcmp (ent->client->resp.team_on->teamid, kf_factions[i]))
			return kf_factions[i];

	return NULL;
}


// one row: plate, [faction killer] weapon [headshot] [faction] victim. The plate is the width of the row (in steps
// of KF_PLATE_STEP) and the x of everything follows the length of the names
static void KF_Line (char *out, int size, kf_entry_t *e, int row)
{
	int		y = KF_Y + row * KF_ROW_H;
	int		ty = y + KF_TEXT_DY;
	int		x = KF_X + KF_PAD_L;
	int		kfx = 0, ktx = 0, wx, hx = 0, vfx = 0, vtx, width;
	char	*p = out;

	if (e->killer[0])
	{
		if (e->killer_fac)
		{
			kfx = x;
			x += KF_FLAG_W + KF_FLAG_GAP;
		}
		ktx = x;
		x += strlen (e->killer) * KF_CHAR_W + KF_GAP;
	}

	wx = x;
	x += e->icon->width;

	if (e->headshot)
	{
		x += KF_HS_GAP;
		hx = x;
		x += KF_FindIcon ("hs")->width;
	}

	x += KF_GAP;
	if (e->victim_fac)
	{
		vfx = x;
		x += KF_FLAG_W + KF_FLAG_GAP;
	}
	vtx = x;
	x += strlen (e->victim) * KF_CHAR_W;

	width = x + KF_PAD_R - KF_X;
	width = (width + KF_PLATE_STEP - 1) / KF_PLATE_STEP * KF_PLATE_STEP;
	if (width < KF_PLATE_MIN)
		width = KF_PLATE_MIN;
	if (width > KF_PLATE_MAX)
		width = KF_PLATE_MAX;

	p += sprintf (p, "yt %d xl %d picn kf_bg_%c_%d ", y, KF_X, e->team, width);
	if (kfx)
		p += sprintf (p, "xl %d picn kf_%s ", kfx, e->killer_fac);
	p += sprintf (p, "xl %d picn kf_%s ", wx, e->icon->name);
	if (hx)
		p += sprintf (p, "xl %d picn kf_hs ", hx);
	if (vfx)
		p += sprintf (p, "xl %d picn kf_%s ", vfx, e->victim_fac);

	p += sprintf (p, "yt %d ", ty);
	if (ktx)
		p += sprintf (p, "xl %d string \"%s\" ", ktx, e->killer);
	sprintf (p, "xl %d string \"%s\" ", vtx, e->victim);
}


static void KF_DropOldest (void)
{
	kf_count--;
	memmove (&kf_entries[0], &kf_entries[1], kf_count * sizeof (kf_entry_t));
}


static void KF_Send (void)
{
	static char	bar[KF_BAR_MAX + 1];
	char		line[256];
	int			i, len, baselen;

	if (!kf_base_set)
		return;

	baselen = strlen (kf_base);

	// the lines go first and the base keeps its place: drop the oldest lines until everything fits
	for (;;)
	{
		len = baselen;
		for (i = 0; i < kf_count; i++)
		{
			KF_Line (line, sizeof (line), &kf_entries[i], i);
			len += strlen (line);
		}
		if (len <= KF_BAR_MAX || kf_count == 0)
			break;
		KF_DropOldest ();
	}

	if (kf_count == 0)
	{
		gi.configstring (CS_STATUSBAR, kf_base);
		return;
	}

	bar[0] = 0;
	for (i = 0; i < kf_count; i++)
	{
		KF_Line (line, sizeof (line), &kf_entries[i], i);
		strcat (bar, line);
	}
	strcat (bar, kf_base);
	gi.configstring (CS_STATUSBAR, bar);
}


// sets the statusbar of the level (once per map) and clears the feed
void Killfeed_SetBase (char *statusbar)
{
	KF_Compact (kf_base, sizeof (kf_base), statusbar);
	kf_base_set = true;
	kf_count = 0;
	KF_Send ();
}


// a death: killer is NULL (or not a client) for suicides and the environment
// returns true when the death is on the feed and its text obituary must not be printed to the clients: with
// killfeed_console 1 it returns false and the console keeps the history of the deaths (0 by default: the client
// cannot keep the text out of the notify area, it would show up on top of the feed)
qboolean Killfeed_Add (edict_t *killer, edict_t *victim, int mod, qboolean headshot)
{
	kf_entry_t	*e;

	if (!victim || !victim->client)
		return false;

	// same visibility as the console obituaries: with death_msg 0 and 2 they are not for everybody
	if (death_msg->value != 1 && death_msg->value != 3)
		return false;

	if (nohud->value)
		return false;

	if (kf_count == KF_MAX_LINES)
		KF_DropOldest ();

	e = &kf_entries[kf_count++];
	KF_CopyName (e->victim, victim->client->pers.netname);
	if (killer && killer->client && killer != victim)
		KF_CopyName (e->killer, killer->client->pers.netname);
	else
		e->killer[0] = 0;
	e->icon = KF_ChooseIcon (killer, victim, mod);
	e->killer_fac = e->killer[0] ? KF_ChooseFaction (killer) : NULL;
	e->victim_fac = KF_ChooseFaction (victim);
	e->headshot = headshot;	e->team = KF_ChooseTeam (killer, victim);
	e->expire = level.time + KF_LIFETIME;

	KF_Send ();
	return !killfeed_console->value;
}


// once per server frame: removes the lines that expired
void Killfeed_Frame (void)
{
	if (!kf_count || level.time < kf_entries[0].expire)
		return;

	while (kf_count && level.time >= kf_entries[0].expire)
		KF_DropOldest ();

	KF_Send ();
}
