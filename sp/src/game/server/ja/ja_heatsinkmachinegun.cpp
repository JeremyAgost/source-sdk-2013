//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

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

IMPLEMENT_SERVERCLASS_ST(CHeatsinkMachineGun, DT_HeatsinkMachineGun)
END_SEND_TABLE()

BEGIN_DATADESC( CHeatsinkMachineGun )

	DEFINE_FIELD( m_flChargeRemainder, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDrainRemainder, FIELD_FLOAT ),

	DEFINE_FIELD( m_bOverheated, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCooling, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( RechargeAmmoThink ),
	DEFINE_THINKFUNC( OverheatClearThink ),

END_DATADESC()

//=========================================================
CHeatsinkMachineGun::CHeatsinkMachineGun( )
{
	m_iClip1 = GetMaxClip1();
}

float CHeatsinkMachineGun::GetFireRate( void )
{
	return ((CCustomWeaponInfo &)GetWpnData()).m_flCycleTime;
}

float CHeatsinkMachineGun::GetHeatsinkCooloffDelay( void ) const
{
	return ((CCustomWeaponInfo &)GetWpnData()).m_flHeatsinkCooloffDelay;
}

float CHeatsinkMachineGun::GetHeatsinkDrainRate( void ) const
{
	return ((CCustomWeaponInfo &)GetWpnData()).m_flHeatsinkDrainRate;
}

float CHeatsinkMachineGun::GetHeatsinkRechargeRate( void ) const
{
	return ((CCustomWeaponInfo &)GetWpnData()).m_flHeatsinkRechargeRate;
}

void CHeatsinkMachineGun::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "SuitRecharge.Deny" );
	PrecacheScriptSound( "AlyxEmp.Charge" );
}

void CHeatsinkMachineGun::Equip( CBaseCombatCharacter *pOwner )
{
	m_iClip1 = GetMaxClip1();
	
	BaseClass::Equip( pOwner );
}

bool CHeatsinkMachineGun::Deploy( void )
{
	SetThink( &CHeatsinkMachineGun::RechargeAmmoThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHeatsinkMachineGun::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;
	
	if ( m_iClip1 < GetMaxClip1() && pOwner->GetAmmoCount( m_iSecondaryAmmoType ) > 0 ) {
		
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

void CHeatsinkMachineGun::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return;
	
	// Fill the energy mag
	m_iClip1 = GetMaxClip1();
	
	if ( m_iState == WEAPON_IS_ACTIVE ) {
		// Only consume a heatsink if the weapon was reloaded while it was out
		pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
	}
	
	m_bInReload = false;
	
	// Clear any overheat condition
	m_bOverheated = false;
	m_bCooling = false;
}

bool CHeatsinkMachineGun::ReloadOrSwitchWeapons( void )
{
	return false;
}

void CHeatsinkMachineGun::ItemHolsterFrame( void )
{
	// Don't call baseclass because we don't want to automatically reload while weapon is in background

	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// Check if the cooloff needs to start
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > GetHeatsinkCooloffDelay() )
	{
		if (m_bOverheated) {

			// Manually trigger the cooloff
			OverheatClearThink();

			// Add one energy point so weapon becomes deployable again
			m_iClip1++;
		}
		else {
			RechargeAmmoThink();
		}

		m_flHolsterTime = gpGlobals->curtime;
	}
}

void CHeatsinkMachineGun::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	if ( m_iClip1 > 0 && m_bOverheated == false && m_bCooling == false ) {
		
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
		
		SendWeaponAnim( GetPrimaryAttackActivity() );
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
		// Register a muzzleflash for the AI
		pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
		
		RemoveAmmo( GetHeatsinkDrainRate() * iBulletsToFire );
	}
	else {
		
		if (m_flNextEmptySoundTime < gpGlobals->curtime)
		{
			WeaponSound(EMPTY);
			EmitSound( "SuitRecharge.Deny" );
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
		}
		
		// Advance primaryAttack time so we don't make up for lost time if the trigger was held during a cooldown
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeatsinkMachineGun::SecondaryAttack( void )
{
	
}

//-----------------------------------------------------------------------------
// Removes the ammo...
//-----------------------------------------------------------------------------
void CHeatsinkMachineGun::RemoveAmmo( float flAmmoAmount )
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
		
		SetThink( &CHeatsinkMachineGun::RechargeAmmoThink );
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
	}
	else {
		// Overheat
		// TODO sound and effect
//		WeaponSound( SPECIAL1 );
		EmitSound( "AlyxEmp.Charge" );
//		EmitSound( "AlyxEmp.Discharge" );
		
		m_bOverheated = true;
		SetThink( &CHeatsinkMachineGun::OverheatClearThink );
		SetNextThink( gpGlobals->curtime + GetHeatsinkCooloffDelay() );
	}
}

#define RECHARGE_FRAME_RATE	5

//-----------------------------------------------------------------------------
// Recharges the ammo...
//-----------------------------------------------------------------------------
void CHeatsinkMachineGun::RechargeAmmoThink(void)
{
	int nMaxAmmo = GetMaxClip1();
	if ( m_iClip1 == nMaxAmmo || m_bOverheated == true || m_bInReload == true ) {
		
		SetThink( NULL );
	}
	else {
		
		float flChargeAmount = GetHeatsinkRechargeRate();
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
		
		SetThink( &CHeatsinkMachineGun::RechargeAmmoThink );
		SetNextThink( gpGlobals->curtime + gpGlobals->frametime * RECHARGE_FRAME_RATE );
	}
	
	if ( m_bCooling == true && m_iClip1 == nMaxAmmo ) {
		// Finished cooldown
		m_bCooling = false;
		
#ifdef CLIENT_DLL
		WeaponSound( SPECIAL2 );
#endif
	}
}

void CHeatsinkMachineGun::OverheatClearThink( void )
{
	// Stop overheating so ammo recharges
	m_bOverheated = false;
	
	// Cooling down so can't fire
	m_bCooling = true;
	
	SetThink( &CHeatsinkMachineGun::RechargeAmmoThink );
	SetNextThink( gpGlobals->curtime + gpGlobals->frametime );
}
