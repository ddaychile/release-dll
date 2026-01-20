/*       D-Day: Normandy by Vipersoft
 ************************************
 *   $Source: /usr/local/cvsroot/dday/src/g_main.c,v $
 *   $Revision: 1.15 $
 *   $Date: 2002/07/23 21:11:37 $
 * 
 ***********************************

Copyright (C) 2002 Vipersoft

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "g_local.h"
#include "g_cmds.h"
#include "g_maps.h"
#include "q_shared.h"
#include <stdio.h>

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*maxclients;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*dedicated;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

//Flyingnun Password
cvar_t  *flyingnun_password;

//KMM: 9/15  CVARS
//cvar_t	*scope_setting;				// Sniperscope FOV
//cvar_t	*auto_reload;				// change magazine automaticly?
//cvar_t	*auto_weapon_change;		// change weapon when out of ammo?
//cvar_t	*observer_on_death;			// if set, go to observer when dead
//cvar_t	*heal_wounds;			// if set, health fixes chest and stomach wounds
//cvar_t	*flame_shots;				// the number of shots a flamethrower shoots
cvar_t  *RI;						// reinforcement interval -- the length of time before reinforcements are spawned.
cvar_t	*team_kill;					// friendly fire toggle (default off)
cvar_t	*class_limits;				// enable class limits
cvar_t	*spawn_camp_check;			// enable spawn camping checks
cvar_t	*spawn_camp_time;			// the amount of time after spawn to check for spawn camps
cvar_t	*invuln_medic;				// if medics are invulnerable
//cvar_t	*teamgren;					// enables "team grenade mode" where the grenades are team-conscience, a soldier can not pick up his own team's grenade
cvar_t	*death_msg;					// when and how the death msgs are displayed
cvar_t	*level_wait;				// pause time at beginning and end of games
cvar_t	*invuln_spawn;				// how long player is invulnerable after spawn

cvar_t	*arty_delay;				// seconds for artillary to position
cvar_t	*arty_time;					// seconds between each volley 
cvar_t  *arty_max;					// number of shots to be fired in each volley
cvar_t  *arty_confirm;				// seconds to wait for artillery confirmation

//bcass start - easter_egg cvar, AGAIN
cvar_t	*easter_egg;				//turns easter eggs on and off
//bcass end

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

//faf
cvar_t  *player_scores;
cvar_t  *max_gibs;
cvar_t  *extra_anims;//faf
cvar_t  *force_auto_select;

cvar_t  *allied_password;
cvar_t  *axis_password;

cvar_t  *objective_protect;
cvar_t  *ent_files;

cvar_t *exbattleinfo; // ZeRo
cvar_t *random_class;	// ZeRo	
cvar_t *mauser_only;  //ddaylife
cvar_t *sniper_only;  //ddaylife 
cvar_t *no_nades; //ddaylife
cvar_t *airstrikes;

cvar_t *ctc;//capture the church

cvar_t *stats;

cvar_t *fast_knife;
cvar_t *chile;
cvar_t *mashup;
cvar_t *sv_maplist;  //faf,  putting this back in from the original quake2 code
cvar_t	*swords;
cvar_t	*nohud;
cvar_t	*mapvoting;
cvar_t	*constant_play;
cvar_t	*serverimg;

//cvar_t	*crosshair;

cvar_t *bots;
//cvar_t *alliedbots;
//cvar_t *axisbots;
cvar_t *alliedlevel;
cvar_t *axislevel;
//cvar_t *playerminforbots;
cvar_t *playermaxforbots;

cvar_t *knifefest;
cvar_t *fullbright;

cvar_t	*campaign;

// kernel: to split the scoreboard
cvar_t *observer_bscore;

// Afk time before being moved to observer
cvar_t* afk_time;

// kernel: q2pro directories
cvar_t *sys_basedir;
cvar_t *sys_homedir;

// kernel: global class limits
cvar_t *force_limits;
cvar_t *limit_infantry;
cvar_t *limit_officer;
cvar_t *limit_lgunner;
cvar_t *limit_hgunner;
cvar_t *limit_sniper;
cvar_t *limit_engineer;
cvar_t *limit_medic;
cvar_t *limit_special;
cvar_t *limit_flamer;

// kernel: make dday faster again
cvar_t *fast_arty;
cvar_t *fast_bleeding;
cvar_t *fast_sniper;

// kernel: spawn retention for tournament mode
cvar_t *tournament;

// evil: global variables for countdown
int countdownActive = 0;
int countdownValue = 0;
int countdownTimer = 0;
float countdownTimeLimit = 0;
float gameStartTime = 0.0f;
qboolean fiveMinWarning = false;
qboolean oneMinWarning = false;
qboolean threeSeconds = false;

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);
void CleanUpCmds();
void PBM_KillAllFires (void);



//===================================================================


//=============================================================
// So the Server can execute a console command for the Client
// as if the client typed it into their console themselves.
//============================================================
void stuffcmd(edict_t *ent, char *s) 
{
	//JABot[start]
	if (ent->ai || !ent->inuse)
		return;
	//[end]


	gi.WriteByte(11);
	gi.WriteString(s);
	gi.unicast(ent, true);
}


void ShutdownGame (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->client)
			continue;
		if (!ent->inuse)
			continue;
		PMenu_Close(ent);
	}
	gi.dprintf ("==== ShutdownGame ====\n");

	CleanUpCmds();
	//ClearUserDLLs();

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}

void SpawnEntities2 (char *mapname, char *entstring, char *spawnpoint);
/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities2;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	gi.error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}



/*        //faf:  from original q2 code
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

qboolean TestMap(const char *basedir, const char *dir, const char *mapname, const char *mapext)
{
	FILE *check;
	char filename[256];

	if (snprintf(filename, 256, "%s/%s/%s.%s", basedir, dir, mapname, mapext) >= 256)
	{
		gi.dprintf("TestMap: filename truncated\n");
		return false;
	}

	if ((check = fopen(filename, "r")))
	{
		fclose(check);
		return true;
	}
	return false;
}

qboolean MapExists (char *map)
{
	FILE *check;
	char dirname[256];

	// kernel: first use homedir to search for maps
	if (TestMap(sys_homedir->string, GAMEVERSION "/maps", map, "bsp"))
		return true;

	if (TestMap(sys_homedir->string, GAMEVERSION "/maps", map, "bsp.override"))
		return true;

	if (TestMap(sys_homedir->string, "baseq2/maps", map, "bsp"))
		return true;

	// kernel: try basedir instead
	if (TestMap(sys_basedir->string, GAMEVERSION "/maps", map, "bsp"))
		return true;

	if (TestMap(sys_basedir->string, GAMEVERSION "/maps", map, "bsp.override"))
		return true;

	if (TestMap(sys_basedir->string, "baseq2/maps", map, "bsp"))
		return true;

	// kernel: if all failed try current directory
	if (TestMap(".", GAMEVERSION "/maps", map, "bsp"))
		return true;

	if (TestMap(".", GAMEVERSION "/maps", map, "bsp.override"))
		return true;

	if (TestMap(".", "baseq2/maps", map, "bsp"))
		return true;

	return false;
}


void Write_Last_Maps()
{
	FILE *fp;
	int i;

	// kernel: try to open from q2 directories
	fp = DDay_OpenFullPathFile(sys_homedir->string, GAMEVERSION, "lastmaps.txt", "w");

	if (!fp)
		fp = DDay_OpenFullPathFile(sys_basedir->string, GAMEVERSION, "lastmaps.txt", "w");

	if (!fp)
		fp = DDay_OpenFullPathFile(".", GAMEVERSION, "lastmaps.txt", "w");

	if (!fp)
	{
		gi.error ("Couldn't open dday/lastmaps.txt");
		return;
	}

	for (i=0; i<20 && last_maps_played[i]; i++)	{
		fprintf (fp, "%s\n", last_maps_played[i]);
	}
	fclose (fp);
}


void Read_Last_Maps()
{
	int		i,c;
	char	*s, *f;
	char	*lastmaps;

	lastmaps = ReadEntFile("lastmaps.txt");

	if (lastmaps)	{  
		c = 0;
		f = strdup (lastmaps);
		s = strtok(f, "\n");
		for (i=1; i<20; i++)		{
			if (s != NULL) {
				last_maps_played[i]= s;
				s = strtok (NULL, "\n");
			}
		}
	}
}




char *Get_Next_MaplistTxt_Map ()
{
	char	*maps;
	int		i,j,c;
	
	char *s, *f;

	int mapcount;
	int	newmapcount;
	int x;
	int randnum;
	int removed;
	char *possible_maps[300];
	char *maplisttxt[300];


	maps = ReadEntFile("maplist.txt");

	mapcount = 0;

	if (maps)
	{
		c = 0;
		f = strdup (maps);
		s = strtok(f, "\n");
		while (c < 300)
		{
			if (s != NULL) 
			{
				if (MapExists(s))
				{
					maplisttxt[c] = s;
					mapcount++;
					c++;
				}
				else
					gi.dprintf("WARNING: Map '%s' in maplist.txt not found on server!\n",s);

				s = strtok (NULL, "\n");
			}
			else
			{
				maplisttxt[c] = "";
				c++;
			}
		}
	}
	else
	{
		return NULL;
	}

	removed = 0;
	//remove most recently played maps
	for (i = 0; i<20 && removed != mapcount-1 && last_maps_played[i]; i++) //for last_maps_played
	{
		for (j=0; j < mapcount; j++)
		{
			if (!strcmp (maplisttxt[j],""))
				continue;




			if (!strcmp (last_maps_played[i], maplisttxt[j]))
			{	
				maplisttxt[j]= "";
				//gi.dprintf("EEEEEEEE Removing %s\n",campaign_spots[checknum].bspname);
				removed++;
				break;
			}
		}
	} 


	newmapcount = 0;
	//sort possible_maps
	for (x=0; x < 300; x++)
	{
		if (!strcmp (maplisttxt[x],""))
			continue;
		possible_maps[newmapcount] = maplisttxt[x];

		newmapcount++;
	}

/*	for (j = 0; j<50; j++)	{
		if (possible_maps[j] > -1)	{
			gi.dprintf ("%s\n",campaign_spots[possible_maps[j]].bspname);
		}
	}	*/
	randnum = (int)(random()*newmapcount);

	
	return possible_maps[randnum];
	
}

