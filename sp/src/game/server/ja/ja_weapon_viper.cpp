//
//  ja_weapon_viper.cpp
//  games
//
//  Created by Jeremy Agostino on 4/29/15.
//
//

#include "cbase.h"
#include "ja/ja_heatsinkmachinegun.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "grenade_ar2.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponViper : public CHeatsinkMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponViper, CHeatsinkMachineGun );
	
	CWeaponViper();
	
	DECLARE_SERVERCLASS();

	bool	IsWeaponZoomed() { return m_bInZoom; }

	void	ItemBusyFrame( void );
	void	ItemPostFrame( void );
	void	AddViewKick( void );

	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	void	StopEffects( void );
	void	CheckZoomToggle( void );
	void	ToggleZoom( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity( void );
	
	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();
	
	DECLARE_ACTTABLE();

protected:

	bool	m_bInZoom;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponViper, DT_WeaponViper)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_viper, CWeaponViper );
PRECACHE_WEAPON_REGISTER(weapon_viper);

BEGIN_DATADESC( CWeaponViper )
	DEFINE_FIELD( m_bInZoom,		FIELD_BOOLEAN ),
END_DATADESC()

acttable_t	CWeaponViper::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },
	
	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims
	
	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims
	
	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims
	
	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims
	
	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims
	
	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities
	
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE(CWeaponViper);

//=========================================================
CWeaponViper::CWeaponViper( )
{
	m_fMinRange1		= 0;// No minimum range.
	m_fMaxRange1		= 3000;
	
	m_bInZoom			= false;
	
	m_bAltFiresUnderwater = false;
	
	m_iClip1 = GetMaxClip1();
}

bool CWeaponViper::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopEffects();
	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponViper::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	if ( m_bInZoom )
	{
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if ( pPlayer->SetFOV( this, 20, 0.1f ) )
		{
			m_bInZoom = true;
		}
	}
}

void CWeaponViper::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		ToggleZoom();
	}
}

void CWeaponViper::ItemBusyFrame( void )
{
	// Allow zoom toggling even when we're reloading
	CheckZoomToggle();
}

void CWeaponViper::ItemPostFrame( void )
{
	// Allow zoom toggling
	CheckZoomToggle();

	BaseClass::ItemPostFrame();
}

void CWeaponViper::StopEffects( void )
{
	// Stop zooming
	if ( m_bInZoom )
	{
		ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponViper::GetPrimaryAttackActivity( void )
{
	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponViper::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	// Big kick, up and a little to the right
	pPlayer->ViewPunch( QAngle( random->RandomFloat( -3, -4 ), random->RandomFloat( -1, -2 ), 0 ) );
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponViper::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};
	
	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);
	
	return proficiencyTable;
}
