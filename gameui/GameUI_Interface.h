//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the interface that the GameUI dll exports
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEUI_INTERFACE_H
#define GAMEUI_INTERFACE_H
#pragma once

#include "GameUI/IGameUI.h"

#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>

class IGameClientExports;

//-----------------------------------------------------------------------------
// Purpose: Implementation of GameUI's exposed interface 
//-----------------------------------------------------------------------------
class CGameUI : public IGameUI
{
public:
	CGameUI();
	~CGameUI();

	virtual void Initialize( CreateInterfaceFn appFactory );
	virtual void Connect( CreateInterfaceFn gameFactory );
	virtual void Start();
	virtual void Shutdown();
	virtual void RunFrame();
	virtual void PostInit();

	// plays the startup mp3 when GameUI starts
	void PlayGameStartupSound();

	// Engine wrappers for activating / hiding the gameUI
	void ActivateGameUI();
	void HideGameUI();

	// Toggle allowing the engine to hide the game UI with the escape key
	void PreventEngineHideGameUI();
	void AllowEngineHideGameUI();

	virtual void SetLoadingBackgroundDialog( vgui::VPANEL panel );

	// Bonus maps interfaces
	virtual void BonusMapUnlock( const char *pchFileName = NULL, const char *pchMapName = NULL );
	virtual void BonusMapComplete( const char *pchFileName = NULL, const char *pchMapName = NULL );
	virtual void BonusMapChallengeUpdate( const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest );
	void BonusMapChallengeNames( char *pchFileName, char *pchMapName, char *pchChallengeName ) override;
	virtual void BonusMapChallengeObjectives( int &iBronze, int &iSilver, int &iGold );
	virtual void BonusMapDatabaseSave( void );
	virtual int BonusMapNumAdvancedCompleted( void );
	virtual void BonusMapNumMedals( int piNumMedals[ 3 ] );

	// notifications
	virtual void OnGameUIActivated();
	virtual void OnGameUIHidden();
	virtual void OLD_OnConnectToServer( const char *game, int IP, int port );	// OLD: use OnConnectToServer2
	virtual void OnConnectToServer2( const char *game, int IP, int connectionPort, int queryPort );
	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure );
	virtual void OnLevelLoadingStarted( bool bShowProgressDialog );
	virtual void OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason );
	virtual void OnDisconnectFromServer_OLD( uint8 eSteamLoginFailure, const char *username ) { OnDisconnectFromServer( eSteamLoginFailure ); }

	// progress
	virtual bool UpdateProgressBar(float progress, const char *statusText);
	// Shows progress desc, returns previous setting... (used with custom progress bars )
	virtual bool SetShowProgressText( bool show );

	// brings up the new game dialog
	virtual void ShowNewGameDialog( int chapter );

	// Xbox 360
	virtual void SessionNotification( const int notification, const int param = 0 );
	virtual void SystemNotification( const int notification );
	virtual void ShowMessageDialog( const uint nType, vgui::Panel *pOwner = NULL );
	virtual void CloseMessageDialog( const uint nType = 0 );
	virtual void UpdatePlayerInfo( uint64 nPlayerId, const char *pName, int nTeam, byte cVoiceState, int nPlayersNeeded, bool bHost );
	virtual void SessionSearchResult( int searchIdx, void *pHostData, XSESSION_SEARCHRESULT *pResult, int ping );
	virtual void OnCreditsFinished( void );

	// X360 Storage device validation:
	//		returns true right away if storage device has been previously selected.
	//		otherwise returns false and will set the variable pointed by pStorageDeviceValidated to 1
	//				  once the storage device is selected by user.
	virtual bool ValidateStorageDevice( int *pStorageDeviceValidated );

	virtual void SetProgressOnStart();

	virtual void OnConfirmQuit( void );

	virtual bool IsMainMenuVisible( void );

	// Client DLL is providing us with a panel that it wants to replace the main menu with
	virtual void SetMainMenuOverride( vgui::VPANEL panel );
	// Client DLL is telling us that a main menu command was issued, probably from its custom main menu panel
	virtual void SendMainMenuCommand( const char *pszCommand );

	void BonusMapChallengeNames( OUT_Z_CAP(fileSize) char *pchFileName, intp fileSize,
		OUT_Z_CAP(mapSize) char *pchMapName, intp mapSize,
		OUT_Z_CAP(challengeSize) char *pchChallengeName, intp challengeSize ) override;

	// state
	bool IsInLevel();
	bool IsInBackgroundLevel();
	bool IsInMultiplayer();
	bool IsInReplay();
	bool IsConsoleUI();
	bool HasSavedThisMenuSession();
	void SetSavedThisMenuSession( bool bState );

	void ShowLoadingBackgroundDialog();
	void HideLoadingBackgroundDialog();
	bool HasLoadingBackgroundDialog();

private:
	void SendConnectedToGameMessage();

	virtual void StartProgressBar();
	virtual bool ContinueProgressBar(float progressFraction);
	virtual void StopProgressBar(bool bError, const char *failureReason, const char *extendedReason = NULL);
	virtual bool SetProgressBarStatusText(const char *statusText);

	//!! these functions currently not implemented
	virtual void SetSecondaryProgressBar(float progress /* range [0..1] */);
	virtual void SetSecondaryProgressBarText(const char *statusText);

	bool FindPlatformDirectory(char *platformDir, int bufferSize);
	void GetUpdateVersion( char *pszProd, char *pszVer);
	void ValidateCDKey();

	CreateInterfaceFn m_GameFactory;

	bool m_bPlayGameStartupSound : 1;
	bool m_bTryingToLoadFriends : 1;
	bool m_bActivatedUI : 1;
	bool m_bHasSavedThisMenuSession : 1;
	bool m_bOpenProgressOnStart : 1;

	int m_iGameIP;
	int m_iGameConnectionPort;
	int m_iGameQueryPort;
	
	int m_iFriendsLoadPauseFrames;

	char m_szPreviousStatusText[128];
	char m_szPlatformDir[MAX_PATH];

	vgui::DHANDLE<class CCDKeyEntryDialog> m_hCDKeyEntryDialog;
};

// Purpose: singleton accessor
extern CGameUI &GameUI();

// expose client interface
extern IGameClientExports *GameClientExports();


#endif // GAMEUI_INTERFACE_H