char *Get_Next_Campaign_Map ();
/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	char *s, *t, *f, *sb, *tb;// *fb;
	static const char *seps = " ,\n\r";
	char *mapname, check[20]; // kernel: check must be char[]
	int i, axiscount=0,alliedcount=0;

	char *nextmap;
	edict_t	*ent = NULL;
	
	i = 0;

	mapname = level.mapname;

	if (level.campaign)
	{
		for (i=0; campaign_spots[i].bspname;i++)
		{
			if (!strcmp (level.mapname, campaign_spots[i].bspname))
			{
				Update_Campaign_Info();
				
				if (alliedplatoons == 0)
				{	campaign_winner = 1;}
				else if (axisplatoons == 0)
					campaign_winner = 0;

				else
				{
					for (i=0; campaign_spots[i].bspname; i++)
					{
						if (campaign_spots[i].owner == 0)
							alliedcount++;
						if (campaign_spots[i].owner == 1)
							axiscount++;
					}
					if  (alliedneedspots > 0 &&	alliedcount >= alliedneedspots)
					{campaign_winner = 0;}
					else if (axisneedspots > 0 && axiscount >= axisneedspots)
					{	campaign_winner = 1;}
				}			
				break;
			}
		}

		WriteCampaignTxt();
	}

	//	if (last_maps_played[0])
	//	gi.dprintf("xxx%s\n",last_maps_played[0]);
	for (i=20; i>0; i--)
	{
		if (last_maps_played[i-1])
		{
			last_maps_played[i]=last_maps_played[i-1];
			//			gi.dprintf("%s\n", last_maps_played[i]);
		}
	}
	if (level.botfiles)
	{
		last_maps_played[0] = level.botfiles;
	}
	else
	{
		last_maps_played[0] = level.mapname;
	}

	Read_Last_Maps ();
	Write_Last_Maps();

	if (level.campaign && campaign_winner < 0)
	{
		nextmap = Get_Next_Campaign_Map();
		if (nextmap != NULL)
		{	
			ent = G_Spawn ();
			ent->classname = "target_changelevel";
			ent->map = nextmap;
			safe_bprintf (PRINT_HIGH, "Next map: %s \n", nextmap);		
			BeginIntermission (ent);	
			return;
		}
		else
		{	//hack job! winning team has no place to go, so pretend the losing team won, get 
			//next map, then set everything back again
			Last_Team_Winner = (Last_Team_Winner+1)%2;
			nextmap = Get_Next_Campaign_Map();
			if (nextmap != NULL)
			{
				Last_Team_Winner = (Last_Team_Winner+1)%2;
				ent = G_Spawn ();
				ent->classname = "target_changelevel";
				ent->map = nextmap;
				safe_bprintf (PRINT_HIGH, "Next map: %s \n", nextmap);		
				BeginIntermission (ent);	
				return;
			}
			else
			{
				(Last_Team_Winner = (Last_Team_Winner + 1) % 2);
			}
		}
	}

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = mapname;
		safe_bprintf (PRINT_HIGH, "Next map: %s \n", level.mapname);
	}


	// faf:  maplist code, based on baseq2 version
	if (*sv_maplist->string) 
	{
		if (!strcmp(sv_maplist->string, "txt"))
		{
			nextmap = Get_Next_MaplistTxt_Map();
			if (nextmap)
			{
				safe_bprintf (PRINT_HIGH, "Next map: %s \n", nextmap);
				BeginIntermission (CreateTargetChangeLevel (nextmap) );
				//last_maplist_map_played = team_list[0]->nextmap;
				//first_non_maplist_map = NULL;
				return;
			}
			else
			{
				safe_bprintf (PRINT_HIGH, "Next map: %s \n", nextmap);
				BeginIntermission (CreateTargetChangeLevel ("hill") );
				return;

			}
		}
		else
		{
			s = strdup(sv_maplist->string);
			f = NULL;
			t = strtok(s, seps);
			while (t != NULL) 
			{
				//add campaigns to maplist
				if (strstr (mapname,t) && strcmp(mapname,t))
				{	
					//gi.dprintf ("rwef %s %s\n",mapname, t);
					if (team_list[0] && strstr(team_list[0]->nextmap,t))
					{
						//if campaign points back to first map, move on in maplist
						char ck[20];
						sprintf(ck,"%s",t);
						strcat (ck,"1");	
						//gi.dprintf ("crere %s %s\n",ck, t);
						if (strcmp(ck,team_list[0]->nextmap)) 
						{
							safe_bprintf (PRINT_HIGH, "Next map: %s \n", team_list[0]->nextmap);
							BeginIntermission (CreateTargetChangeLevel (team_list[0]->nextmap) );
							//last_maplist_map_played = team_list[0]->nextmap;
							//first_non_maplist_map = NULL;
							return;
						}
						else
						{
							//last_maplist_map_played = mapname;
							//first_non_maplist_map = NULL;
							mapname = t;//pretend we just left map 'invade' (for example) so it continues maplist
						}
					}
				}
				if (!f)
					f = t;
				t = strtok(NULL, seps);
			}
			s = strdup(sv_maplist->string);
			f = NULL;
			t = strtok(s, seps);
			while (t != NULL) 
			{
				if (Q_stricmp(t, mapname) == 0)   //if the running map is on maplist
				{
					//safe_bprintf (PRINT_HIGH,"sdf %s %s\n",mapname, t);
					// it's in the list, go to the next one
					t = strtok(NULL, seps);
					if (t == NULL) // end of list, go to first one
					{ 
						if (f == NULL) // there isn't a first one, same level
						{
							if (MapExists(mapname))
							{
								safe_bprintf (PRINT_HIGH, "Next map: %s \n", mapname);
								BeginIntermission (CreateTargetChangeLevel (mapname) );
								//last_maplist_map_played = mapname;
								//first_non_maplist_map = NULL;
								return;
							}
						}
						else
						{
							if (MapExists(f))
							{
								safe_bprintf (PRINT_HIGH, "Next map: %s \n", f);
								BeginIntermission (CreateTargetChangeLevel (f) );
								return;
							}
							else 
							{
								// kernel: check if f+"1" can be a map
								snprintf(check, 20, "%s1", f);
								if (MapExists(check))
								{
									safe_bprintf (PRINT_HIGH, "Next map: %s \n", check);
									BeginIntermission (CreateTargetChangeLevel (check));
									return;
								}
							}
						}
					} 
					else
					{
						if (MapExists(t))
						{
							safe_bprintf (PRINT_HIGH, "Next map: %s \n", t);
							BeginIntermission (CreateTargetChangeLevel (t) );
							return;
						}
						else 
						{
							//last_maplist_map_played = t;
							//first_non_maplist_map = NULL;
							snprintf(check, 20, "%s1", t);
							//gi.dprintf("%s kljfdsjlk\n",check);
							if (MapExists(check))
							{
								safe_bprintf (PRINT_HIGH, "Next map: %s \n", check);
								BeginIntermission (CreateTargetChangeLevel (check) );
								return;
							}
							else
								gi.dprintf("map doesn't exist for sv_maplist.\n");

						}
					}
				}

				if (!f)
					f = t;
				t = strtok(NULL, seps);
			}
				// if last_maplist_map_played == 0, then start at first maplist map
				// if it has a value, play the map after that
				// if it has a value and map after that is null, play first map in maplist
			
			// t becomes tb
			// f becomes fb
			// s becomes sb
			// seps stays same


			if (t == NULL) //faf:  happens when running a map thats not on maplist and map is to change
			{
				//let it stay off maplist until a map on the maplist is changed to
				if (team_list[0]->nextmap && MapExists(team_list[0]->nextmap))
				{
					safe_bprintf (PRINT_HIGH, "Next map: %s \n", team_list[0]->nextmap);
					BeginIntermission (CreateTargetChangeLevel (team_list[0]->nextmap));
					return;
				}
				else
				{
					//restart maplist
					sb = strdup(sv_maplist->string);
					tb = strtok(s, seps);

					if (MapExists(tb))
					{
						safe_bprintf (PRINT_HIGH, "Next map: %s \n", tb);
						BeginIntermission (CreateTargetChangeLevel (tb));
						return;
					}
					else
					{
						snprintf(check, 20, "%s1", tb);
						if (MapExists(check))
						{
							safe_bprintf (PRINT_HIGH, "Next map: %s \n", check);
							BeginIntermission (CreateTargetChangeLevel (check) );
							return;
						}
					}
				}
			}
			else
			{
				// kernel: t was not found by MapExists, repeat the current map
				safe_bprintf(PRINT_HIGH, "Next map: %s\n", level.mapname);
				BeginIntermission(CreateTargetChangeLevel(level.mapname));
				return;
			}
		}
	}

	//alternate maplist, not as good

	//else if ((int)dmflags->value & DF_MAP_LIST)  // maplist active? 
	else if ((int)dmflags->value & DF_MAP_LIST  && maplist.nummaps > 0)  // faf: fixes crash
	{ 
		switch (maplist.rotationflag)        // choose next map in list 
		{ 
		case ML_ROTATE_SEQ:        // sequential rotation 
			i = (maplist.currentmap + 1) % maplist.nummaps; 
			break; 
		
		case ML_ROTATE_RANDOM:     // random rotation 
			i = (int) (random() * maplist.nummaps); 
			break;
		
		default:       // should never happen, but set to first map if it does 
			i = 0; 
		} // end switch 


		maplist.currentmap = i; 
		
		ent = G_Spawn (); 
		ent->classname = "target_changelevel"; 
		if (maplist.mapnames[i] && !level.nextmap[0])
			ent->map = maplist.mapnames[i]; 
		else if (level.nextmap[0])
			ent->map = level.nextmap;
		else
			ent->map = level.mapname;

		safe_bprintf (PRINT_HIGH, "Next map: %s \n", ent->map);
	} 
	
	else if (Last_Team_Winner <= 1 && Last_Team_Winner > -1 && team_list[Last_Team_Winner] && team_list[Last_Team_Winner]->nextmap)
	{
		ent= G_Spawn();
		ent->classname = "target_changelevel";
		//ent->map = team_list[Last_Team_Winner]->nextmap;
		//faf: f campaign mode
		ent->map = team_list[0]->nextmap;
		safe_bprintf (PRINT_HIGH, "Next map: %s \n", ent->map);
	}
	//tie game, advance to next map
	else if (Last_Team_Winner == -1 && team_list[0] && team_list[0]->nextmap)
	{
		ent= G_Spawn();
		ent->classname = "target_changelevel";
		//ent->map = team_list[Last_Team_Winner]->nextmap;
		//faf: f campaign mode
		ent->map = team_list[0]->nextmap;
		safe_bprintf (PRINT_HIGH, "Next map: %s \n", ent->map);
	}
	else if (level.nextmap[0])
	{	// go to a specific map
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.nextmap;
		safe_bprintf (PRINT_HIGH, "Next map: %s \n", ent->map);
	}
	else
	{	// search for a changeleve
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
	}

	if (!ent)
	{	// the map designer didn't include a changelevel,
		// so create a fake ent that goes back to the same level
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.mapname;
		safe_bprintf (PRINT_HIGH, "Next map: %s \n", ent->map);
	}

	BeginIntermission(ent);
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int			i=0,tempscore=0;
	float		delay=0.0;
	qboolean Is_Game_Over=false;
	gclient_t	*cl;

	int allies_kills_win = 0;
	int axis_kills_win = 0;
	int allies_points_win = 0;
	int axis_points_win = 0;

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;


	//faf: ctb code
	if (level.ctb_time)
	{	
		vec3_t		w; //faf
		float		range;//faf
		edict_t		*check;
		edict_t		*usaflag;
		edict_t     *grmflag;
		edict_t     *e;

		if (level.time == level.ctb_time)
			gi.bprintf (PRINT_HIGH, "Timelimit hit!\n");

		if (level.time == level.ctb_time + 1)
			gi.bprintf (PRINT_HIGH, "Next team to bring the briefcase to their base wins!\n");

		if (level.time >= level.ctb_time)
		{
//			gi.bprintf (PRINT_HIGH, "Timelimit hit!\n");

			for (check = g_edicts; check < &g_edicts[globals.num_edicts]; check++)
			{
				if (!check->inuse)
					continue;

				if (!strcmp(check->classname, "usa_base"))
				{
					usaflag = check;
				}

			}

			for (check = g_edicts; check < &g_edicts[globals.num_edicts]; check++)
			{
				if (!check->inuse)
					continue;

				if (!strcmp(check->classname, "grm_base"))
				{
					grmflag = check;
				}
			}


			if (grmflag && usaflag)
			{
				for (check = g_edicts; check < &g_edicts[globals.num_edicts]; check++)
				{
					if (!check->inuse)
						continue;

					if (check->deadflag)
						continue;


					if (!strcmp(check->classname, "briefcase"))
					{

						VectorSubtract (check->s.origin, usaflag->s.origin, w);
						range = VectorLength (w);

						if (range < 40)  //briefcase is near usa flag at end of map
						{
							team_list[0]->score	+= 100;
						}

						VectorSubtract (check->s.origin, grmflag->s.origin, w);
						range = VectorLength (w);

						if (range < 40)  //briefcase is near grm flag at end of map
						{
							team_list[1]->score	+= 100;

						}
					}
					
				}
				//see if anyone's carrying a briefcase near the flag
				for (i=0 ; i < game.maxclients ; i++)
				{
					e = g_edicts + 1 + i;
					if (!e->inuse || e->flyingnun || !e->client)
						continue;

					if(e->client->pers.inventory[ITEM_INDEX(FindItem("briefcase"))])
					{
						VectorSubtract (e->s.origin, usaflag->s.origin, w);
						range = VectorLength (w);

						if (range < 40)  //briefcase is near usa flag at end of map
						{
							team_list[0]->score	+= 100;
						}

						VectorSubtract (e->s.origin, grmflag->s.origin, w);
						range = VectorLength (w);

						if (range < 40)  //briefcase is near grm flag at end of map
						{
							team_list[1]->score	+= 100;

						}
					}
				
				}
			}
		}
	}
