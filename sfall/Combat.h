/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2011  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

struct ChanceModifier {
	long id;
	int maximum;
	int mod;

	ChanceModifier() : id(0), maximum(95), mod(0) {}

	ChanceModifier(long _id, int max, int _mod) {
		id = _id;
		maximum = max;
		mod = _mod;
	}

	void SetDefault() {
		maximum = 95;
		mod = 0;
	}
};

extern long determineHitChance;

void Combat_Init();
void Combat_OnGameLoad();
void BodypartHitChances();

long __fastcall Combat_check_item_ammo_cost(TGameObj* weapon, AttackType hitMode);
bool __stdcall Combat_IsBurstDisabled(TGameObj* critter);

void __stdcall SetBlockCombat(long toggle);

void __stdcall SetHitChanceMax(TGameObj* critter, DWORD maximum, DWORD mod);
void __stdcall KnockbackSetMod(TGameObj* object, DWORD type, float val, DWORD mode);
void __stdcall KnockbackRemoveMod(TGameObj* object, DWORD mode);

void __stdcall SetNoBurstMode(TGameObj* critter, bool on);
void __stdcall DisableAimedShots(DWORD pid);
void __stdcall ForceAimedShots(DWORD pid);
