/*       D-Day: Normandy by Vipersoft
 ************************************
 *   $Source: /usr/local/cvsroot/dday/src/p_classes.c,v $
 *   $Revision: 1.10 $
 *   $Date: 2002/07/23 22:48:27 $
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
#include "p_classes.h"
#include "q_shared.h"

// g_classes.c
// D-Day: Normandy Player Classes
// this file is for definitions for classes in DDay.

void initialize_random_seed() {  // Necesario para el modo random_class - ZeRo
	srand(time(NULL));
}

//fills a gun with bullets
void Load_Weapon (edict_t *ent, gitem_t	*item)
{
	gitem_t	*ammo_item;

	// Loads primary weapon when spawning
	ammo_item = FindItemInTeam(item->ammo, item->dllname);
	if (!ammo_item)
	{
		gi.dprintf("WARNING: in Give_Class_Weapon %s spawned with no ammo for %s -> %s\n", ent->client->pers.netname, item->pickup_name, item->ammo);
		return;
	}


	if (team_list[1] && !strcmp(item->dllname, team_list[1]->teamid))  //faf:  if its a team 1 weap...(usually grm)
	{
		if (item->position == LOC_PISTOL)
			ent->client->mags[item->mag_index].pistol_rnd = ammo_item->quantity;
		else if (item->position == LOC_SUBMACHINEGUN)
			ent->client->mags[1].submg_rnd = ammo_item->quantity;
		else if (item->position == LOC_L_MACHINEGUN)
			ent->client->mags[1].lmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_H_MACHINEGUN)
			ent->client->mags[1].hmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_ROCKET)
			ent->client->mags[1].antitank_rnd = ammo_item->quantity;
		else if (item->position == LOC_RIFLE)
			ent->client->mags[1].rifle_rnd = ammo_item->quantity;
		else if (item->position == LOC_SNIPER)
			ent->client->mags[1].sniper_rnd = ammo_item->quantity;
		else if (item->position == LOC_SHOTGUN)
			ent->client->mags[1].shotgun_rnd = ammo_item->quantity;
		else if (item->position == LOC_SUBMACHINEGUN2)
			ent->client->mags[1].submg2_rnd = ammo_item->quantity;
	}

	else if (!strcmp(item->dllname, team_list[0]->teamid))  //usually allied weapons here...
	{
		if (item->position == LOC_PISTOL)
			ent->client->mags[item->mag_index].pistol_rnd = ammo_item->quantity;
		else if (item->position ==  LOC_RIFLE)
			ent->client->mags[0].rifle_rnd = ammo_item->quantity;
		else if (item->position == LOC_SNIPER)
			ent->client->mags[0].sniper_rnd = ammo_item->quantity;  //faf:  not used for usa but so plugin team 1 can use same ammo for inf rifle and sniper rifle
		else if (item->position == LOC_SUBMACHINEGUN)
			ent->client->mags[0].submg_rnd = ammo_item->quantity;
		else if (item->position == LOC_L_MACHINEGUN)
			ent->client->mags[0].lmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_H_MACHINEGUN)
			ent->client->mags[0].hmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_ROCKET)
			ent->client->mags[0].antitank_rnd = ammo_item->quantity;
		else if (item->position == LOC_SNIPER)
			ent->client->mags[0].sniper_rnd = ammo_item->quantity;
		else if (item->position == LOC_SHOTGUN)
			ent->client->mags[0].shotgun_rnd = ammo_item->quantity;
		else if (item->position == LOC_SUBMACHINEGUN2)
			ent->client->mags[0].submg2_rnd = ammo_item->quantity;
	}

	// Free For All Gun Game: a weapon of a faction that is not in the map uses the magazines of its slot
	else if (G_IsFFA() && ffa_gg_on && item->position != LOC_FLAME)
	{
		mags_t	*m = &ent->client->mags[(item->mag_index == 1) ? 1 : 0];

		if (item->position == LOC_PISTOL)
			m->pistol_rnd = ammo_item->quantity;
		else if (item->position == LOC_RIFLE)
			m->rifle_rnd = ammo_item->quantity;
		else if (item->position == LOC_SNIPER)
			m->sniper_rnd = ammo_item->quantity;
		else if (item->position == LOC_SUBMACHINEGUN)
			m->submg_rnd = ammo_item->quantity;
		else if (item->position == LOC_L_MACHINEGUN)
			m->lmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_H_MACHINEGUN)
			m->hmg_rnd = ammo_item->quantity;
		else if (item->position == LOC_ROCKET)
			m->antitank_rnd = ammo_item->quantity;
		else if (item->position == LOC_SHOTGUN)
			m->shotgun_rnd = ammo_item->quantity;
		else if (item->position == LOC_SUBMACHINEGUN2)
			m->submg2_rnd = ammo_item->quantity;
	}

	else if (!strcmp(item->ammo, "flame_mag"))
		ent->client->flame_rnd = ammo_item->quantity;

		// El sniper se entrega listo para disparar (cerrojo cerrado) al spawnear o recogerlo por primera vez
		if (item->position == LOC_SNIPER)
			ent->client->sniper_loaded[item->mag_index] = true;
}



gitem_t *FindTeamItem (char *dllname, int position);
void Give_Class_Weapon(edict_t *ent)
{
	gitem_t		*item;
	gclient_t	*client;
	gitem_t		*ammo_item;

	client=ent->client;

	// Free For All Gun Game: the weapon is the one of the step of the player
	if (G_IsFFA() && ffa_gg_on)
	{
		FFA_GunGameGive(ent);
		return;
	}

	// Free For All defines its own loadout: the legacy loadout overrides below do not apply to it
	if (knifefest->value && !G_IsFFA())
	{
		item = FindItem("Helmet");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
		item = FindItem("Fists");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
		item = FindItem("Knife");
		client->pers.inventory[ITEM_INDEX(item)] = 5;
		client->pers.selected_item=ITEM_INDEX(item);
		client->newweapon=item;
		ChangeWeapon(ent);
		return;
	}


	if (client->resp.mos == ENGINEER)
	{
		item = FindItem("Sandbags");
		client->pers.inventory[ITEM_INDEX(item)]= 2;

	}

	//give everyone a knife & fists & helmet
	// UNLESS invuln_medic
	if (client->resp.mos == MEDIC && invuln_medic->value == 1)
	{
		item = FindItem("Morphine");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}
	else
	{
		item = FindItem("Helmet");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
		item = FindItem("Fists");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
		item = FindItem("Knife");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}
	// faf rifle-only code  //ddaylife 
	if (!G_IsFFA() && (mauser_only->value == 1) && !(client->resp.mos == MEDIC))
	{
		item = FindTeamItem(team_list[1]->teamid, LOC_RIFLE);
	}
	else if (!G_IsFFA() && (sniper_only->value == 1) && !(client->resp.mos == MEDIC))
	{
		item = FindTeamItem(team_list[(client->resp.team_on->index)]->teamid, LOC_SNIPER);
	}
	else if (!G_IsFFA() && swords->value == 1 && client->resp.mos != MEDIC)
	{
		item = FindItem("Sword");
	}
	else if (!G_IsFFA() && random_class->value == 1) // Modo de clases al azar al spawnear, se activa con random_class 1 - ZeRo
	{ 
		int random_value = (rand() % 9); // `rand() % 9` genera [0, 8] que son los numeros correspondientes a las clases.
		client->resp.mos = random_value; // Le asignamos el numero generado.
		item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->weapon1,
							  client->resp.team_on->teamid); // Le entregamos el arma principal a la clase sorteada.
	}

	else if (G_IsFFA() && ffa_pistol) // Free For All pistol match: the pistol of the class is the only weapon
		item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->weapon2,
							  client->resp.team_on->teamid);

	else
		item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->weapon1,
							  client->resp.team_on->teamid);

	if (!item) { //pbowens: prevents from crashing the game
		safe_cprintf(ent, PRINT_HIGH, "weapon1 item not found!\n");
		return;
	}

	// Loads primary weapon when spawning
	Load_Weapon (ent, item);

	client->pers.selected_item=ITEM_INDEX(item);
	client->newweapon=item;
	client->pers.inventory[client->pers.selected_item]=1;

	item = NULL;

	// Free For All: no secondary weapon (pistol), only the primary weapon and the knife
	if (!G_IsFFA() &&
		(item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->weapon2,
                               client->resp.team_on->teamid)))
		client->pers.inventory[ITEM_INDEX(item)] = 1;

	// Loads secondary weapon, if existant, when spawning
	if (item)
	{
		ammo_item = FindItemInTeam(item->ammo, item->dllname);
		if (ammo_item)
		{
			// El cargador va al slot de equipo del arma (mag_index, que InitTeam fija al indice de equipo).
			// Antes se elegia el slot comparando item->dllname con team_list; una pistola nueva que
			// comparte item entre facciones (la Browning del Medic) lee por mag_index, asi que si no
			// coincidian el arma aparecia descargada. Ahora init y lectura usan el mismo indice.
			if (item->position == LOC_PISTOL)
				ent->client->mags[item->mag_index].pistol_rnd = ammo_item->quantity;
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Ammo item %s not found in team %s!\n", item->ammo, item->dllname);
		}
		/*
		ammo_item = FindItem(item->ammo);
		if (!strcmp(item->ammo, "p38_mag"))
			ent->client->mags[1].pistol_rnd = ammo_item->quantity;
		else if (!strcmp(item->ammo, "colt45_mag"))
			ent->client->mags[0].pistol_rnd = ammo_item->quantity;
		*/
	}

	//}
	//if(client->resp.team_on->mos[client->resp.mos]->grenades)
	//{

	if (!no_nades->value && !G_IsFFA())//ddaylife
		// kernel: this forces team id when assigns grenades (italians has potato masher like germans)
		if ((item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->grenades,
                                   client->resp.team_on->teamid)))
			client->pers.inventory[ITEM_INDEX(item)]=client->resp.team_on->mos[client->resp.mos]->grenadenum;
	//}
	//if(client->resp.team_on->mos[client->resp.mos]->special)
	//{
    if (!G_IsFFA() &&	// Free For All: only the weapon of the match and the knife (not the binoculars of the Officer)
		(item = FindItemInTeam(client->resp.team_on->mos[client->resp.mos]->special,
                               client->resp.team_on->teamid)))
		client->pers.inventory[ITEM_INDEX(item)]=client->resp.team_on->mos[client->resp.mos]->specnum;
	//}

	ChangeWeapon(ent);
}
			


