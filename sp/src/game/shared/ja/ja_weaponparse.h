//
//  ja_weaponparse.h
//  games
//
//  Created by Jeremy Agostino on 4/29/15.
//
//

#ifndef games_ja_weaponparse_h
#define games_ja_weaponparse_h

#include "weapon_parse.h"

class CCustomWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CCustomWeaponInfo, FileWeaponInfo_t );
	
	float	m_flCycleTime;
	
	float	m_flHeatsinkRechargeRate;
	float	m_flHeatsinkDrainRate;
	float	m_flHeatsinkCooloffDelay;
	
	void	Parse( ::KeyValues* pKeyValuesData, const char* szWeaponName );
};

#endif