//faf end


	for(i=0; i < MAX_TEAMS;i++) 
	{
		if (!team_list[i])
			break;
		
		if (team_list[i]->time_to_win) 
		{

			delay = (team_list[i]->time_to_win - level.time);
		
		/*	gi.dprintf("time_to_win [%i] = %f\n", i, team_list[i]->time_to_win);
			gi.dprintf("delay       [%i] = %f\n", i, delay);
			gi.dprintf("level.time  [%i] = %f\n", i, level.time); */

			if (delay <= 0) {
				safe_bprintf( PRINT_HIGH, "Team %s is victorious!\n", team_list[i]->teamname);
				Last_Team_Winner=i;
				EndDMLevel ();
				break;
			} 

			else if (delay == 30)
				safe_bprintf ( PRINT_HIGH, "30 seconds left before team %s wins the battle!\n", team_list[i]->teamname);
			else if (delay == 60)
				safe_bprintf ( PRINT_HIGH, "1 minute left before team %s wins the battle!\n", team_list[i]->teamname);
			else if (delay == 300)
				safe_bprintf ( PRINT_HIGH, "5 minutes left before team %s wins the battle!\n", team_list[i]->teamname);
		}
	}
	//faf:  rewrite this so tie games are announced correctly
	if (team_list[0] && team_list[1])
	{
		if (team_list[0]->need_kills > 0 &&
			team_list[0]->kills >= team_list[0]->need_kills) 
			allies_kills_win++;

		if (team_list[1]->need_kills > 0 &&
			team_list[1]->kills >= team_list[1]->need_kills) 
			axis_kills_win++;

		if (team_list[0]->need_points > 0 && 
			team_list[0]->score >= team_list[0]->need_points) 
			allies_points_win++;

		if (team_list[1]->need_points > 0 && 
			team_list[1]->score >= team_list[1]->need_points) 
			axis_points_win++;

		if (team_list[0]->kills_and_points)
		{
			if (allies_kills_win + allies_points_win < 2)
			{
				allies_kills_win = 0;
				allies_points_win = 0;
			}
		}
		if (team_list[1]->kills_and_points)
		{
			if (axis_kills_win + axis_points_win < 2)
			{
				axis_kills_win = 0;
				axis_points_win = 0;
			}
		}
			
		if (allies_kills_win || allies_points_win ||
			axis_kills_win || axis_points_win)
		{
			if (allies_kills_win + allies_points_win >
				axis_kills_win + axis_points_win)
			{
				if (allies_kills_win)
				{
					safe_bprintf( PRINT_HIGH, "Team %s is victorious (%i / %i kills)!\n", 
					team_list[0]->teamname, 
					team_list[0]->kills,
					team_list[0]->need_kills);

					Last_Team_Winner=0;
					EndDMLevel ();
					return;
				}
				else 
				{
					safe_bprintf( PRINT_HIGH, "Team %s is victorious (%i / %i points)!\n", 
					team_list[0]->teamname, 
					team_list[0]->score,
					team_list[0]->need_points);

					Last_Team_Winner=0;
					EndDMLevel ();
					return;
				}
			}
			else if (allies_kills_win + allies_points_win <
				axis_kills_win + axis_points_win)
			{
				if (axis_kills_win)
				{
					safe_bprintf( PRINT_HIGH, "Team %s is victorious (%i / %i kills)!\n", 
					team_list[1]->teamname, 
					team_list[1]->kills,
					team_list[1]->need_kills);

					Last_Team_Winner=1;
					EndDMLevel ();
					return;
				}
				else 
				{
				safe_bprintf( PRINT_HIGH, "Team %s is victorious (%i / %i points)!\n", 
					team_list[1]->teamname, 
					team_list[1]->score,
					team_list[1]->need_points);

					Last_Team_Winner=1;
					EndDMLevel ();
					return;
				}
			}
			else //faf:  for tie games
			{
				//lol centerprintall("TTTTTT III EEEE   GGG   AA  M   M EEEE !!!\n  TT    I  E     G     A  A MM MM E    !!!\n  TT    I  EEE   G  GG AAAA M M M EEE  !!!\n  TT    I  E     G   G A  A M   M E       \n  TT   III EEEE   GGG  A  A M   M EEEE !!!\n");
				centerprintall("T I E   G A M E !");
				 Last_Team_Winner = -1;

				EndDMLevel ();
				return;
			}
		}
	}

		
	//faf: this is not used below
	if(Is_Game_Over)
	{
		for(i=0;i<MAX_TEAMS;i++)
		{
			if(team_list[i]->score > tempscore) 
			{
				tempscore=team_list[i]->score;
				Last_Team_Winner=i;
			}
		}
		safe_bprintf (PRINT_HIGH, "%s is victorius.\n",team_list[Last_Team_Winner]->teamname);
		EndDMLevel ();
		return;
	}

	// evil: print and play sound for timelimit if is setted
	if (timelimit->value)
	{
		int totalTime = timelimit->value * 60;
		int timeElapsed = level.time - gameStartTime;
		int timeRemaining = totalTime - timeElapsed;

		if (timeRemaining == 300 && !fiveMinWarning)
		{
			gi.positioned_sound(vec3_origin, &g_edicts[0], CHAN_AUTO, gi.soundindex("misc/5_minute.wav"), 1, ATTN_NONE, 0);
			centerprintall("Quedan 5 minutos de tiempo!");
			fiveMinWarning = true;
		}

		if (timeRemaining == 60 && !oneMinWarning)
		{
			gi.positioned_sound(vec3_origin, &g_edicts[0], CHAN_AUTO, gi.soundindex("misc/1_minute.wav"), 1, ATTN_NONE, 0);
			centerprintall("Queda 1 minuto de tiempo!", 5.0f);
			oneMinWarning = true;
		}

		if (timeRemaining == 4 && !threeSeconds)
		{
			gi.positioned_sound(vec3_origin, &g_edicts[0], CHAN_AUTO, gi.soundindex("misc/final_count.wav"), 1, ATTN_NONE, 0);
			centerprintall("Quedan 3 segundos!", 5.0f);
			threeSeconds = true;
		}

		if (timeRemaining == 3)
			centerprintall("Quedan 2 segundos!", 5.0f);

		if (timeRemaining == 2)
			centerprintall("Queda 1 segundo!", 5.0f);

		// end game if timelimit hits
		if (timeRemaining <= 0)
		{
			safe_bprintf(PRINT_HIGH, "Timelimit hit.\n");

			// check who team wins by kills
			// 0:allies 1:axis
			if (team_list[0]->kills > team_list[1]->kills)
			{
				Last_Team_Winner = 0;
			}
			else if (team_list[0]->kills < team_list[1]->kills)
			{
				Last_Team_Winner = 1;
			}				
			else
			{
				Last_Team_Winner = -1; // tie or draw
			}
						
			ResetCountTimer();
			EndDMLevel();
			return;
		}
	}

	if (fraglimit->value)
	{
		// kernel: check if fraglimit changed its value to update need_kills properties
		for (i = 0; i < MAX_TEAMS; ++i)
		{
			if (team_list[i]->need_kills != (int) fraglimit->value)
				team_list[i]->need_kills = (int) fraglimit->value;
		}
		/*for (i=0 ; i<maxclients->value ; i++)
		{
			cl = game.clients + i;
			if (!g_edicts[i+1].inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				safe_bprintf (PRINT_HIGH, "Fraglimit hit.\n");
				ResetCountTimer();
				EndDMLevel ();
				return;
			}
		}*/
	}
}