//this function is for giving the player ammo...
void Give_Class_Ammo(edict_t *ent)
{
	gitem_t *item, *ammo_item;

	// Free For All Gun Game: FFA_GunGameGive already gave the ammo of the step
	if (G_IsFFA() && ffa_gg_on)
		return;

	if (ent->client->resp.team_on->mos[ent->client->resp.mos]->ammo1 && !(G_IsFFA() && ffa_pistol))
	{
		if (!G_IsFFA() && (mauser_only->value == 1) && !(ent->client->resp.mos == MEDIC))
			item= FindTeamItem(team_list[1]->teamid, LOC_RIFLE);
		else if (!G_IsFFA() && (sniper_only->value == 1) && !(ent->client->resp.mos == MEDIC))
			item= FindTeamItem(team_list[(ent->client->resp.team_on->index)]->teamid, LOC_SNIPER);
		else
			item = FindItemInTeam(ent->client->resp.team_on->mos[ent->client->resp.mos]->weapon1,
								  ent->client->resp.team_on->teamid);

		if (item)
		{
			ammo_item = FindItemInTeam(item->ammo, item->dllname);
			if (!Add_Ammo(ent, ammo_item, ent->client->resp.team_on->mos[ent->client->resp.mos]->ammo1))
				safe_cprintf(ent, PRINT_HIGH, "No ammo for %s\n", item->pickup_name);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "WARNING: in Give_Class_Ammo() item %s is NULL for team %s\n",
						 ent->client->resp.team_on->mos[ent->client->resp.mos]->weapon1,
						 ent->client->resp.team_on->teamid);
		}
	}

	// Free For All only gives the pistol (and its own number of magazines) in a pistol match
	if (ent->client->resp.team_on->mos[ent->client->resp.mos]->ammo2 && (!G_IsFFA() || ffa_pistol))
	{
		item = FindItemInTeam(ent->client->resp.team_on->mos[ent->client->resp.mos]->weapon2,
							  ent->client->resp.team_on->teamid);
		if (item)
		{
			ammo_item = FindItemInTeam(item->ammo, item->dllname);
			if (!Add_Ammo(ent, ammo_item, G_IsFFA() ? FFA_PISTOL_MAGS : ent->client->resp.team_on->mos[ent->client->resp.mos]->ammo2))
				safe_cprintf(ent, PRINT_HIGH, "No ammo for %s\n", item->pickup_name);
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "WARNING: in Give_Class_Ammo() item %s is NULL for team %s\n",
						 ent->client->resp.team_on->mos[ent->client->resp.mos]->weapon2,
						 ent->client->resp.team_on->teamid);
		}
	}
}


