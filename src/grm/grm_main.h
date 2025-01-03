/*       D-Day: Normandy by Vipersoft
 ************************************
 *   $Source: /usr/local/cvsroot/dday/src/grm/grm_main.h,v $
 *   $Revision: 1.7 $
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


#include "grm_defines.h"
#include "../g_local.h"
#include "../g_cmds.h"


void Weapon_Pistol_Fire (edict_t *ent);
void Weapon_Rifle_Fire (edict_t *ent);
void Weapon_Submachinegun_Fire (edict_t *ent);
void Weapon_LMG_Fire (edict_t *ent);
void Weapon_MG42_Fire (edict_t *ent);
void Weapon_Rocket_Fire (edict_t *ent);
void Weapon_Sniper_Fire (edict_t *ent);



void Weapon_P38 (edict_t *ent);
void Weapon_m98k(edict_t *ent);
void Weapon_MP40(edict_t *ent);
void Weapon_MP43(edict_t *ent);
void Weapon_MG42(edict_t *ent);
void Weapon_Psk(edict_t *ent);
void Weapon_m98ks(edict_t *ent);

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_LFIRE_LAST, int FRAME_LIDLE_LAST, int FRAME_RELOAD_LAST, int FRAME_LASTRD_LAST,
					 int FRAME_DEACTIVATE_LAST, int FRAME_RAISE_LAST,int FRAME_AFIRE_LAST, int FRAME_AIDLE_LAST,
					 int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent));
