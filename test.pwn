#include <a_samp>
#include "not_menu.inc"

main() {}

MENU:testMenu(playerid, response, listitem)
{
	#pragma unused playerid, response, listitem
	return 1;
}

public OnPlayerSpawn(playerid)
{
	ShowPlayerMenu(playerid, testMenu, "Test Menu");
	AddPlayerMenuItem(playerid, "Item 1", "Description 1");
	AddPlayerMenuItemModel(playerid, "Item 2", "Description 2", 355, .rz = -45.0, .modelinfo = "Weapon");
	FinalizePlayerMenu(playerid);

	if(IsPlayerInMenu(playerid))
	{
		new id[32];
		GetPlayerMenuID(playerid, id, sizeof(id));
		new page = GetPlayerMenuPage(playerid);
		new total = GetPlayerMenuTotalItems(playerid);
		new item = GetPlayerMenuListitem(playerid);
		#pragma unused page, total, item
		SetPlayerMenuListitem(playerid, 0);
		RemovePlayerMenuItemModel(playerid, 1);
		HidePlayerMenu(playerid);
	}
	return 1;
}