// Free For All Gun Game: takes every weapon and ammo of the player and gives the weapon of his step of the
// ladder (loaded, with its spare magazines) and the knife, fists and helmet. It is used when the player
// spawns (Give_Class_Weapon) and when he gets to the next step while he is alive.
void FFA_GunGameGive (edict_t *ent)
{
	gclient_t	*client = ent->client;
	gitem_t		*item, *ammo_item;
	int			i, level = client->resp.ffa_level;

	if (ffa_ladder_len < 1)
		return;

	if (level < 0)
		level = 0;
	if (level >= ffa_ladder_len)
		level = ffa_ladder_len - 1;

	item = ffa_ladder[level];

	for (i = 0; i <= game.num_items; i++)
	{
		if (itemlist[i].flags & (IT_WEAPON | IT_AMMO))
			client->pers.inventory[i] = 0;
	}

	client->pers.inventory[ITEM_INDEX(FindItem("Helmet"))] = 1;
	client->pers.inventory[ITEM_INDEX(FindItem("Fists"))] = 1;
	// on the last step the knife is the only weapon, so there are some to throw (as in knifefest)
	client->pers.inventory[ITEM_INDEX(FindItem("Knife"))] = (level == ffa_ladder_len - 1) ? 5 : 1;

	client->pers.inventory[ITEM_INDEX(item)] = 1;

	if (item->position != LOC_KNIFE)
	{
		Load_Weapon (ent, item);

		ammo_item = FindItemInTeam(item->ammo, item->dllname);
		if (ammo_item && ffa_ladder_mags[level] > 0)
			Add_Ammo(ent, ammo_item, ffa_ladder_mags[level]);
	}

	client->pers.selected_item = ITEM_INDEX(item);
	client->newweapon = item;

	// as when the weapon is put away in the weapon code (p_generic_wep.c) before the change
	client->drop = false;
	client->aim = false;
	ChangeWeapon(ent);
}

