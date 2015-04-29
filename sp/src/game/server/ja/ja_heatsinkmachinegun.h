//
//  ja_heatsinkmachinegun.h
//  games
//
//  Created by Jeremy Agostino on 4/29/15.
//
//

#ifndef games_ja_heatsinkmachinegun_h
#define games_ja_heatsinkmachinegun_h

#include "basehlcombatweapon.h"
#include "ja/ja_weaponparse.h"

/*
 * New "Heatsink Machine Gun" class
 */
class CHeatsinkMachineGun : public CHLMachineGun
{
	DECLARE_CLASS( CHeatsinkMachineGun, CHLMachineGun );
public:
	
	CHeatsinkMachineGun( void );
	
	DECLARE_SERVERCLASS();
	
	float	GetFireRate( void );
	float	GetHeatsinkRechargeRate( void ) const;
	float	GetHeatsinkDrainRate( void ) const;
	float	GetHeatsinkCooloffDelay( void ) const;
	
	void	RemoveAmmo( float flAmmoAmount );
	void	RechargeAmmoThink( void );
	void	OverheatClearThink( void );
	
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	
	bool	Deploy( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );
	void	FinishReload( void );
	bool	ReloadOrSwitchWeapons( void );
	
	//	bool	UsesClipsForAmmo1( void ) { return false; }
	
	DECLARE_DATADESC();
	
protected:
	
	float	m_flChargeRemainder;
	float	m_flDrainRemainder;
	
	bool	m_bOverheated;
	bool	m_bCooling;
};

#endif