//evil: for reset timelimit, countdown timers and booleans
void ResetCountTimer(void)
{
	timelimit->value = 0;
	countdownActive = 0;
	countdownValue = 0;
	countdownTimer = 0;
	countdownTimeLimit = 0;
	fiveMinWarning = false;
	oneMinWarning = false;
	threeSeconds = false;
}


/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];
	char	campaignfilename[MAX_QPATH] = "";

	WriteCampaignTxt();


if (campaign_winner>-1)
{
	sprintf(campaignfilename, "dday/campaigns/%s.campaign", level.campaign);
/*	won't work  if (remove (campaignfilename))
		gi.dprintf("removed %s\n",campaignfilename); */
	SetupCampaign (true);
	gi.cvar_set("campaign", "");

}

	//JABot[start] (Disconnect all bots before changing map)
	BOT_RemoveBot("all", NULL);
	//[end]




 

	Com_sprintf (command, sizeof(command), "map \"%s\"\n", level.changemap);
//	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();
/*-----/ PM /-----/ NEW:  Remove all flames before leaving level. /-----*/
    PBM_KillAllFires ();
/*----------------------------------------------------------------------*/


	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}

	// reload MOTD
	if (!DDay_LoadMOTD())
		gi.dprintf("Failed to load the MOTD.\n");

	//LevelExitUserDLLs();
}