// Free For All Gun Game: weapon of the step of the player (the ladder must be built: ffa_ladder_len > 0)
gitem_t *FFA_GunGameWeapon (edict_t *ent)
{
	int		step = ent->client->resp.ffa_level;

	if (step < 0)
		step = 0;
	if (step >= ffa_ladder_len)
		step = ffa_ladder_len - 1;

	return ffa_ladder[step];
}

// Free For All Gun Game: the player killed somebody. He goes up one step; any kill on the last step wins (the
// knife, the fists, the thrown helmet...), but not one with a weapon of an earlier step: a grenade thrown before
// he went up, or the bleed-out of a wound he made before getting to the last step.
void FFA_GunGameKill (edict_t *killer, edict_t *victim, int mod)
{
	gclient_t	*client = killer->client;
	int			last = ffa_ladder_len - 1;

	if (!ffa_gg_on || ffa_gg_winner)
		return;

	if (client->resp.ffa_level >= last)
	{
		switch (mod & ~MOD_FRIENDLY_FIRE)
		{
		case MOD_WOUND:
			// the wound must have been made on the last step
			if (victim->client && victim->client->last_wound_inflictor == killer &&
				victim->client->last_wound_ffa_level >= last)
				ffa_gg_winner = killer;
			break;
		case MOD_PISTOL: case MOD_SHOTGUN: case MOD_RIFLE: case MOD_LMG: case MOD_HMG: case MOD_GRENADE:
		case MOD_G_SPLASH: case MOD_ROCKET: case MOD_R_SPLASH: case MOD_SUBMG: case MOD_SNIPER:
		case MOD_HANDGRENADE: case MOD_HG_SPLASH: case MOD_HELD_GRENADE: case MOD_BACKBLAST: case MOD_TNT:
		case MOD_TNT1: case MOD_HELD_TNT: case MOD_TNT_SPLASH: case MOD_TNT1_SPLASH: case MOD_BAYONET:
			// weapons of the earlier steps (a grenade or a rocket still in the air when he went up)
			break;
		default:
			ffa_gg_winner = killer;
			break;
		}
		return;
	}

	client->resp.ffa_level++;
	client->resp.ffa_streak = 0;

	if (client->resp.ffa_level >= ffa_ladder_len - 1)
		safe_bprintf (PRINT_HIGH, "%s is on the knife level!\n", client->pers.netname);

	// the weapon changes at the next frame: the kill happens in the middle of the weapon code of the killer
	client->resp.ffa_pending = 1;
}

