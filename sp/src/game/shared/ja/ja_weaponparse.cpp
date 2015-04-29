//
//  ja_weaponparse.cpp
//  games
//
//  Created by Jeremy Agostino on 4/29/15.
//
//

#include "cbase.h"
#include "ja_weaponparse.h"
#include "KeyValues.h"

void CCustomWeaponInfo::Parse( ::KeyValues* pKeyValuesData, const char* szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );
	
	m_flCycleTime	= pKeyValuesData->GetFloat( "CycleTime", 0.15 );
	
	m_flHeatsinkRechargeRate	= pKeyValuesData->GetFloat( "HeatsinkRechargeRate", 1.667f );
	m_flHeatsinkDrainRate		= pKeyValuesData->GetFloat( "HeatsinkDrainRate", 4.0f );
	m_flHeatsinkCooloffDelay	= pKeyValuesData->GetFloat( "HeatsinkCooloffDelay", 3.0f );
}

// This function probably exists somewhere in your mod already.
FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CCustomWeaponInfo;
}
