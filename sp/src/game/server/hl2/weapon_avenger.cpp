//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
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

static ConVar sk_avenger_max_ammo("sk_avenger_max_ammo", "100" );
static ConVar sk_avenger_recharge_rate("sk_avenger_recharge_rate", "20" );
static ConVar sk_avenger_drain_rate("sk_avenger_drain_rate", "40" );
static ConVar sk_avenger_overheat_delay("sk_avenger_overheat_delay", "8" );

class CWeaponAvenger : public CHLMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAvenger, CHLMachineGun );

	CWeaponAvenger();

	DECLARE_SERVERCLASS();
	
	void	RemoveAmmo( float flAmmoAmount );
	void	RechargeAmmoThink( void );
	void	OverheatClearThink( void );
	
	void	Precache( void );
	void	AddViewKick( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	
	bool	Deploy( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );
	void	FinishReload( void );
	bool	ReloadOrSwitchWeapons( void );
	
//	bool	UsesClipsForAmmo1( void ) { return false; }
	float	GetFireRate( void ) { return 0.075f; }	// 13.3hz
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();
	
	DECLARE_ACTTABLE();

protected:

	float	m_flChargeRemainder;
	float	m_flDrainRemainder;
	
	bool	m_bOverheated;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAvenger, DT_WeaponAvenger)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_avenger, CWeaponAvenger );
PRECACHE_WEAPON_REGISTER(weapon_avenger);

BEGIN_DATADESC( CWeaponAvenger )

	DEFINE_FIELD( m_flChargeRemainder, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDrainRemainder, FIELD_FLOAT ),

	DEFINE_FIELD( m_bOverheated, FIELD_BOOLEAN ),

	DEFINE_FUNCTION( RechargeAmmoThink ),
	DEFINE_FUNCTION( OverheatClearThink ),

END_DATADESC()

acttable_t	CWeaponAvenger::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
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

IMPLEMENT_ACTTABLE(CWeaponAvenger);