void turret_off (edict_t *self);

// Free For All Gun Game: plays a sound only for that player, at full volume wherever he is (a svc_sound sent to
// his client alone: without attenuation, so it is not lost among the shots or when he is dead)
static void FFA_GunGameSound (edict_t *ent, char *name)
{
	if (!ent->client || ent->ai || !name || !name[0])
		return;

	gi.WriteByte (svc_sound);
	gi.WriteByte (2 | 8);					// SND_ATTENUATION | SND_ENT
	gi.WriteByte (gi.soundindex (name));
	gi.WriteByte (0);						// attenuation 0 (ATTN_NONE)
	gi.WriteShort (((ent - g_edicts) << 3) | CHAN_AUTO);
	gi.unicast (ent, true);
}

// Free For All Gun Game: gives the weapon of the new step to the player who killed (ClientBeginServerFrame)
void FFA_GunGameThink (edict_t *ent)
{
	gclient_t	*client = ent->client;

	if (!client->resp.ffa_pending)
		return;

	client->resp.ffa_pending = 0;

	if (!ffa_gg_on || ent->health <= 0 || ent->deadflag)
		return;

	// the kill cures him of the bleeding and the limp (not the health), as a healthpack does
	ent->wound_location = 0;
	ent->die_time = 0;
	client->bleedwarn = false;
	WeighPlayer (ent);

	turret_off (ent);
	FFA_GunGameGive (ent);
	safe_cprintf (ent, PRINT_HIGH, "Level %i of %i: %s\n", client->resp.ffa_level + 1, ffa_ladder_len,
				  ffa_ladder[client->resp.ffa_level]->pickup_name);
	FFA_GunGameSound (ent, ffa_gg_snd_up->string);
}

// Free For All Gun Game: the player died. A suicide or a kill with a knife takes him down one step; so do the
// deaths in a row without a kill (ffa_gg_deaths; 0 = a fifth of the steps, at least 2, so the longer the ladder
// the more he can die; a negative value turns that demotion off). The weapon of the new step is given when he
// spawns; the sound of going down is heard now.
void FFA_GunGameDeath (edict_t *victim, qboolean suicide, int mod)
{
	gclient_t	*client = victim->client;
	int			limit = ffa_gg_deaths ? (int)ffa_gg_deaths->value : 0;
	qboolean	knifed = !suicide && (mod & ~MOD_FRIENDLY_FIRE) == MOD_KNIFE;

	if (!ffa_gg_on || ffa_gg_winner)
		return;

	if (limit == 0)
	{
		limit = ffa_ladder_len / 5;
		if (limit < 2)
			limit = 2;
	}

	client->resp.ffa_streak++;

	if (suicide || knifed || (limit > 0 && client->resp.ffa_streak >= limit))
	{
		if (client->resp.ffa_level > 0)
		{
			client->resp.ffa_level--;
			safe_cprintf (victim, PRINT_HIGH, "You go down to level %i of %i\n", client->resp.ffa_level + 1, ffa_ladder_len);
			FFA_GunGameSound (victim, ffa_gg_snd_down->string);
		}
		client->resp.ffa_streak = 0;
	}
}

