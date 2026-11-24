// g_tornado.c: tornado de broma (remolino de arena) que anda por el mapa, levanta a los
// jugadores, los hace girar y los suelta lejos. La caida no hace dano (P_FallingDamage).
//
// Apagado por defecto: tornado 0. Con tornado 1 aparece solo cada tornado_interval segundos
// en los mapas de tornado_maps; con tornado_gungame 1, lo mismo pero solo en el Gun Game.
// Recorre el mapa por los nodos de los bots; en mapas sin nodos va de un punto de aparicion a
// otro. "sv tornado [id|off]" lo hace aparecer (o lo saca) a mano.
//
// Solo usa cosas que ya trae el pak0 de dday (sprites/null.sp2, debris1/2) y sound/wind2x.wav,
// asi que no hay que descargar nada.

#include "g_local.h"
#include "ai/ai_nodes_shared.h"	// NODE_ALL, LINK_*: el tornado usa los nodos de los bots

#define TORNADO_LEVELS		7	// alturas del embudo
#define TORNADO_PER_LEVEL	5	// nubes de polvo por altura
#define TORNADO_DEBRIS		8	// escombros que giran adentro
#define TORNADO_MAX			2	// tornados a la vez
#define TORNADO_HOLD		5.0	// segundos maximos de subida (si un techo no lo deja llegar arriba, gira ahi)
#define TORNADO_SPIN		450	// velocidad del giro
#define TORNADO_COOLDOWN	3.0	// segundos sin poder volver a agarrarlo
#define TORNADO_SAFE		15.0	// tope del seguro contra la caida si nunca aterriza
#define TORNADO_SOUND_TIME	2.7	// largo de wind2x.wav

#define TORNADO_SOUND		"wind2x.wav"
#define TORNADO_RAD(a)		((a) * (M_PI / 180.0))

static void Tornado_Think (edict_t *self);

// ruta de cada tornado por los nodos de los bots (self->style = indice)
static astarpath_t tornado_path[TORNADO_MAX];
#define TORNADO_LINKS	(LINK_MOVE | LINK_STAIRS | LINK_FALL | LINK_JUMP | LINK_CROUCH)

static qboolean Tornado_IsTornado (edict_t *e)
{
	return e && e->inuse && e->think == Tornado_Think;
}