int HumanPlayerCount(void)
{
	int i;
	int playercount = 0;
    edict_t *check_ent;

        
	for (i = 1; i <= maxclients->value; i++)
    {
         check_ent = g_edicts + i;
         if (!check_ent->inuse)
			 continue;
		 if (!check_ent->client ||
			 !check_ent->client->resp.team_on)
			 continue;
		 if (check_ent->ai)
			 continue;

		 playercount++;

	}
	return playercount;
                              
}


/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i;
	edict_t	*ent,*check_ent;
	int axiscount, alliedcount;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	//AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	if ((level.framenum%40 == 1 && qbots && bots->value == 0) ||
		(bots->value == 1 && 
		//num_clients >= playermaxforbots->value
		HumanPlayerCount() >= playermaxforbots->value
		
		))
	{

		qbots = false;
		for (i = 1; i <= maxclients->value; i++)
		{
			check_ent = g_edicts + i;
			if (!check_ent->inuse)
				continue;
			if (!check_ent->client)
				continue;

			if (check_ent->ai)
				BOT_RemoveBot("",check_ent);

		}
	}

	//maintain certain player/bot levels per team according to cvars
	else if (level.framenum > 40 && bots->value && 
		((level.framenum < 300 && level.framenum%10 == 1) ||
		(level.framenum%40 == 1))) //check every 4 seconds, except in beginning
	{
		alliedcount = PlayerCountForTeam(0); 
		axiscount = PlayerCountForTeam(1); 
		if (alliedcount > alliedlevel->value)
		{
			for (i = 1; i <= maxclients->value; i++)
			{
				check_ent = g_edicts + i;
				if (!check_ent->inuse)
					continue;
				if (!check_ent->client ||
					!check_ent->client->resp.team_on)
					continue;

				if (check_ent->client->resp.team_on->index != 0)
					continue;
				if (check_ent->ai)
				{
					BOT_RemoveBot("",check_ent);
					if (PlayerCountForTeam(0) <= alliedlevel->value)
						break;
				}

			}
		}
		else if (level.allied_cmps && alliedcount < alliedlevel->value &&
			 ( level.framenum > 1800 || (HumanPlayerCount() <= playermaxforbots->value - 1 || playermaxforbots->value == 0) ))
		{
				BOT_SpawnBot ( 0, "", "", NULL);
		}

		if (axiscount > axislevel->value)
		{
			for (i = 1; i <= maxclients->value; i++)
			{
				check_ent = g_edicts + i;
				if (!check_ent->inuse)
					continue;
				if (!check_ent->client ||
					!check_ent->client->resp.team_on)
					continue;

				if (check_ent->client->resp.team_on->index != 1)
					continue;
				if (check_ent->ai)
				{
					BOT_RemoveBot("",check_ent);
					if (PlayerCountForTeam(1) <= axislevel->value)
						break;
				}

			}
		}
		else if (level.axis_cmps && axislevel->value && axiscount < axislevel->value &&
		 ( level.framenum > 1800 || (HumanPlayerCount() <= playermaxforbots->value - 1 || playermaxforbots->value == 0) ))
		{
			BOT_SpawnBot ( 1, "", "", NULL);
		}
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		/*if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}*/

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
			//JABot[start]
			if( !ent->ai ) //jabot092(2)
			//[end]
				continue;
		}

		G_RunEntity (ent);
	}


	// evil: handle countdown if its active
	if (countdownActive)
	{
		if (countdownTimer > 0)
		{
			countdownTimer--;
		}
		else
		{			
			if (countdownValue > 0)
			{
				fiveMinWarning = false;
				oneMinWarning = false;
				threeSeconds = false;
				
				if (countdownValue == 5)
				{
					centerprintall("Preparados!");
					gi.positioned_sound(vec3_origin, &g_edicts[0], CHAN_AUTO, gi.soundindex(va("misc/first_count.wav")), 1, ATTN_NONE, 0);
				}

				if (countdownValue == 3)
					centerprintall("3...");

				if (countdownValue == 2)
					centerprintall("2...");

				if (countdownValue == 1)
					centerprintall("1...");

				countdownValue--;
				countdownTimer = 10;
			}
			else
			{
				centerprintall("Que comience el juego!");
				gameStartTime = level.time;
				timelimit->value = countdownTimeLimit;				
				countdownActive = 0;
			}
		}
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

	//JABot[start]
	AITools_Frame();	//give think time to AI debug tools
	//[end]

	if (nohud->value && level.framenum %100 == 1)
			gi.configstring (CS_STATUSBAR, " if 21 xr 1 0 yb 0 xv  0 yv  0 pic 21 endif "); //just crosshair

}
