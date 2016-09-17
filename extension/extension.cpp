/**
 * vim: set ts=4 :
 * =============================================================================
 * Steam ID Extension
 * Copyright (C) 2016 Forward Command Post.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

#include <cinttypes>
#include <limits>
#include <regex>

#include "steam/steamclientpublic.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SteamID g_SteamID;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_SteamID);

#define STEAMID_ENGINE 0
#define STEAMID_STEAM2 1
#define STEAMID_STEAM3 2
#define STEAMID_64BIT 3

template <typename T> constexpr size_t NumericStringMaxLength() {
    return std::numeric_limits<T>::digits10 + 1 + std::numeric_limits<T>::is_signed + 1;
}

bool Parse64BitSteamID(const char *steamIDString, CSteamID &steamID) {
    static std::regex steamIDRegex = std::regex("(\\d+)", std::regex::optimize);

    std::cmatch steamIDParts;

    if (!std::regex_match(steamIDString, steamIDParts, steamIDRegex)) {
        return false;
    }

    steamID.SetFromUint64(strtoull(steamIDParts[1].str(), NULL, 10));

    return true;
}

bool ParseSteam2SteamID(const char *steamIDString, CSteamID &steamID) {
    static std::regex steamIDRegex = std::regex("STEAM_(\\d):(\\d):(\\d+)", std::regex::optimize);

    std::cmatch steamIDParts;

    if (!std::regex_match(steamIDString, steamIDParts, steamIDRegex)) {
        return false;
    }

    uint32_t accountID = (strtoull(steamIDParts[3].str(), NULL, 10) * 2) + strtoul(steamIDParts[2].str(), NULL, 10);
    EUniverse universe = (EUniverse) strtoul(steamIDParts[1].str(), NULL, 10);
    if (universe == k_EUniverseInvalid) {
        universe = k_EUniversePublic;
    }

    steamID.Set(accountID, universe, k_EAccountTypeIndividual);

    return true;
}

inline void GetSteamIDParts(std::cmatch steamIDParts, uint32_t &accountID, uint32_t &instance, EUniverse &universe, EAccountType &accountType) {
    accountType = k_EAccountTypeInvalid;
    instance = 0;

    switch (*(steamIDParts[1].str())) {
        case 'I':
            accountType = k_EAccountTypeInvalid;
            break;
        case 'U':
            accountType = k_EAccountTypeIndividual;
            break;
        case 'M':
            accountType = k_EAccountTypeMultiseat;
            break;
        case 'G':
            accountType = k_EAccountTypeGameServer;
            break;
        case 'A':
            accountType = k_EAccountTypeAnonGameServer;
            break;
        case 'P':
            accountType = k_EAccountTypePending;
            break;
        case 'C':
            accountType = k_EAccountTypeContentServer;
            break;
        case 'g':
            accountType = k_EAccountTypeClan;
            break;
        case 'T':
            accountType = k_EAccountTypeChat;
            break;
        case 'L':
            accountType = k_EAccountTypeChat;
            instance |= k_EChatInstanceFlagLobby;
            break;
        case 'c':
            accountType = k_EAccountTypeChat;
            instance |= k_EChatInstanceFlagClan;
            break;
        case 'a':
            accountType = k_EAccountTypeAnonUser;
            break;
        default:
            // ???
            break;
    }

    universe = (EUniverse) strtoul(steamIDParts[2].str(), NULL, 10);
    accountID = strtoull(steamIDParts[3].str(), NULL, 10);
}

bool ParseSteam3SteamID(const char *steamIDString, CSteamID &steamID) {
    static std::regex steamIDMainRegex = std::regex("\\[([a-zA-Z]):(\\d):(\\d+)\\]", std::regex::optimize);
    static std::regex steamIDSecondaryRegex = std::regex("\\[([a-zA-Z]):(\\d):(\\d+):(\\d+)\\]", std::regex::optimize);

    std::cmatch steamIDParts;

    uint32_t accountID;
    uint32_t instance;
    EUniverse universe;
    EAccountType accountType;

    if (std::regex_match(steamIDString, steamIDParts, steamIDMainRegex)) {
        GetSteamIDParts(steamIDParts, accountID, instance, universe, accountType);

        if (accountType == k_EAccountTypeChat) {
            steamID.Set(accountID, universe, accountType);
        }
        else {
            steamID.InstancedSet(accountID, instance, universe, accountType);
        }

        return true;
    }
    else if (std::regex_match(steamIDString, steamIDParts, steamIDSecondaryRegex)) {
        GetSteamIDParts(steamIDParts, accountID, instance, universe, accountType);

        instance |= strtoul(steamIDParts[4].str(), NULL, 10);

        steamID.InstancedSet(accountID, instance, universe, accountType);

        return true;
    }
    else {
        return false;
    }
}

void Render64BitSteamID(const CSteamID &steamID, char steamIDString[], size_t steamIDStringSize) {
	V_snprintf(steamIDString, steamIDStringSize, "%" PRIu64, steamID.ConvertToUint64());
}

void RenderSteam2SteamID(const CSteamID &steamID, char steamIDString[], size_t steamIDStringSize) {
	V_snprintf(steamIDString, steamIDStringSize, "STEAM_0:%" PRIu8 ":%" PRIu32, steamID.GetAccountID() % 2, steamID.GetAccountID() / 2);
}

void RenderSteam3SteamID(const CSteamID &steamID, char steamIDString[], size_t steamIDStringSize) {
    char accountType = 'I';

    switch (steamID.GetEAccountType()) {
        case k_EAccountTypeInvalid:
            accountType = 'I';
            break;
        case k_EAccountTypeIndividual:
            accountType = 'U';
            break;
        case k_EAccountTypeMultiseat:
            accountType = 'M';
            break;
        case k_EAccountTypeGameServer:
            accountType = 'G';
            break;
        case k_EAccountTypeAnonGameServer:
            accountType = 'A';
            break;
        case k_EAccountTypePending:
            accountType = 'P';
            break;
        case k_EAccountTypeContentServer:
            accountType = 'C';
            break;
        case k_EAccountTypeClan:
            accountType = 'g';
            break;
        case k_EAccountTypeChat:
            if (steamID.GetUnAccountInstance() & k_EChatInstanceFlagClan) {
                accountType = 'c';
            }
            else if ((steamID.GetUnAccountInstance() & k_EChatInstanceFlagLobby) || (steamID.GetUnAccountInstance() & k_EChatInstanceFlagMMSLobby)) {
                accountType = 'L';
            }
            else {
                accountType = 'T';
            }
            break;
        case k_EAccountTypeAnonUser:
            accountType = 'a';
            break;
        case k_EAccountTypeConsoleUser:
        default:
            // ???
            accountType = 'i';
            break;
    }

    if ((steamID.GetEAccountType() == k_EAccountTypeIndividual && steamID.GetUnAccountInstance() != k_unSteamUserDesktopInstance) || steamID.GetEAccountType() == k_EAccountTypeMultiseat || steamID.GetEAccountType() == k_EAccountTypeAnonGameServer) {
        V_snprintf(steamIDString, steamIDStringSize, "[%c:%" PRIu8 ":%" PRIu32 ":%" PRIu32 "]", accountType, steamID.GetEUniverse(), steamID.GetAccountID(), steamID.GetUnAccountInstance());
    }
    else {
        V_snprintf(steamIDString, steamIDStringSize, "[%c:%" PRIu8 ":%" PRIu32 "]", accountType, steamID.GetEUniverse(), steamID.GetAccountID());
    }
}

cell_t Native_Convert(IPluginContext *pContext, const cell_t *params) {
    CSteamID steamID;

    char *source;

    pContext->LocalToString(params[4], &src);

    bool converted = false;

    if (params[5] == STEAMID_STEAM2) {
        converted = ParseSteam2SteamID(source, steamID);
    }
    else if (params[5] == STEAMID_STEAM3) {
        converted = ParseSteam3SteamID(source, steamID);
    }
    else if (params[5] == STEAMID_STEAM64) {
        converted = Parse64BitSteamID(source, steamID);
    }
    else {
        converted = Parse64BitSteamID(source, steamID) || ParseSteam3SteamID(source, steamID) || ParseSteam2SteamID(source, steamID);
    }

    if (!converted) {
        return false;
    }

    char destination[128];

    if (params[3] == STEAMID_STEAM2) {
        RenderSteam2SteamID(steamID, destination, sizeof(destination));
    }
    else if (params[3] == STEAMID_STEAM3) {
        RenderSteam3SteamID(steamID, destination, sizeof(destination));
    }
    else if (params[3] == STEAMID_STEAM64) {
        Render64BitSteamID(steamID, destination, sizeof(destination));
    }
    else {
        return false;
    }

    pContext->StringToLocal(params[1], params[2], destination);

    return true;
}

const sp_nativeinfo_t SteamIDNatives[] = {
	{"SteamID_Convert", Native_Convert},
	{NULL, NULL},
};

void Sample::SDK_OnAllLoaded() {
	sharesys->AddNatives(myself, SteamIDNatives);
}