static int Tornado_Count (void)
{
	edict_t *e;
	int i, n = 0;

	for (i = (int)maxclients->value + 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
		if (Tornado_IsTornado (e))
			n++;
	return n;
}

// se puede levantar a este jugador?
static qboolean Tornado_CanLift (edict_t *p)
{
	gclient_t *cl = p->client;

	if (!p->inuse || !cl || !cl->resp.team_on)
		return false;
	if (p->health <= 0 || p->deadflag || p->s.modelindex != 255)
		return false;
	if (p->movetype != MOVETYPE_WALK || p->solid == SOLID_NOT)
		return false;
	if (cl->turret || cl->limbo_mode)
		return false;
	return true;
}

// piso bajo pos (mas o menos 64 arriba, 384 abajo), sin agua; con needsky, ademas al aire libre.
// ground: punto del piso; top: z hasta donde llega el embudo (bajo techo, hasta el techo)
static qboolean Tornado_FindGround (vec3_t pos, vec3_t ground, float *top, qboolean needsky)
{
	vec3_t start, end, mins = {-8, -8, 0}, maxs = {8, 8, 8};
	trace_t tr, up;
	float t;

	VectorCopy (pos, start);
	start[2] += 64;
	VectorCopy (pos, end);
	end[2] -= 384;
	tr = gi.trace (start, mins, maxs, end, NULL, MASK_SOLID);
	if (tr.startsolid || tr.allsolid || tr.fraction == 1.0)
		return false;

	VectorCopy (tr.endpos, ground);

	VectorCopy (ground, start);
	start[2] += 8;
	if (gi.pointcontents (start) & MASK_WATER)
		return false;

	// tiene que estar al aire libre: lo primero que hay arriba es cielo
	start[2] += 8;
	VectorCopy (start, end);
	end[2] += 2048;
	up = gi.trace (start, NULL, NULL, end, NULL, MASK_SOLID);
	if (needsky && up.fraction < 1.0 && !(up.surface && (up.surface->flags & SURF_SKY)))
		return false;

	t = ground[2] + tornado_height->value;
	if (t > up.endpos[2] - 48)
		t = up.endpos[2] - 48;
	if (t - ground[2] < (needsky ? 128 : 48))
		return false;

	*top = t;
	return true;
}

// suelta al jugador. fling: lanzarlo hacia afuera y arriba
static void Tornado_Drop (edict_t *self, edict_t *p, qboolean fling)
{
	gclient_t *cl = p->client;
	vec3_t dir;
	float f = tornado_fling->value;

	cl->tornado = NULL;
	cl->tornado_top_time = 0;
	cl->tornado_free_time = level.time + TORNADO_COOLDOWN;
	cl->tornado_safe_until = level.time + TORNADO_SAFE;	// seguro hasta que aterrice

	if (!fling)
		return;

	VectorSubtract (p->s.origin, self->s.origin, dir);
	dir[2] = 0;
	if (VectorNormalize (dir) < 1)
		VectorSet (dir, 1, 0, 0);

	p->velocity[0] = dir[0] * f + self->move_origin[0];
	p->velocity[1] = dir[1] * f + self->move_origin[1];
	p->velocity[2] = f * 0.6;
	p->groundentity = NULL;
}

// saca el tornado y sus partes, y suelta a los que llevaba
static void Tornado_Remove (edict_t *self)
{
	edict_t *e;
	int i;

	for (i = 1; i <= maxclients->value; i++)
	{
		e = g_edicts + i;
		if (e->inuse && e->client && e->client->tornado == self)
			Tornado_Drop (self, e, true);
	}

	for (i = (int)maxclients->value + 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
		if (e->inuse && e->owner == self && e->classname && !strcmp (e->classname, "tornado_part"))
			G_FreeEdict (e);

	G_FreeEdict (self);
}

// partes: nubes de polvo (sprite invisible con estela de humo) y escombros que giran en un
// embudo que se abre hacia arriba
static void TornadoPart_Think (edict_t *self)
{
	edict_t *t = self->owner;
	float h, hf, r, ang, t0 = level.time;

	if (!Tornado_IsTornado (t))
	{
		G_FreeEdict (self);
		return;
	}

	h = t->pos2[2] - t->s.origin[2];	// alto del embudo

	if (self->count < 0)	// escombro: sube y baja
		hf = 0.5 + 0.38 * sin (t0 * self->wait + self->random);
	else
		hf = (self->count + 0.5) / TORNADO_LEVELS;

	r = 16 + tornado_radius->value * 1.1 * hf;
	ang = TORNADO_RAD (self->random * 57.3 + self->speed * t0);

	self->s.origin[0] = t->s.origin[0] + cos (ang) * r + sin (t0 * 0.9 + hf * 2.5) * 30 * hf;
	self->s.origin[1] = t->s.origin[1] + sin (ang) * r + cos (t0 * 0.7 + hf * 2.0) * 30 * hf;
	self->s.origin[2] = t->s.origin[2] + 8 + h * hf;

	if (self->count < 0)
	{
		self->s.angles[0] = anglemod (self->s.angles[0] + 37);
		self->s.angles[1] = anglemod (self->s.angles[1] + 23);
	}

	gi.linkentity (self);
	self->nextthink = level.time + FRAMETIME;
}

static void Tornado_AddPart (edict_t *t, int count, float phase, float speed, char *model)
{
	edict_t *p = G_Spawn ();

	p->classname = "tornado_part";
	p->owner = t;
	p->count = count;
	p->random = phase;
	p->speed = speed;
	p->wait = 0.6 + random () * 0.8;
	p->movetype = MOVETYPE_NONE;
	p->solid = SOLID_NOT;
	p->takedamage = DAMAGE_NO;
	p->clipmask = 0;
	p->s.modelindex = gi.modelindex (model);
	p->s.effects = EF_GRENADE;	// estela de humo/polvo
	VectorCopy (t->s.origin, p->s.origin);
	p->think = TornadoPart_Think;
	p->nextthink = level.time + FRAMETIME;
	TornadoPart_Think (p);
}

// polvo que levanta en la base
static void Tornado_Dust (edict_t *self)
{
	vec3_t pos, dir;
	float ang = random () * 2 * M_PI;
	float r = tornado_radius->value * (0.3 + random () * 0.5);

	pos[0] = self->s.origin[0] + cos (ang) * r;
	pos[1] = self->s.origin[1] + sin (ang) * r;
	pos[2] = self->s.origin[2] + 6;
	VectorSet (dir, -sin (ang), cos (ang), 1.2);
	VectorNormalize (dir);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (24);
	gi.WritePosition (pos);
	gi.WriteDir (dir);
	gi.WriteByte (SPLASH_BROWN_WATER);
	gi.multicast (pos, MULTICAST_PVS);
}

static void Tornado_SetPos (edict_t *self, vec3_t ground, float top)
{
	vec3_t old;

	VectorCopy (self->s.origin, old);
	VectorCopy (ground, self->s.origin);
	self->pos2[2] = top;
	VectorSubtract (self->s.origin, old, self->move_origin);
	VectorScale (self->move_origin, 1.0 / FRAMETIME, self->move_origin);
	self->move_origin[2] = 0;
	gi.linkentity (self);
}

// hay cielo sobre el nodo?
static qboolean Tornado_NodeOutdoors (int n)
{
	vec3_t pos, ground;
	float top;

	AI_NodeOrigin (n, pos);
	return Tornado_FindGround (pos, ground, &top, true);
}

// nodo de los bots mas cercano a pos
static int Tornado_NodeNear (vec3_t pos, edict_t *pass)
{
	vec3_t p;

	VectorCopy (pos, p);
	p[2] += 24;
	return AI_FindClosestReachableNode (p, pass, NODE_DENSITY * 4, NODE_ALL);
}

// ruta nueva por los nodos de los bots hacia un punto lejano al aire libre (a veces donde
// hay un jugador al azar). false si el mapa no tiene nodos o no hay ruta
static qboolean Tornado_NewRoute (edict_t *self)
{
	astarpath_t *path = &tornado_path[self->style];
	edict_t *p;
	vec3_t v, pos;
	int n = AI_NumNodes (), start, goal, tries, i, k;

	path->numNodes = -1;
	if (n < 2)
		return false;

	start = Tornado_NodeNear (self->s.origin, self);
	if (start < 0)
		return false;

	for (tries = 0; tries < 30; tries++)
	{
		goal = -1;
		if (tries < 10 && random () < 0.3)
		{
			k = rand () % (int)maxclients->value;
			for (i = 0; i < maxclients->value && goal < 0; i++)
			{
				p = g_edicts + 1 + (k + i) % (int)maxclients->value;
				if (Tornado_CanLift (p))
					goal = Tornado_NodeNear (p->s.origin, p);
			}
		}
		if (goal < 0)
			goal = rand () % n;
		if (goal == start)
			continue;

		AI_NodeOrigin (goal, pos);
		VectorSubtract (pos, self->s.origin, v);
		v[2] = 0;
		if (tries < 20 && (VectorLength (v) < 800 || !Tornado_NodeOutdoors (goal)))
			continue;

		if (AStar_GetPath (start, goal, TORNADO_LINKS, path) && path->numNodes >= 0)
		{
			self->teleport_time = 0;	// plazo para llegar al proximo nodo
			return true;
		}
	}

	path->numNodes = -1;
	return false;
}

// sigue la ruta: va derecho al proximo nodo (los nodos enlazados se pueden caminar) pegado al
// piso, aunque pase bajo un techo. false si no hay ruta o se trabo
static qboolean Tornado_FollowRoute (edict_t *self)
{
	astarpath_t *path = &tornado_path[self->style];
	vec3_t target, dir, dest, ground;
	float d, top, step = tornado_speed->value * FRAMETIME;

	if (path->numNodes < 0)
	{
		if (level.time < self->delay)	// no hubo ruta hace poco: no recalcular cada frame
			return false;
		if (!Tornado_NewRoute (self))
		{
			self->delay = level.time + 3;
			return false;
		}
	}

	AI_NodeOrigin (path->nodes[path->numNodes], target);
	VectorSubtract (target, self->s.origin, dir);
	dir[2] = 0;
	d = VectorNormalize (dir);

	if (d < 32)	// llego al nodo: el siguiente, o una ruta nueva al terminar
	{
		if (path->numNodes == 0)
			return Tornado_NewRoute (self);
		path->numNodes--;
		self->teleport_time = 0;
		return true;
	}

	if (!self->teleport_time)
		self->teleport_time = level.time + 2 + 2 * d / (step / FRAMETIME);
	else if (level.time > self->teleport_time)	// no llega: trabado
		return Tornado_NewRoute (self);

	if (step > d)
		step = d;
	VectorMA (self->s.origin, step, dir, dest);
	dest[2] = (self->s.origin[2] > target[2] ? self->s.origin[2] : target[2]);
	if (!Tornado_FindGround (dest, ground, &top, false))
		return Tornado_NewRoute (self);

	Tornado_SetPos (self, ground, top);
	return true;
}

// destino para mapas sin nodos: cerca de un punto de aparicion (en FFA / Gun Game son los del
// _ffa.ent, repartidos por todo el mapa) o de un jugador al azar, al aire libre y lejos
static qboolean Tornado_PickGoal (edict_t *self)
{
	static char *spawns[] = {"info_reinforcements_start", "info_player_start", "info_player_deathmatch", "info_team_start"};
	edict_t *list[256], *e;
	vec3_t start, end, dir, ground, v;
	trace_t tr;
	float top, yaw;
	int n = 0, i, tries;

	for (i = 0; i < 4; i++)
		for (e = NULL; n < 256 && (e = G_Find (e, FOFS (classname), spawns[i])) != NULL; )
			list[n++] = e;
	for (i = 1; i <= maxclients->value && n < 256; i++)
		if (Tornado_CanLift (g_edicts + i))
			list[n++] = g_edicts + i;
	if (!n)
		return false;

	for (tries = 0; tries < 30; tries++)
	{
		e = list[rand () % n];
		yaw = random () * 2 * M_PI;
		VectorSet (dir, cos (yaw), sin (yaw), 0);
		VectorCopy (e->s.origin, start);
		start[2] += 16;
		VectorMA (start, random () * 400, dir, end);
		tr = gi.trace (start, NULL, NULL, end, e, MASK_SOLID);
		VectorMA (tr.endpos, -32, dir, end);
		if (!Tornado_FindGround (end, ground, &top, true))
			continue;
		VectorSubtract (ground, self->s.origin, v);
		v[2] = 0;
		if (tries < 25 && (VectorLength (v) < 700 || VectorLength (v) > 2000))
			continue;

		VectorCopy (ground, self->pos1);
		self->air_finished = level.time + 4 + 2.5 * VectorLength (v) / tornado_speed->value;
		self->dmg_radius = VectorLength (v);			// mejor distancia lograda
		self->touch_debounce_time = level.time + 5;	// plazo para acercarse 64 mas
		return true;
	}
	return false;
}

// prueba un paso con rumbo yaw: sin pared, con piso y sin desnivel grande
static qboolean Tornado_TryStep (edict_t *self, float yaw, float step)
{
	vec3_t dir, dest, start, end, ground;
	vec3_t mins = {-16, -16, -16}, maxs = {16, 16, 16};
	float top;
	trace_t tr;

	VectorSet (dir, cos (TORNADO_RAD (yaw)), sin (TORNADO_RAD (yaw)), 0);
	VectorMA (self->s.origin, step, dir, dest);

	VectorCopy (self->s.origin, start);
	start[2] += 32;
	VectorCopy (dest, end);
	end[2] += 32;
	tr = gi.trace (start, mins, maxs, end, self, MASK_SOLID);
	if (tr.fraction < 1.0 || !Tornado_FindGround (dest, ground, &top, false) ||
		fabs (ground[2] - self->s.origin[2]) > 24)
		return false;

	self->ideal_yaw = yaw;
	Tornado_SetPos (self, ground, top);
	return true;
}

// sin nodos: va hacia un destino; si una pared lo tapa la bordea siguiendo su rumbo y, si no
// puede, probando rumbos hacia un mismo lado. Si en 5 s no se acerca, otro destino
static void Tornado_Seek (edict_t *self)
{
	static float offs[] = {35, 70, 105, 140, 175};
	vec3_t v;
	float d, yaw, diff, step = tornado_speed->value * FRAMETIME;
	int i, side;

	VectorSubtract (self->pos1, self->s.origin, v);
	v[2] = 0;
	d = VectorLength (v);
	if (self->air_finished && d < self->dmg_radius - 64)
	{
		self->dmg_radius = d;
		self->touch_debounce_time = level.time + 5;
	}
	if (!self->air_finished || d < 64 || level.time > self->air_finished ||
		level.time > self->touch_debounce_time)
	{
		if (!Tornado_PickGoal (self))
		{
			self->air_finished = 0;
			Tornado_TryStep (self, self->ideal_yaw + crandom () * 40, step);	// sin destino: deambula
			return;
		}
		VectorSubtract (self->pos1, self->s.origin, v);
		v[2] = 0;
	}

	if (!self->count)
		self->count = (random () < 0.5) ? 1 : -1;
	yaw = vectoyaw (v);

	// derecho al destino
	if (Tornado_TryStep (self, yaw, step))
		return;

	// bordeando: seguir con el rumbo de antes mientras no se aleje demasiado del destino
	diff = anglemod (self->ideal_yaw - yaw + 180) - 180;
	if (fabs (diff) < 150 && Tornado_TryStep (self, self->ideal_yaw, step))
		return;

	// rumbos cada vez mas abiertos, primero hacia el lado de siempre
	for (side = 0; side < 2; side++)
		for (i = 0; i < 5; i++)
			if (Tornado_TryStep (self, anglemod (yaw + offs[i] * (side ? -self->count : self->count)), step))
			{
				if (side)	// solo se pudo por el otro lado: bordear por ahi
					self->count = -self->count;
				return;
			}

	self->air_finished = 0;	// encerrado: otro destino
}

static void Tornado_Move (edict_t *self)
{
	VectorClear (self->move_origin);
	if (!Tornado_FollowRoute (self))
		Tornado_Seek (self);
}

// levanta, hace girar y suelta a los jugadores que agarra
static void Tornado_Lift (edict_t *self)
{
	edict_t *p;
	gclient_t *cl;
	float R = tornado_radius->value, d, rz, vrad, ux, uy;
	float spin = TORNADO_SPIN, lift = tornado_lift->value, vz;
	float spintime = tornado_spins->value * 2 * M_PI * (R * 0.45) / TORNADO_SPIN;	// segundos de las vueltas arriba
	int i;

	for (i = 1; i <= maxclients->value; i++)
	{
		p = g_edicts + i;
		if (!p->inuse || !(cl = p->client))
			continue;

		if (!Tornado_CanLift (p))
		{
			if (cl->tornado == self)	// murio, se fue a una torreta, etc.
				Tornado_Drop (self, p, false);
			continue;
		}

		ux = p->s.origin[0] - self->s.origin[0];
		uy = p->s.origin[1] - self->s.origin[1];
		d = sqrt (ux * ux + uy * uy);
		rz = p->s.origin[2] - self->s.origin[2];

		if (cl->tornado != self)
		{
			if (cl->tornado || level.time < cl->tornado_free_time)
				continue;
			if (d > R || rz < -48 || p->s.origin[2] > self->pos2[2])
				continue;

			cl->tornado = self;
			cl->tornado_catch_time = level.time;
			cl->tornado_top_time = 0;
			safe_centerprintf (p, "Te agarro el tornado!\n");
		}

		cl->tornado_safe_until = level.time + TORNADO_SAFE;

		// si se escapo (pared), solo lo suelta
		if (d > R * 1.6)
		{
			Tornado_Drop (self, p, false);
			continue;
		}

		// arriba (o si no llega, a los TORNADO_HOLD s) da tornado_spins vueltas y sale volando
		if (!cl->tornado_top_time &&
			(p->s.origin[2] >= self->pos2[2] - 48 || level.time - cl->tornado_catch_time > TORNADO_HOLD))
			cl->tornado_top_time = level.time;
		if (cl->tornado_top_time && level.time - cl->tornado_top_time >= spintime)
		{
			Tornado_Drop (self, p, true);
			continue;
		}

		if (d < 1)
		{
			ux = 1;
			uy = 0;
			d = 1;
		}
		ux /= d;
		uy /= d;

		// girar (antihorario, como el embudo), acercarse a un anillo interior y subir
		vrad = (R * 0.45 - d) * 4;
		if (vrad > 250)
			vrad = 250;
		else if (vrad < -250)
			vrad = -250;

		p->velocity[0] = -uy * spin + ux * vrad + self->move_origin[0];
		p->velocity[1] = ux * spin + uy * vrad + self->move_origin[1];
		if (cl->tornado_top_time)	// flota cerca de arriba mientras gira
		{
			vz = (self->pos2[2] - 40 - p->s.origin[2]) * 4;
			if (vz > 200)
				vz = 200;
			else if (vz < -200)
				vz = -200;
			p->velocity[2] = vz;
		}
		else
			p->velocity[2] = lift;
		p->groundentity = NULL;
	}
}

static void Tornado_Think (edict_t *self)
{
	if (level.intermissiontime || level.time >= self->timestamp)
	{
		Tornado_Remove (self);
		return;
	}

	Tornado_Move (self);
	Tornado_Lift (self);

	if (level.framenum % 2 == 0)
		Tornado_Dust (self);

	if (level.time >= self->wait)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex (TORNADO_SOUND), 1, ATTN_NORM, 0);
		self->wait = level.time + TORNADO_SOUND_TIME;
	}

	self->nextthink = level.time + FRAMETIME;
}