//=========================================================
CWeaponAvenger::CWeaponAvenger( )
{
	m_fMinRange1		= 0;// No minimum range. 
	m_fMaxRange1		= 1400;

	m_bAltFiresUnderwater = false;
	
	m_iClip1 = sk_avenger_max_ammo.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAvenger::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponAvenger::Equip( CBaseCombatCharacter *pOwner )
{
	if( pOwner->Classify() == CLASS_PLAYER_ALLY )
	{
		m_fMaxRange1 = 3000;
	}
	else
	{
		m_fMaxRange1 = 1400;
	}
	
	BaseClass::Equip( pOwner );
}

bool CWeaponAvenger::Deploy( void )
{
	SetThink( &CWeaponAvenger::RechargeAmmoThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponAvenger::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponAvenger::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;
	
	if ( pOwner->GetAmmoCount( m_iSecondaryAmmoType ) > 0 ) {
		
#ifdef CLIENT_DLL
		// Play reload
		WeaponSound( RELOAD );
#endif
		SendWeaponAnim( ACT_VM_RELOAD );
		
		// Play the player's reload animation
		if ( pOwner->IsPlayer() )
		{
			( ( CBasePlayer * )pOwner)->SetAnimation( PLAYER_RELOAD );
		}
		
		MDLCACHE_CRITICAL_SECTION();
		float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
		pOwner->SetNextAttack( flSequenceEndTime );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;
		
		m_bInReload = true;
		
		return true;
	}
	else {
		return false;
	}
}

void CWeaponAvenger::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return;
	
	m_iClip1 = sk_avenger_max_ammo.GetInt();
	pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
	
	m_bInReload = false;
	
	if ( m_bOverheated == true ) {
		OverheatClearThink();
	}
}

bool CWeaponAvenger::ReloadOrSwitchWeapons( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAvenger::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

#define FIRING_DISCHARGE_RATE			(1.0f * 10)
#define AMMO_UPDATE_FRAME_MULTIPLIER	10

void CWeaponAvenger::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	if ( m_iClip1 > 0 && m_bOverheated == false ) {
		
		m_nShotsFired++;
		
		pPlayer->DoMuzzleFlash();
		
		// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems,
		// especially if the weapon we're firing has a really fast rate of fire.
		int iBulletsToFire = 0;
		float fireRate = GetFireRate();
		
		// MUST call sound before removing a round from the clip of a CHLMachineGun
		while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
		{
			WeaponSound(SINGLE, m_flNextPrimaryAttack);
			m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
			iBulletsToFire++;
		}
		
		// Make sure we don't fire more than the amount in the clip
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		
		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
		
		// Fire the bullets
		FireBulletsInfo_t info;
		info.m_iShots = iBulletsToFire;
		info.m_vecSrc = pPlayer->Weapon_ShootPosition( );
		info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
		info.m_vecSpread = pPlayer->GetAttackSpread( this );
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType = m_iPrimaryAmmoType;
		info.m_iTracerFreq = 2;
		FireBullets( info );
		
		//Factor in the view kick
		AddViewKick();
		
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
		
		if (!m_iClip1 && pPlayer->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
		{
			// HEV suit - indicate out of ammo condition
			pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
		}
		
		SendWeaponAnim( GetPrimaryAttackActivity() );
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
		// Register a muzzleflash for the AI
		pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
		
		RemoveAmmo( FIRING_DISCHARGE_RATE * iBulletsToFire );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAvenger::SecondaryAttack( void )
{
	
}

//-----------------------------------------------------------------------------
// Removes the ammo...
//-----------------------------------------------------------------------------
void CWeaponAvenger::RemoveAmmo( float flAmmoAmount )
{
	m_flDrainRemainder += flAmmoAmount;
	int nAmmoToRemove = (int)m_flDrainRemainder;
	m_flDrainRemainder -= nAmmoToRemove;
	m_iClip1 -= nAmmoToRemove;
	if ( m_iClip1 < 0 )
	{
		m_iClip1 = 0;
		m_flDrainRemainder = 0.0f;
	}
	
	if ( m_iClip1 > 0 ) {
		
		SetThink( &CWeaponAvenger::RechargeAmmoThink );
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
	}
	else {
		// Overheat
		
		m_bOverheated = true;
		SetThink( &CWeaponAvenger::OverheatClearThink );
		SetNextThink( gpGlobals->curtime + sk_avenger_overheat_delay.GetFloat() );
	}
}

//-----------------------------------------------------------------------------
// Recharges the ammo...
//-----------------------------------------------------------------------------
void CWeaponAvenger::RechargeAmmoThink(void)
{
	int nMaxAmmo = sk_avenger_max_ammo.GetInt();
	if ( m_iClip1 == nMaxAmmo || m_bOverheated == true || m_bInReload == true ) {
		
		SetThink( NULL );
	}
	else {
		
		float flRechargeRate = sk_avenger_recharge_rate.GetInt();
		float flChargeAmount = flRechargeRate * (gpGlobals->frametime * AMMO_UPDATE_FRAME_MULTIPLIER);
		if ( m_flDrainRemainder != 0.0f )
		{
			if ( m_flDrainRemainder >= flChargeAmount )
			{
				m_flDrainRemainder -= flChargeAmount;
				return;
			}
			else
			{
				flChargeAmount -= m_flDrainRemainder;
				m_flDrainRemainder = 0.0f;
			}
		}
		
		m_flChargeRemainder += flChargeAmount;
		int nAmmoToAdd = (int)m_flChargeRemainder;
		m_flChargeRemainder -= nAmmoToAdd;
		m_iClip1 += nAmmoToAdd;
		if ( m_iClip1 > nMaxAmmo )
		{
			m_iClip1 = nMaxAmmo;
			m_flChargeRemainder = 0.0f;
		}
		
		SetThink( &CWeaponAvenger::RechargeAmmoThink );
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime * AMMO_UPDATE_FRAME_MULTIPLIER );
	}
}

void CWeaponAvenger::OverheatClearThink( void )
{
	m_bOverheated = false;
	SetThink( &CWeaponAvenger::RechargeAmmoThink );
	SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponAvenger::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponAvenger::GetProficiencyValues()
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