void Show_Mos(edict_t *ent)
{
/*	int i;
	
	if(!ent->client->resp.team_on || !ent->client->resp.mos)
	{
		safe_cprintf(ent,PRINT_HIGH,"Must be on a team to view the open class slots.\n");
		return;
	}

	safe_cprintf(ent,PRINT_HIGH,"\nOpen class slots for %s: \n",ent->client->resp.team_on->teamname);

	for(i=1; i < MAX_MOS; i++) {
		if (ent->client->resp.team_on->mos[i]->available == 99) 
		{
			safe_cprintf(ent, PRINT_HIGH," %10s -- unlimited\n", ent->client->resp.team_on->mos[i]->name);
		} else {
			safe_cprintf(ent, PRINT_HIGH," %10s -- %i\n", 
			ent->client->resp.team_on->mos[i]->name,
			ent->client->resp.team_on->mos[i]->available);
		}
	} */
}
 


void InitMOS_List(TeamS_t *team, SMos_t *mos_list)
{
	int i;
	
	SMos_t **MOS;
	team->mos=MOS=gi.TagMalloc( (sizeof(SMos_t)*MAX_MOS),TAG_LEVEL);
	
	MOS[0] = NULL;
	for(i=1;i<MAX_MOS;i++)
	{
		MOS[i]=gi.TagMalloc( sizeof(SMos_t), TAG_LEVEL);
		MOS[i]->name=gi.TagMalloc( MAX_MOS_NAME_CHAR, TAG_LEVEL);
		MOS[i]->weapon1=gi.TagMalloc( MAX_MOS_NAME_CHAR, TAG_LEVEL);
		MOS[i]->weapon2=gi.TagMalloc( MAX_MOS_NAME_CHAR, TAG_LEVEL);
		MOS[i]->grenades=gi.TagMalloc( MAX_MOS_NAME_CHAR, TAG_LEVEL);
		MOS[i]->special=gi.TagMalloc( MAX_MOS_NAME_CHAR, TAG_LEVEL);
		MOS[i]->MOS_Spaw_Point=gi.TagMalloc( sizeof(mos_list[i].MOS_Spaw_Point), TAG_LEVEL);

		strcpy(MOS[i]->name,mos_list[i].name);

		if(mos_list[i].weapon1)
			strcpy(MOS[i]->weapon1,mos_list[i].weapon1);
		MOS[i]->ammo1=mos_list[i].ammo1;

		if(mos_list[i].weapon2) 
			strcpy(MOS[i]->weapon2,mos_list[i].weapon2);
		MOS[i]->ammo2=mos_list[i].ammo2;
		
		if(mos_list[i].grenades) 
			strcpy(MOS[i]->grenades,mos_list[i].grenades);
		MOS[i]->grenadenum=mos_list[i].grenadenum;
		
		if(mos_list[i].special)
			strcpy(MOS[i]->special,mos_list[i].special);

		MOS[i]->specnum=mos_list[i].specnum;
		MOS[i]->mos=mos_list[i].mos;
		MOS[i]->available=mos_list[i].available;
		MOS[i]->normal_weight=mos_list[i].normal_weight;
		MOS[i]->max_weight=mos_list[i].max_weight;
		MOS[i]->speed_mod=mos_list[i].speed_mod;
		MOS[i]->MOS_Spaw_Point=mos_list[i].MOS_Spaw_Point;
		MOS[i]->skinname=mos_list[i].skinname;
	}

	//team->language = language;
	//team->playermodel = playermodel;
	//team->teamid = teamid;

	//team->mos gi.TagMalloc( (sizeof(char)*playermodel),TAG_LEVEL);

}