// ningun jugador encima de ground (que no lo agarre al aparecer)
static qboolean Tornado_FarFromPlayers (vec3_t ground)
{
	edict_t *p;
	vec3_t v;
	int i;

	for (i = 1; i <= maxclients->value; i++)
	{
		p = g_edicts + i;
		if (!Tornado_CanLift (p))
			continue;
		VectorSubtract (p->s.origin, ground, v);
		if (VectorLength (v) < tornado_radius->value + 96)
			return false;
	}
	return true;
}

// un lugar al azar del mapa al aire libre (sobre un nodo de los bots), lejos de los jugadores.
// false si el mapa no tiene nodos o no hubo suerte
static qboolean Tornado_PickAnywhere (vec3_t spot, float *top)
{
	vec3_t pos, ground;
	int n = AI_NumNodes (), tries;

	if (n < 1)
		return false;

	for (tries = 0; tries < 40; tries++)
	{
		AI_NodeOrigin (rand () % n, pos);
		if (!Tornado_FindGround (pos, ground, top, true) || !Tornado_FarFromPlayers (ground))
			continue;
		VectorCopy (ground, spot);
		return true;
	}
	return false;
}

// busca un lugar al aire libre a 350-1000 de anchor, lejos de los jugadores
static qboolean Tornado_PickSpot (edict_t *anchor, vec3_t spot, float *top)
{
	vec3_t start, end, dir, cand, ground;
	trace_t tr;
	float yaw, dist;
	int tries;

	for (tries = 0; tries < 40; tries++)
	{
		yaw = random () * 2 * M_PI;
		dist = 350 + random () * 650;
		VectorSet (dir, cos (yaw), sin (yaw), 0);

		VectorCopy (anchor->s.origin, start);
		start[2] += 16;
		VectorMA (start, dist, dir, end);
		tr = gi.trace (start, NULL, NULL, end, anchor, MASK_SOLID);
		VectorMA (tr.endpos, -32, dir, cand);

		if (!Tornado_FindGround (cand, ground, top, true) || !Tornado_FarFromPlayers (ground))
			continue;

		VectorCopy (ground, spot);
		return true;
	}
	return false;
}

