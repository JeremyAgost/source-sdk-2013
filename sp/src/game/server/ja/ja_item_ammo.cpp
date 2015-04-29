//
//  ja_item_ammo.cpp
//  games
//
//  Created by Jeremy Agostino on 4/29/15.
//
//

#include "cbase.h"
#include "gamerules.h"
#include "items.h"
#include "ammodef.h"
#include "eventlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int ITEM_GiveAmmo( CBasePlayer *pPlayer, float flCount, const char *pszAmmoName, bool bSuppressSound = false );

// TODO put in extra source
class CItem_HeatSink : public CItem
{
public:
	DECLARE_CLASS( CItem_HeatSink, CItem );
	
	void Spawn( void )
	{
		Precache( );
		SetModel( "models/items/ar2_grenade.mdl");
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/items/ar2_grenade.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if (ITEM_GiveAmmo( pPlayer, 1, "JA_HeatSinkClip"))
		{
			if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_NO )
			{
				UTIL_Remove(this);
			}
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(item_heatsink, CItem_HeatSink);
