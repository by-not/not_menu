# not_menu

> **Language:** English | [Español](README.es.md)

An interactive **PlayerTextDraw** menu library for **SA-MP** and **open.mp** with keyboard + mouse navigation, automatic pagination, dynamic info box, and a 3D model preview panel.

## Features

- **Dual navigation:** Up/Down/Space/Enter on keyboard, plus mouse support (1st click highlights, 2nd click selects).
- **Automatic pagination:** Clickable `Next >>` / `<< Back` buttons and keyboard page cycling.
- **Dynamic info box:** Automatically scales box height based on line wrapping using GTA:SA font character widths.
- **3D model preview panel:** Optional side panel (`Font 5`) with custom rotation, zoom, vehicle colors, and description per item.
- **Clean callback syntax:** `MENU:myMenu(playerid, response, listitem)` macro.
- **Zero external dependencies:** Requires only `<a_samp>`, with standard ALS hooks compatible with YSI (`y_hooks`).

## Installation

1. Copy `not_menu.inc` into your `pawno/include/` (or `qawno/include/`) folder.
2. Include it after `<a_samp>`:

```pawn
#include <a_samp>

// Optional configuration (define before including):
#define MENU_MAX_LISTITEMS          24
#define MENU_MAX_LISTITEM_SIZE      128
#define MENU_MAX_LISTITEMS_PERPAGE  10
#define MENU_CURSOR_COLOR           0x5A6FE7FF
#define MENU_PREVIEW_TITLE          "Preview"

#include <not_menu>
```

## Quick Start

```pawn
stock OpenShopMenu(playerid)
{
    ShowPlayerMenu(playerid, shopMenu, "Shop", .type = "BUY ITEM", .captionBoxColor = 0xAC3E36BB);
    AddPlayerMenuItemModel(playerid, "Burger", "Restores 50 HP");
    AddPlayerMenuItemModel(playerid, "AK-47", "Assault rifle", 355, .rz = -45.0, .modelinfo = "Damage: High");
    FinalizePlayerMenu(playerid);
}

MENU:shopMenu(playerid, response, listitem)
{
    if(response == MENU_RESPONSE_SELECT)
    {
        // listitem is the 0-based global item index
    }
    else if(response == MENU_RESPONSE_CLOSE)
    {
        // Player closed the menu
    }
    return 1;
}
```

## API Summary

- `ShowPlayerMenu(playerid, menuid, const caption[], ...)`
- `AddPlayerMenuItemModel(playerid, const text[], const info[] = "", modelid = -1, ...)`
- `AddPlayerMenuItem(playerid, const text[], const info[] = "")`
- `FinalizePlayerMenu(playerid)`
- `RemovePlayerMenuItemModel(playerid, listitem)`
- `HidePlayerMenu(playerid)`
- `IsPlayerInMenu(playerid)`
- `GetPlayerMenuListitem(playerid)`, `GetPlayerMenuPage(playerid)`, `GetPlayerMenuTotalItems(playerid)`, `GetPlayerMenuID(playerid)`, `SetPlayerMenuListitem(playerid, listitem)`

## Credits

- Written and refactored by **not**.
- Character width table by **Nero_3D** (`td-string-width`).
- Menu UI architecture inspired by **Gammix**.

## License

Released under the [MIT License](LICENSE).