// hace aparecer un tornado cerca de anchor, o si es NULL en cualquier lado del mapa (en mapas sin
// nodos, cerca de un jugador al azar)
edict_t *Tornado_Spawn (edict_t *anchor)
{
	edict_t *t, *p, *e, *list[MAX_CLIENTS];
	vec3_t spot;
	float top;
	int i, n = 0, start, slot;

	if (Tornado_Count () >= TORNADO_MAX)
		return NULL;

	for (i = 1; i <= maxclients->value && n < MAX_CLIENTS; i++)
	{
		p = g_edicts + i;
		if (Tornado_CanLift (p))
			list[n++] = p;
	}

	if (anchor)
	{
		if (!Tornado_PickSpot (anchor, spot, &top))
			return NULL;
	}
	else if (!n)
		return NULL;
	else if (!Tornado_PickAnywhere (spot, &top))	// sin nodos: cerca de un jugador al azar
	{
		start = rand () % n;
		for (i = 0; i < n; i++)
		{
			anchor = list[(start + i) % n];
			if (Tornado_PickSpot (anchor, spot, &top))
				break;
		}
		if (i == n)
			return NULL;
	}

	// indice libre para la ruta
	slot = 0;
	for (i = (int)maxclients->value + 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
		if (Tornado_IsTornado (e) && e->style == slot)
		{
			slot++;
			i = (int)maxclients->value;	// de nuevo desde el principio
			e = g_edicts + i;
		}

	t = G_Spawn ();
	t->classname = "tornado";
	t->style = slot;
	tornado_path[slot].numNodes = -1;
	t->delay = 0;
	t->movetype = MOVETYPE_NONE;
	t->solid = SOLID_NOT;
	t->takedamage = DAMAGE_NO;
	t->clipmask = 0;
	t->s.modelindex = gi.modelindex ("sprites/null.sp2");
	t->s.sound = gi.soundindex (TORNADO_SOUND);	// loop de cerca
	VectorCopy (spot, t->s.origin);
	t->pos2[2] = top;
	t->timestamp = level.time + tornado_time->value;
	t->ideal_yaw = random () * 360;
	t->teleport_time = 0;
	t->air_finished = 0;
	t->count = 0;
	t->wait = 0;
	t->think = Tornado_Think;
	t->nextthink = level.time + FRAMETIME;
	gi.linkentity (t);

	for (i = 0; i < TORNADO_LEVELS * TORNADO_PER_LEVEL; i++)
		Tornado_AddPart (t, i / TORNADO_PER_LEVEL,
			(i % TORNADO_PER_LEVEL) * (2 * M_PI / TORNADO_PER_LEVEL) + (i / TORNADO_PER_LEVEL) * 0.7,
			300 + (i / TORNADO_PER_LEVEL) * -25, "sprites/null.sp2");

	for (i = 0; i < TORNADO_DEBRIS; i++)
		Tornado_AddPart (t, -1, random () * 2 * M_PI, 260 + random () * 120,
			(i & 1) ? "models/objects/debris2/tris.md2" : "models/objects/debris1/tris.md2");

	gi.bprintf (PRINT_HIGH, "Cuidado: se viene un TORNADO!\n");
	return t;
}

static qboolean Tornado_MapAllowed (void)
{
	char maps[MAX_QPATH * 8], *tok;

	strncpy (maps, tornado_maps->string, sizeof (maps) - 1);
	maps[sizeof (maps) - 1] = 0;
	for (tok = strtok (maps, " ,"); tok; tok = strtok (NULL, " ,"))
		if (!strcmp (tok, "*") || !Q_stricmp (tok, level.mapname))
			return true;
	return false;
}

// cada frame (G_RunFrame): con tornado 1 (o tornado_gungame 1 en el Gun Game), uno cada
// tornado_interval (al azar entre la mitad y ese valor) en tornado_maps
void Tornado_RunFrame (void)
{
	float interval;

	if (level.intermissiontime)
		return;
	if (!tornado->value && !(tornado_gungame->value && G_IsFFA () && ffa_gungame->value))
		return;
	if (!Tornado_MapAllowed ())
		return;

	interval = tornado_interval->value;
	if (interval < 10)
		interval = 10;

	if (!level.tornado_next)
	{
		level.tornado_next = level.time + interval * (0.5 + 0.5 * random ());
		return;
	}
	if (level.time < level.tornado_next)
		return;

	level.tornado_next = level.time + interval * (0.5 + 0.5 * random ());
	if (!Tornado_Count ())
		Tornado_Spawn (NULL);
}

// sv tornado [id|off]
void Svcmd_Tornado_f (void)
{
	char *arg = gi.argv (2);
	edict_t *e, *anchor = NULL;
	int i, n = 0;

	if (!Q_stricmp (arg, "off"))
	{
		for (i = (int)maxclients->value + 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
			if (Tornado_IsTornado (e))
			{
				Tornado_Remove (e);
				n++;
			}
		gi.cprintf (NULL, PRINT_HIGH, "Tornados sacados: %d\n", n);
		return;
	}

	if (arg[0])
	{
		i = atoi (arg);
		if (i < 0 || i >= maxclients->value || !Tornado_CanLift (g_edicts + i + 1))
		{
			gi.cprintf (NULL, PRINT_HIGH, "Uso: sv tornado [id de un jugador vivo | off]\n");
			return;
		}
		anchor = g_edicts + i + 1;
	}

	if (Tornado_Count () >= TORNADO_MAX)
		gi.cprintf (NULL, PRINT_HIGH, "Ya hay %d tornados.\n", TORNADO_MAX);
	else if (!Tornado_Spawn (anchor))
		gi.cprintf (NULL, PRINT_HIGH, "No encontre un lugar al aire libre para el tornado.\n");
	else
		gi.cprintf (NULL, PRINT_HIGH, "Tornado en camino.\n");
}
