#if defined _steamid_included
#endinput
#endif
#define _steamid_included

native bool SteamID_Convert(char[] destinationString, int destinationSize, AuthIdType destinationFormat = AuthId_SteamID64, const char[] sourceString, AuthIdType sourceFormat = AuthId_Engine);

public Extension __ext_SteamID =
{
	name = "SteamID",
	file = "steamid.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
}
