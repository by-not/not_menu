/*
	=====================================================
	NOT_MENU.INC - Interactive TextDraw Menu System
	=====================================================
	Author: not
	Version: 1.1
	Date: 2026
	License: MIT
	Credits / Inspiration: Nero_3D (td-string-width), Gammix (menu UI style)

	DESCRIPTION:
	Complete PlayerTextDraw-based menu system for SA-MP and open.mp.
	Supports keyboard (Up/Down/Space/Enter) and mouse (1st click highlights,
	2nd click selects) navigation, automatic pagination, dynamic-height item
	info box, and an optional 3D model preview side panel (Font 5).

	MAIN FEATURES:
	- Dual keyboard and mouse navigation with clickable Next/Back buttons
	- Automatic pagination with configurable items per page
	- Dynamic-height item info box calculated via character width tables
	- Optional 3D model preview panel with custom rotation, zoom, colors, and description
	- Flicker-free creation and clean MENU: callback macro
	- Standard ALS hooks compatible with YSI (y_hooks) and other includes
	- Built-in Latin character conversion for SA-MP TextDraws (pure ASCII source)

	BASIC CONFIGURATION:
	```pawn
	#define MENU_MAX_LISTITEMS          24         // Max items per menu (default: 24)
	#define MENU_MAX_LISTITEM_SIZE      128        // Max chars per item (default: 128)
	#define MENU_MAX_LISTITEMS_PERPAGE  10         // Items per page (default: 10)
	#define MENU_ITEM_PADDING           12.0       // Vertical item spacing (default: 12.0)
	#define MENU_INFO_MAX_WIDTH         600        // Info box wrap width (default: 600)
	#define MENU_CURSOR_COLOR           0x5A6FE7FF // Cursor hover highlight color
	#define MENU_PREVIEW_TITLE          "Preview"  // 3D preview panel header
	#define MENU_SOUND_UPDOWN           1054       // Navigation sound
	#define MENU_SOUND_CLOSE            1084       // Close sound
	#define MENU_SOUND_SELECT           1083       // Select sound
	#include <not_menu>
	```

	USAGE EXAMPLE:
	```pawn
	ShowPlayerMenu(playerid, shopMenu, "Shop", .type = "BUY ITEM", .captionBoxColor = 0xAC3E36BB);
	AddPlayerMenuItemModel(playerid, "Burger", "Restores 50 HP");
	AddPlayerMenuItemModel(playerid, "AK-47", "Assault rifle", 355, .rz = -45.0);
	FinalizePlayerMenu(playerid);

	MENU:shopMenu(playerid, response, listitem)
	{
	    if(response == MENU_RESPONSE_SELECT) { // listitem = global 0-based index }
	    if(response == MENU_RESPONSE_CLOSE)  { // player closed menu }
	    return 1;
	}
	```

	PUBLIC API:
	- ShowPlayerMenu, AddPlayerMenuItemModel, AddPlayerMenuItem, FinalizePlayerMenu
	- RemovePlayerMenuItemModel, HidePlayerMenu, IsPlayerInMenu
	- GetPlayerMenuListitem, GetPlayerMenuPage, GetPlayerMenuTotalItems
	- GetPlayerMenuID, SetPlayerMenuListitem

	RESPONSES:
	- MENU_RESPONSE_UP      -> navigated up
	- MENU_RESPONSE_DOWN    -> navigated down
	- MENU_RESPONSE_SELECT  -> selected item (SPACE or 2nd mouse click)
	- MENU_RESPONSE_CLOSE   -> closed menu (ENTER or ESC)

	COMPATIBILITY:
	- SA-MP 0.3.7 and open.mp
	- Zero external dependencies (requires only <a_samp>)
	- Compatible with not_notify.inc and not_taskbar.inc (shared char-width guard)

	=====================================================
*/

#if defined _not_menu_included
	#endinput
#endif
#define _not_menu_included

#include <a_samp>

// ----------------------------------------------------------------------------
// Configuracion - Sonidos
// ----------------------------------------------------------------------------

#if !defined MENU_SOUND_UPDOWN
	#define MENU_SOUND_UPDOWN (1054)
#endif

#if !defined MENU_SOUND_CLOSE
	#define MENU_SOUND_CLOSE (1084)
#endif

#if !defined MENU_SOUND_SELECT
	#define MENU_SOUND_SELECT (1083)
#endif

// ----------------------------------------------------------------------------
// Configuracion - Limites
// ----------------------------------------------------------------------------

#if !defined MENU_MAX_LISTITEMS
	#define MENU_MAX_LISTITEMS (24)
#endif

#if !defined MENU_MAX_LISTITEM_SIZE
	#define MENU_MAX_LISTITEM_SIZE (128)
#endif

#if !defined MENU_MAX_LISTITEMS_PERPAGE
	#define MENU_MAX_LISTITEMS_PERPAGE (10)
#endif

#if !defined MENU_ITEM_PADDING
	#define MENU_ITEM_PADDING (12.0)
#endif

#if !defined MENU_CURSOR_COLOR
	#define MENU_CURSOR_COLOR (0x5A6FE7FF)
#endif

#if !defined MENU_PREVIEW_TITLE
	#define MENU_PREVIEW_TITLE "Preview"
#endif

// ----------------------------------------------------------------------------
// Macros y constantes
// ----------------------------------------------------------------------------

#define INVALID_MENU_ID				(-1)

// Calcula cuantas paginas se necesitan dado N items y M items por pagina
#define MENU_COUNT_PAGES(%0,%1) \
	(((%0) - 1) / (%1) + 1)

// Declara el forward y public del callback de un menu
// Uso: MENU:miMenu(playerid, response, listitem) { }
#define MENU:%0(%1) \
	forward menu_%0(%1); public menu_%0(%1)

// Alias con M mayuscula (compatibilidad con codigo anterior)
#define Menu:%0(%1) \
	MENU:%0(%1)

// Acceso limpio a los campos de MenuInfo
#define MenuInfo::%0[%1]	g_MenuInfo[%1][%0]

// ----------------------------------------------------------------------------
// Enumeraciones
// ----------------------------------------------------------------------------

enum
{
	MENU_RESPONSE_UP,
	MENU_RESPONSE_DOWN,
	MENU_RESPONSE_SELECT,
	MENU_RESPONSE_CLOSE
};

static enum E_PLAYER_MENU
{
	E_PLAYER_MENU_ID[32],
	E_PLAYER_MENU_PAGE,
	E_PLAYER_MENU_LISTITEM,
	E_PLAYER_MENU_TOTAL_LISTITEMS,
	E_PLAYER_MENU_TICKCOUNT,
	E_PLAYER_MENU_NEXT_BTN[MENU_MAX_LISTITEM_SIZE],
	E_PLAYER_MENU_BACK_BTN[MENU_MAX_LISTITEM_SIZE],
	E_PLAYER_MENU_CAPTION_BOX_COLOR
};

static enum E_MENU_TD
{
	E_MENU_TD_LISTITEM_COUNT,
	E_MENU_TD_LISTITEMS[MENU_MAX_LISTITEMS_PERPAGE],
	E_MENU_TD_INFO_BOX,
	E_MENU_TD_INFO_ICON,
	E_MENU_TD_INFO_TEXT,
	E_MENU_TD_NAV_BAR,
	E_MENU_TD_NEXT_PAGE,
	E_MENU_TD_PREV_PAGE,
	// Panel de visualizacion de modelo
	E_MENU_TD_MDL_CAPTION_BOX,
	E_MENU_TD_MDL_CAPTION_TITLE,
	E_MENU_TD_MDL_BG,
	E_MENU_TD_MDL_ITEM_NAME,
	E_MENU_TD_MDL_PREVIEW,
	E_MENU_TD_MDL_INFO_BOX,
	E_MENU_TD_MDL_INFO_ICON,
	E_MENU_TD_MDL_INFO_TEXT
};

static enum E_MENU_MODEL_DATA
{
	E_MENU_MODEL_ID,
	Float:E_MENU_MODEL_RX,
	Float:E_MENU_MODEL_RY,
	Float:E_MENU_MODEL_RZ,
	Float:E_MENU_MODEL_ZOOM,
	E_MENU_MODEL_COLOR1,
	E_MENU_MODEL_COLOR2,
	E_MENU_MODEL_INFO[MENU_MAX_LISTITEM_SIZE char]
};

// ----------------------------------------------------------------------------
// Variables estaticas
// ----------------------------------------------------------------------------

static g_MenuInfo[MAX_PLAYERS][E_PLAYER_MENU];

static PlayerText:g_MenuTD[MAX_PLAYERS][24 + MENU_MAX_LISTITEMS_PERPAGE];
static g_MenuTDCount[MAX_PLAYERS];
static g_MenuTDID[MAX_PLAYERS][E_MENU_TD];
static g_MenuTDDynamicStart[MAX_PLAYERS];

static g_MenuListitems[MAX_PLAYERS][MENU_MAX_LISTITEMS][MENU_MAX_LISTITEM_SIZE char];
static g_MenuListitemsInfo[MAX_PLAYERS][MENU_MAX_LISTITEMS][MENU_MAX_LISTITEM_SIZE char];
static g_MenuListitemsModel[MAX_PLAYERS][MENU_MAX_LISTITEMS][E_MENU_MODEL_DATA];


// ----------------------------------------------------------------------------
// Calculo de lineas de texto
// Portado de not_notify.inc - protegido contra doble definicion
// ----------------------------------------------------------------------------

// Ancho de la info box: texto de X=508 a X=616 (~108 unidades). Factor empirico: ~3.7
#if !defined MENU_INFO_MAX_WIDTH
	#define MENU_INFO_MAX_WIDTH (600)
#endif

#if !defined _NOT_TD_CHAR_WIDTH_DEFINED
#define _NOT_TD_CHAR_WIDTH_DEFINED
static const TDCharacterDefaultWidth[4] = {27, 20, 27, 20};
static const TDCharacterWidth[4][176] = {
	{
		 0,  12,  12,  12,  12,  12,  12,  12,
		12,  12,  12,  12,  12,  12,  12,  12,
		12,  12,  12,  12,  12,  12,  12,  12,
		12,  12,  12,  12,  12,  12,  12,  12,
		12,  13,  13,  28,  28,  28,  28,   8,
		17,  17,  30,  28,  28,  12,   9,  21,
		28,  14,  28,  28,  28,  28,  28,  28,
		28,  28,  13,  13,  30,  30,  30,  30,
		10,  25,  23,  21,  24,  22,  20,  24,
		24,  17,  20,  22,  20,  30,  27,  27,
		26,  26,  24,  23,  24,  31,  23,  31,
		24,  23,  21,  28,  33,  33,  14,  28,
		10,  11,  12,   9,  11,  10,  10,  12,
		12,   7,   7,  13,   5,  18,  12,  10,
		12,  11,  10,  12,   8,  13,  13,  18,
		17,  13,  12,  30,  30,  37,  35,  37,
		25,  25,  25,  25,  33,  21,  24,  24,
		24,  24,  17,  17,  17,  17,  27,  27,
		27,  27,  31,  31,  31,  31,  11,  11,
		11,  11,  11,  20,   9,  10,  10,  10,
		10,   7,   7,   7,   7,  10,  10,  10,
		10,  13,  13,  13,  13,  27,  12,  30
	}, {
		 0,  15,  15,  15,  15,  15,  15,  15,
		15,  15,  15,  15,  15,  15,  15,  15,
		15,  15,  15,  15,  15,  15,  15,  15,
		15,  15,  15,  15,  15,  15,  15,  15,
		15,   9,  17,  27,  20,  34,  23,  12,
		12,  12,  21,  20,  12,  14,  12,  15,
		23,  15,  21,  21,  21,  21,  21,  21,
		20,  21,  12,  12,  24,  24,  24,  19,
		10,  22,  19,  19,  22,  16,  19,  24,
		22,  11,  16,  21,  15,  28,  24,  27,
		20,  25,  19,  19,  18,  23,  23,  31,
		23,  19,  21,  21,  13,  35,  11,  21,
		10,  19,  20,  14,  20,  19,  13,  20,
		19,   9,   9,  19,   9,  29,  19,  21,
		19,  19,  15,  15,  14,  18,  19,  27,
		20,  20,  17,  21,  17,  20,  15,  15,
		22,  22,  22,  22,  29,  19,  16,  16,
		16,  16,  11,  11,  11,  11,  27,  27,
		27,  27,  23,  23,  23,  23,  20,  19,
		19,  19,  19,  30,  14,  19,  19,  19,
		19,   9,   9,   9,   9,  21,  21,  21,
		21,  18,  18,  18,  18,  24,  19,  19
	}, {
		 0,  15,  23,  15,  21,  21,  21,  21,
		21,  21,  20,  21,  12,  12,  24,  24,
		24,  19,  10,  22,  19,  19,  22,  16,
		19,  24,  22,  11,  16,  21,  15,  28,
		12,  13,  13,  28,  37,  28,  30,   8,
		17,  17,  30,  28,  28,  12,   9,  21,
		27,  16,  27,  27,  27,  27,  27,  27,
		27,  27,  18,  13,  30,  30,  30,  30,
		10,  29,  26,  25,  28,  26,  25,  27,
		28,  12,  24,  25,  24,  30,  27,  29,
		26,  26,  25,  26,  25,  26,  28,  32,
		27,  26,  26,  28,  33,  33,  10,  28,
		10,  29,  26,  25,  28,  26,  25,  27,
		28,  12,  24,  25,  24,  30,  27,  29,
		26,  26,  25,  26,  25,  26,  28,  32,
		27,  26,  26,  30,  30,  37,  35,  37,
		29,  29,  29,  29,  33,  25,  26,  26,
		26,  26,  14,  14,  14,  14,  29,  29,
		29,  29,  26,  26,  26,  26,  21,  29,
		29,  29,  29,  33,  25,  26,  26,  26,
		26,  14,  14,  14,  14,  29,  29,  29,
		29,  26,  26,  26,  26,  25,  25,  30
	}, {
		 0,   9,   9,  18,  18,  18,  18,  18,
		18,  18,  18,  19,  19,  19,   0,   9,
		 9,   9,   9,  18,  18,  18,  18,  18,
		18,  18,  18,  19,  19,  19,   0,   9,
		15,  10,  17,  27,  20,  34,  23,  10,
		15,  15,  21,  20,  12,  14,   9,  15,
		20,  18,  19,  19,  21,  19,  19,  19,
		19,  19,  16,  12,  24,  24,  24,  21,
		10,  19,  19,  19,  20,  19,  16,  19,
		19,   9,  19,  20,  14,  29,  19,  19,
		19,  19,  19,  19,  21,  19,  20,  32,
		21,  19,  19,  21,  13,  35,  10,  21,
		10,  19,  19,  19,  20,  19,  16,  19,
		19,   9,  19,  20,  14,  29,  19,  19,
		19,  19,  19,  19,  21,  19,  20,  32,
		21,  19,  19,  21,  17,  20,  15,  15,
		19,  19,  19,  19,  29,  19,  19,  19,
		19,  19,   9,   9,   9,   9,  19,  19,
		19,  19,  19,  19,  19,  19,  19,  19,
		19,  19,  19,  29,  19,  19,  19,  19,
		19,   9,   9,   9,   9,  19,  19,  19,
		19,  19,  19,  19,  19,  21,  21,  19
	}
};

stock GetTextDrawCharacterWidth(character, font, bool:proportional = true)
{
	if(!(0 <= font <= 3) || !(0 <= character < 176)) return 0;
	if(!proportional || character >= sizeof(TDCharacterWidth[])) return TDCharacterDefaultWidth[font];
	return TDCharacterWidth[font][character];
}
#endif // _NOT_TD_CHAR_WIDTH_DEFINED

static stock GetMenuInfoLineCount(const message[], maxwidth = MENU_INFO_MAX_WIDTH)
{
	new size = 0, lines = 1, i = 0, c;
	while((c = message[i]) != '\0')
	{
		if(c == '~' && message[i + 1] != '\0' && message[i + 2] == '~')
		{
			if(message[i + 1] == 'n') { lines++; size = 0; }
			i += 3;
			continue;
		}
		new charWidth = GetTextDrawCharacterWidth(c, 1, true);
		if(size + charWidth > maxwidth) { lines++; size = charWidth; }
		else size += charWidth;
		i++;
	}
	return lines;
}


/**
 *   Y88b        888    888                   888                         888                     888          d8b          888                                      888
 *    Y88b       888    888                   888                         888                     888          Y8P          888                                      888
 *     Y88b      888    888                   888                         888                     888                       888                                      888
 *      Y88b     8888888888  .d88b.   .d88b.  888  888 .d8888b        .d88888  8888b.  88888b.   .d88888      888 88888b.  888888 .d88b.  888d888 88888b.   8888b.  888
 *      d88P     888    888 d88""88b d88""88b 888 .88P 88K           d88" 888     "88b 888 "88b d88" 888      888 888 "88b 888   d8P  Y8b 888P"   888 "88b     "88b 888
 *     d88P      888    888 888  888 888  888 888888K  "Y8888b.      888  888 .d888888 888  888 888  888      888 888  888 888   88888888 888     888  888 .d888888 888
 *    d88P       888    888 Y88..88P Y88..88P 888 "88b      X88      Y88b 888 888  888 888  888 Y88b 888      888 888  888 Y88b. Y8b.     888     888  888 888  888 888
 *   d88P        888    888  "Y88P"   "Y88P"  888  888  88888P'       "Y88888 "Y888888 888  888  "Y88888      888 888  888  "Y888 "Y8888  888     888  888 "Y888888 888
 */

// Convierte caracteres especiales latinos al equivalente interno de TextDraw.
// Soporta tildes (mayusculas y minusculas), ene y puntuacion especial.
// Parametros:
//   text[] - Texto con caracteres latinos (cp1252)
// Retorna: string con codigos de GTA charset listos para PlayerTextDrawSetString
static stock TextDrawFixedLatinoText(const text[])
{
	new string[1024];
	new len = strlen(text);
	if(len >= sizeof(string)) len = sizeof(string) - 1;

	for(new i = 0; i < len; i++)
	{
		switch(text[i] & 0xFF)
		{
			case 0xE0: string[i] = 151; // a grave
			case 0xE1: string[i] = 152; // a acute
			case 0xE2: string[i] = 153; // a circumflex
			case 0xE4: string[i] = 154; // a umlaut
			case 0xC0: string[i] = 128; // A grave
			case 0xC1: string[i] = 129; // A acute
			case 0xC2: string[i] = 130; // A circumflex
			case 0xC4: string[i] = 131; // A umlaut
			case 0xE8: string[i] = 157; // e grave
			case 0xE9: string[i] = 158; // e acute
			case 0xEA: string[i] = 159; // e circumflex
			case 0xEB: string[i] = 160; // e umlaut
			case 0xC8: string[i] = 134; // E grave
			case 0xC9: string[i] = 135; // E acute
			case 0xCA: string[i] = 136; // E circumflex
			case 0xCB: string[i] = 137; // E umlaut
			case 0xEC: string[i] = 161; // i grave
			case 0xED: string[i] = 162; // i acute
			case 0xEE: string[i] = 163; // i circumflex
			case 0xEF: string[i] = 164; // i umlaut
			case 0xCC: string[i] = 138; // I grave
			case 0xCD: string[i] = 139; // I acute
			case 0xCE: string[i] = 140; // I circumflex
			case 0xCF: string[i] = 141; // I umlaut
			case 0xF2: string[i] = 165; // o grave
			case 0xF3: string[i] = 166; // o acute
			case 0xF4: string[i] = 167; // o circumflex
			case 0xF6: string[i] = 168; // o umlaut
			case 0xD2: string[i] = 142; // O grave
			case 0xD3: string[i] = 143; // O acute
			case 0xD4: string[i] = 144; // O circumflex
			case 0xD6: string[i] = 145; // O umlaut
			case 0xF9: string[i] = 169; // u grave
			case 0xFA: string[i] = 170; // u acute
			case 0xFB: string[i] = 171; // u circumflex
			case 0xFC: string[i] = 172; // u umlaut
			case 0xD9: string[i] = 146; // U grave
			case 0xDA: string[i] = 147; // U acute
			case 0xDB: string[i] = 148; // U circumflex
			case 0xDC: string[i] = 149; // U umlaut
			case 0xF1: string[i] = 174; // n tilde
			case 0xD1: string[i] = 173; // N tilde
			case 0xBF: string[i] = 175; // inverted ?
			case 0xA1: string[i] = 64;  // inverted !
			default: string[i] = text[i];
		}
	}
	string[len] = EOS;
	return string;
}

/**
 *   Y88b        888 88888888888                                                    888
 *    Y88b       888     888                                                        888
 *     Y88b      888     888                                                        888
 *      Y88b     888     888   .d88b.  888  888  888 888d888 8888b.  888  888  888 .d8888b
 *      d88P     888     888  d8P  Y8b 888  888  888 888P"      "88b 888  888  888 88K
 *     d88P      888     888  88888888 888  888  888 888    .d888888 888  888  888 "Y8888b.
 *    d88P       888     888  Y8b.     Y88b 888 d88P 888    888  888 Y88b 888 d88P      X88
 *   d88P        888     888   "Y8888   "Y8888888P"  888    "Y888888  "Y8888888P"   88888P'
 */

// Actualiza la info box y el panel de visualizacion del item seleccionado.
// Muestra/oculta la info box y escala la caja al numero real de lineas de texto.
// Muestra/oculta el panel de modelo 3D segun si el item tiene modelid != -1.
// Parametros:
//   playerid - ID del jugador
static stock Internal_UpdateListitemInfo(playerid)
{
	new listitemid = (MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] + (MenuInfo::E_PLAYER_MENU_PAGE[playerid] * MENU_MAX_LISTITEMS_PERPAGE));

	// --- Info box del item ---
	if(g_MenuListitemsInfo[playerid][listitemid]{0})
	{
		new string[MENU_MAX_LISTITEM_SIZE];
		strunpack(string, g_MenuListitemsInfo[playerid][listitemid]);

		new lines = GetMenuInfoLineCount(string);
		PlayerTextDrawLetterSize(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_BOX]], 0.0, float(lines));

		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_BOX]]);
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_ICON]]);
		PlayerTextDrawSetString(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_TEXT]], TextDrawFixedLatinoText(string));
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_TEXT]]);
	}
	else
	{
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_BOX]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_ICON]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_INFO_TEXT]]);
	}

	// --- Panel de visualizacion de modelo ---
	if(g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_ID] != -1)
	{
		// Nombre del item en el panel
		new itemName[MENU_MAX_LISTITEM_SIZE];
		strunpack(itemName, g_MenuListitems[playerid][listitemid]);
		PlayerTextDrawSetString(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_ITEM_NAME]], TextDrawFixedLatinoText(itemName));

		// Preview del modelo
		PlayerTextDrawSetPreviewModel(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW]],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_ID]);
		PlayerTextDrawSetPreviewRot(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW]],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_RX],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_RY],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_RZ],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_ZOOM]);
		PlayerTextDrawSetPreviewVehCol(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW]],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_COLOR1],
			g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_COLOR2]);

		// Mostrar panel
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_BOX]]);
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_TITLE]]);
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_BG]]);
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_ITEM_NAME]]);
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW]]);

		// Descripcion del objeto (opcional)
		if(g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_INFO][0])
		{
			new modelInfo[MENU_MAX_LISTITEM_SIZE];
			strunpack(modelInfo, g_MenuListitemsModel[playerid][listitemid][E_MENU_MODEL_INFO]);

			// Escalar la caja segun las lineas reales del texto (igual que la info box del item)
			new mdlLines = GetMenuInfoLineCount(modelInfo);
			PlayerTextDrawLetterSize(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_BOX]], 0.0, float(mdlLines));

			PlayerTextDrawSetString(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_TEXT]], TextDrawFixedLatinoText(modelInfo));
			PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_BOX]]);
			PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_ICON]]);
			PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_TEXT]]);
		}
		else
		{
			PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_BOX]]);
			PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_ICON]]);
			PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_TEXT]]);
		}
	}
	else
	{
		// Sin modelo: ocultar todo el panel
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_BOX]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_TITLE]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_BG]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_ITEM_NAME]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_BOX]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_ICON]]);
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_TEXT]]);
	}
}

// Destruye los TDs dinamicos y los recrea calculando posiciones verticales
// segun el numero real de items en la pagina. Gestiona fondo, slots, info box y botones.
// Parametros:
//   playerid   - ID del jugador
//   forceItems - Si > 0, usa ese valor como itemsInPage (creacion inicial sin parpadeo)
static stock Internal_ResizeDynamicTDs(playerid, forceItems = 0)
{
	// Cuantos items hay en la pagina actual
	new
		page        = MenuInfo::E_PLAYER_MENU_PAGE[playerid],
		totalItems  = MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid],
		pageStart   = (page * MENU_MAX_LISTITEMS_PERPAGE),
		itemsInPage = (forceItems > 0) ? forceItems : (totalItems - pageStart);

	if(itemsInPage > MENU_MAX_LISTITEMS_PERPAGE) itemsInPage = MENU_MAX_LISTITEMS_PERPAGE;
	if(itemsInPage < 1) itemsInPage = 1;

	// Destruir todos los TDs dinamicos anteriores
	for(new i = g_MenuTDDynamicStart[playerid]; i < g_MenuTDCount[playerid]; i++)
		PlayerTextDrawDestroy(playerid, g_MenuTD[playerid][i]);
	g_MenuTDCount[playerid] = g_MenuTDDynamicStart[playerid];

	// Posiciones clave calculadas dinamicamente
	new
		Float:listY       = 150.0,
		// LetterSize Y correcto: n*4/3 (sin el -1/3 de la formula anterior)
		// navBarY usa n*MENU_ITEM_PADDING que es exactamente consistente con el factor 9.0
		// Float:letterSizeY = (float(itemsInPage) * 4.0) / 3.0,
		Float:navBarY     = listY + (float(itemsInPage) * MENU_ITEM_PADDING),
		Float:infoY       = navBarY + 14.0;

	if(totalItems <= MENU_MAX_LISTITEMS_PERPAGE) infoY -= 12.0;

	// [5] Fondo de la lista de items - altura dinamica
	// Formula calibrada: (n * 4.0 - 1.0) / 3.0  ?  n=1:1.0, n=3:3.67, n=10:13.0
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, listY, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, (float(itemsInPage) * 4.0 - 1.0) / 3.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]++]);

	// [6..6+N] Slots de items - solo los que caben en la pagina
	for(new i; i < MENU_MAX_LISTITEMS_PERPAGE; i++)
	{
		g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, (listY + (MENU_ITEM_PADDING * float(i))), "_");
		PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
		PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
		PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.2, 1.0);
		PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
		PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
		PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
		PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
		PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
		PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0x00000000);
		PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 10.0);
		PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
		g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i] = g_MenuTDCount[playerid]++;
	}

	// [N+1] Caja de informacion - Y dinamica
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, infoY, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_INFO_BOX] = g_MenuTDCount[playerid]++;

	// [N+2] Icono info - Y dinamica
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, infoY, "LD_CHAT:badchat");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 4);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.159, 0.899);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 10.0, 10.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_INFO_ICON] = g_MenuTDCount[playerid]++;

	// [N+3] Texto info - Y dinamica
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 508.0, infoY, "...");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.18, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 616.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_INFO_TEXT] = g_MenuTDCount[playerid]++;

	// [N+4] Barra de fondo de navegacion - mismo color que captionBoxColor
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, navBarY, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], MenuInfo::E_PLAYER_MENU_CAPTION_BOX_COLOR[playerid]);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	// Oculto por defecto, Internal_UpdateListitems lo muestra si hay mas de una pagina
	g_MenuTDID[playerid][E_MENU_TD_NAV_BAR] = g_MenuTDCount[playerid]++;

	// [N+5] Boton "<< Anterior"
	new prevBtnText[MENU_MAX_LISTITEM_SIZE];
	format(prevBtnText, MENU_MAX_LISTITEM_SIZE, "%s", MenuInfo::E_PLAYER_MENU_BACK_BTN[playerid]);
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 528.0, navBarY, TextDrawFixedLatinoText(prevBtnText));
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 2);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 200);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.18, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 10.0, 60.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	g_MenuTDID[playerid][E_MENU_TD_PREV_PAGE] = g_MenuTDCount[playerid]++;

	// [N+6] Boton "Siguiente >>"
	new nextBtnText[MENU_MAX_LISTITEM_SIZE];
	format(nextBtnText, MENU_MAX_LISTITEM_SIZE, "%s", MenuInfo::E_PLAYER_MENU_NEXT_BTN[playerid]);
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 585.0, navBarY, TextDrawFixedLatinoText(nextBtnText));
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 2);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 200);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.18, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 10.0, 60.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	g_MenuTDID[playerid][E_MENU_TD_NEXT_PAGE] = g_MenuTDCount[playerid]++;
}

// Redibuja los items de la pagina actual, actualiza el contador X/Total,
// aplica resaltado al seleccionado y gestiona la visibilidad de la barra de navegacion.
// Parametros:
//   playerid - ID del jugador
static stock Internal_UpdateListitems(playerid)
{
	new string[MENU_MAX_LISTITEM_SIZE];

	// Actualizar contador "X/Total"
	new listitem = ((MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] + (MenuInfo::E_PLAYER_MENU_PAGE[playerid] * MENU_MAX_LISTITEMS_PERPAGE)) + 1);
	format(string, sizeof string, "%i/%i", listitem, MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]);
	PlayerTextDrawSetString(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEM_COUNT]], string);

	for(new i; i < MENU_MAX_LISTITEMS_PERPAGE; i++)
	{
		listitem = (i + (MenuInfo::E_PLAYER_MENU_PAGE[playerid] * MENU_MAX_LISTITEMS_PERPAGE));

		// Ocultar slots que superan el total de items
		if(listitem >= MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid])
		{
			PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]]);
			continue;
		}

		// Resaltar item seleccionado (blanco sobre fondo oscuro) vs normal (gris)
		if(i == MenuInfo::E_PLAYER_MENU_LISTITEM[playerid])
		{
			PlayerTextDrawColor(playerid,    g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]], 200);
			PlayerTextDrawBoxColor(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]], 0xFFFFFF80);
		}
		else
		{
			PlayerTextDrawColor(playerid,    g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]], -1);
			PlayerTextDrawBoxColor(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]], 0x00000000);
		}

		strunpack(string, g_MenuListitems[playerid][listitem]);
		PlayerTextDrawSetString(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]], TextDrawFixedLatinoText(string));
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]]);
	}

	Internal_UpdateListitemInfo(playerid);

	// Mostrar/ocultar barra de navegacion y botones segun paginas disponibles
	new totalPages  = MENU_COUNT_PAGES(MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid], MENU_MAX_LISTITEMS_PERPAGE);
	new bool:hasPrev = (MenuInfo::E_PLAYER_MENU_PAGE[playerid] > 0);
	new bool:hasNext = ((MenuInfo::E_PLAYER_MENU_PAGE[playerid] + 1) < totalPages);

	// La barra de fondo solo se muestra si hay al menos un boton visible
	if(hasPrev || hasNext)
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_NAV_BAR]]);
	else
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_NAV_BAR]]);

	if(hasNext)
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_NEXT_PAGE]]);
	else
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_NEXT_PAGE]]);

	if(hasPrev)
		PlayerTextDrawShow(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_PREV_PAGE]]);
	else
		PlayerTextDrawHide(playerid, g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_PREV_PAGE]]);
}

// Destruye todos los textdraws del menu y limpia el estado del jugador.
// Restaura movimiento, cancela cursor y limpia arrays de items para evitar datos viejos.
// Parametros:
//   playerid - ID del jugador
static stock Internal_DestroyMenu(playerid)
{
	for(new i; i < g_MenuTDCount[playerid]; i++)
		PlayerTextDrawDestroy(playerid, g_MenuTD[playerid][i]);

	g_MenuTDCount[playerid]                = 0;
	MenuInfo::E_PLAYER_MENU_ID[playerid][0] = EOS;

	// Limpiar arrays de items para que no queden datos viejos al abrir el siguiente menu
	for(new i; i < MENU_MAX_LISTITEMS; i++)
	{
		g_MenuListitems[playerid][i]{0}     = EOS;
		g_MenuListitemsInfo[playerid][i]{0} = EOS;
		g_MenuListitemsModel[playerid][i][E_MENU_MODEL_ID]     = -1;
		g_MenuListitemsModel[playerid][i][E_MENU_MODEL_INFO][0] = EOS;
	}

	TogglePlayerControllable(playerid, 1);
	CancelSelectTextDraw(playerid);
}

/**
 *   Y88b        8888888888                              8888888          888                                      888
 *    Y88b       888                                       888            888                                      888
 *     Y88b      888                                       888            888                                      888
 *      Y88b     8888888 888  888 88888b.   .d8888b        888   88888b.  888888 .d88b.  888d888 88888b.   8888b.  888
 *      d88P     888     888  888 888 "88b d88P"           888   888 "88b 888   d8P  Y8b 888P"   888 "88b     "88b 888
 *     d88P      888     888  888 888  888 888             888   888  888 888   88888888 888     888  888 .d888888 888
 *    d88P       888     Y88b 888 888  888 Y88b.           888   888  888 Y88b. Y8b.     888     888  888 888  888 888
 *   d88P        888      "Y88888 888  888  "Y8888P      8888888 888  888  "Y888 "Y8888  888     888  888 "Y888888 888
 *
 *       .d8888b.  888                         88888888888                888      888888b.
 *      d88P  Y88b 888                             888                    888      888  "88b
 *      Y88b.      888                             888                    888      888  .88P
 *       "Y888b.   88888b.   .d88b.  888  888  888 888   8888b.  .d8888b  888  888 8888888K.   8888b.  888d888
 *          "Y88b. 888 "88b d88""88b 888  888  888 888      "88b 88K      888 .88P 888  "Y88b     "88b 888P"
 *            "888 888  888 888  888 888  888  888 888  .d888888 "Y8888b. 888888K  888    888 .d888888 888
 *      Y88b  d88P 888  888 Y88..88P Y88b 888 d88P 888  888  888      X88 888 "88b 888   d88P 888  888 888
 *       "Y8888P"  888  888  "Y88P"   "Y8888888P"  888  "Y888888  88888P' 888  888 8888888P"  "Y888888 888
 */

/*
	ShowPlayerMenu
	Abre un menu de TextDraws navegable. Bloquea el movimiento del jugador.
	Crea los TDs con tamano maximo desde el inicio para evitar parpadeo.

	Parametros:
		playerid         - ID del jugador
		menuid[]         - Nombre del callback sin el prefijo "menu_"
		caption[]        - Titulo del menu
		type[]           - Subtitulo/tipo (default "Opciones")
		captionTextColor - Color del texto del titulo (default 0xB1B1B1FF)
		captionBoxColor  - Color del fondo del caption (default 0x1B1B1BFF)
		nextbutton[]     - Texto boton siguiente pagina (default "Siguiente >>")
		backbutton[]     - Texto boton pagina anterior (default "<< Anterior")

	Retorna:
		1 si se creo correctamente
		0 si playerid es invalido

	Nota:
		Despues de ShowPlayerMenu, agregar items con AddPlayerMenuItemModel y
		llamar FinalizePlayerMenu al final. El callback se declara con la macro
		MENU: y recibe (playerid, response, listitem).

	Ejemplo:
		ShowPlayerMenu(playerid, miMenu, "Tienda", .type = "COMPRAR", .captionBoxColor = 0xAC3E36BB);
		AddPlayerMenuItemModel(playerid, "Hamburguesa", "Restaura 50HP");
		AddPlayerMenuItemModel(playerid, "AK-47", "", 355, .rz = -45.0);
		FinalizePlayerMenu(playerid);
*/

/*
stock ShowPlayerMenu(playerid, const menuid[], const caption[], const type[] = "Opciones", captionTextColor = 0x1B1B1BFF, captionBoxColor = 0xAC3E36BB, const nextbutton[] = "Siguiente >>", const backbutton[] = "<< Anterior")
*/

#define ShowPlayerMenu(%0,%1, \
	Internal_ShowPlayerMenu(%0, #%1,

stock Internal_ShowPlayerMenu(playerid, const menuid[], const caption[], const type[] = "Options", captionTextColor = 0xB1B1B1FF, captionBoxColor = 0x1B1B1BFF, const nextbutton[] = "Next >>", const backbutton[] = "<< Back")
{
	if(playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	// Destroy any previously active menu and clear item buffers
	for(new i; i < g_MenuTDCount[playerid]; i++)
	{
		PlayerTextDrawDestroy(playerid, g_MenuTD[playerid][i]);
	}
	g_MenuTDCount[playerid] = 0;
	for(new i; i < MENU_MAX_LISTITEMS; i++)
	{
		g_MenuListitems[playerid][i]{0} = EOS;
		g_MenuListitemsInfo[playerid][i]{0} = EOS;
		g_MenuListitemsModel[playerid][i][E_MENU_MODEL_ID] = -1;
		g_MenuListitemsModel[playerid][i][E_MENU_MODEL_INFO][0] = EOS;
	}

	// [0] Fondo del caption
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, 106.0, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 3.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], captionBoxColor);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]++]);

	// [1] Titulo del menu
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 557.0, 113.0, TextDrawFixedLatinoText(caption));
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 2);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.3, 1.5);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], captionTextColor);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 122.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]++]);

	// [2] Fondo de la barra de subtitulo
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 496.0, 136.0, "_box");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 421075300);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 618.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]++]);

	// Convertir tipo a mayusculas

	// [3] Subtitulo (tipo de menu)
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 498.0, 136.0, TextDrawFixedLatinoText(type));
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.2, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 130.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]++]);

	// [4] Contador de item (X/Total), alineado a la derecha
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 616.0, 136.0, "-/-");
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 3);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.2, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawShow(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]]);
	g_MenuTDID[playerid][E_MENU_TD_LISTITEM_COUNT] = g_MenuTDCount[playerid]++;

	// [5..12] Panel de visualizacion de modelo - estatico, oculto por defecto
	// [5] Fondo del caption del panel
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 400.0, 106.0, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 3.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], captionBoxColor);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 480.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_BOX] = g_MenuTDCount[playerid]++;

	// [6] Titulo MENU_PREVIEW_TITLE
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 440.0, 113.0, MENU_PREVIEW_TITLE);
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 2);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.3, 1.5);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], captionTextColor);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 80.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_CAPTION_TITLE] = g_MenuTDCount[playerid]++;

	// [7] Fondo del area del modelo
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 400.0, 136.0, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 8.2);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 480.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_BG] = g_MenuTDCount[playerid]++;

	// [8] Nombre del item seleccionado
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 440.0, 138.0, "_");
	PlayerTextDrawAlignment(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 2);
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.18, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 80.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_ITEM_NAME] = g_MenuTDCount[playerid]++;

	// [9] Preview Font 5 del modelo
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 410.0, 148.0, "_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 5);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 0.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 60.0, 60.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetPreviewModel(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetPreviewRot(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 0.0, 0.0, 1.0);
	PlayerTextDrawSetPreviewVehCol(playerid,g_MenuTD[playerid][g_MenuTDCount[playerid]], 0, 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_PREVIEW] = g_MenuTDCount[playerid]++;

	// [10] Fondo de la descripcion del modelo
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 400.0, 216.0, "_box_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.0, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 128);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 480.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_BOX] = g_MenuTDCount[playerid]++;

	// [11] Icono LD_CHAT:badchat
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 400.0, 216.0, "LD_CHAT:badchat");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 4);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.158, 0.898);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawUseBox(playerid,          g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawBoxColor(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 10.0, 10.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_ICON] = g_MenuTDCount[playerid]++;

	// [12] Texto de descripcion del modelo
	g_MenuTD[playerid][g_MenuTDCount[playerid]] = CreatePlayerTextDraw(playerid, 412.0, 216.0, "_");
	PlayerTextDrawBackgroundColor(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 150);
	PlayerTextDrawFont(playerid,            g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawLetterSize(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0.18, 1.0);
	PlayerTextDrawColor(playerid,           g_MenuTD[playerid][g_MenuTDCount[playerid]], -1);
	PlayerTextDrawSetOutline(playerid,      g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawSetProportional(playerid, g_MenuTD[playerid][g_MenuTDCount[playerid]], 1);
	PlayerTextDrawSetShadow(playerid,       g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	PlayerTextDrawTextSize(playerid,        g_MenuTD[playerid][g_MenuTDCount[playerid]], 478.0, 0.0);
	PlayerTextDrawSetSelectable(playerid,   g_MenuTD[playerid][g_MenuTDCount[playerid]], 0);
	g_MenuTDID[playerid][E_MENU_TD_MDL_INFO_TEXT] = g_MenuTDCount[playerid]++;

	// Inicializar estado del menu ANTES de crear los TDs dinamicos,
	// para que Internal_ResizeDynamicTDs use los valores correctos (captionBoxColor, etc.)
	format(MenuInfo::E_PLAYER_MENU_ID[playerid], 32, "%s", menuid);
	MenuInfo::E_PLAYER_MENU_PAGE[playerid]              = 0;
	MenuInfo::E_PLAYER_MENU_LISTITEM[playerid]          = 0;
	MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]   = 0;
	MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid]         = 0;
	MenuInfo::E_PLAYER_MENU_CAPTION_BOX_COLOR[playerid] = captionBoxColor;
	format(MenuInfo::E_PLAYER_MENU_NEXT_BTN[playerid], MENU_MAX_LISTITEM_SIZE, "%s", nextbutton);
	format(MenuInfo::E_PLAYER_MENU_BACK_BTN[playerid], MENU_MAX_LISTITEM_SIZE, "%s", backbutton);

	// A partir de aqui los TDs son dinamicos.
	// Los creamos ahora con size maximo (MENU_MAX_LISTITEMS_PERPAGE) para que el
	// jugador vea el menu completo desde el primer frame sin parpadeo.
	// AddPlayerMenuItemModel solo actualiza contenido, no recrea TDs.
	g_MenuTDDynamicStart[playerid] = g_MenuTDCount[playerid];
	Internal_ResizeDynamicTDs(playerid, MENU_MAX_LISTITEMS_PERPAGE);

	TogglePlayerControllable(playerid, 0);
	return 1;
}

/*
	AddPlayerMenuItemModel
	Agrega un item al menu. Si modelid != -1, activa el panel de visualizacion 3D.
	Reemplaza a AddPlayerMenuItem - ambas signaturas son validas.

	Parametros:
		playerid    - ID del jugador
		text[]      - Texto del item
		info[]      - Texto de la info box (opcional, "" para omitir)
		modelid     - ID del modelo 3D (-1 = sin modelo, por defecto)
		rx/ry/rz    - Rotacion del modelo (rz por defecto -45.0)
		zoom        - Zoom del modelo (por defecto 1.0)
		color1/2    - Colores del vehiculo (-1 para no aplicar)
		modelinfo[] - Descripcion del objeto en el panel (opcional)

	Retorna:
		1 si se agrego correctamente
		0 si playerid invalido, sin menu activo o maximo alcanzado

	Ejemplos:
		AddPlayerMenuItemModel(playerid, "Hamburguesa", "Restaura 50HP");
		AddPlayerMenuItemModel(playerid, "AK-47", "Rifle de asalto", 355, .rz = -30.0, .modelinfo = "Dano: Alto");
		AddPlayerMenuItemModel(playerid, "Infernus", "", 411, .color1 = 3, .color2 = 3, .modelinfo = "240km/h");
*/
stock AddPlayerMenuItemModel(playerid, const text[], const info[] = "", modelid = -1, Float:rx = 0.0, Float:ry = 0.0, Float:rz = -45.0, Float:zoom = 1.0, color1 = -1, color2 = -1, const modelinfo[] = "")
{
	if(playerid < 0 || playerid >= MAX_PLAYERS)       return 0;
	if(!MenuInfo::E_PLAYER_MENU_ID[playerid][0])      return 0;
	if(MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid] == MENU_MAX_LISTITEMS) return 0;

	new idx = MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid];

	strpack(g_MenuListitems[playerid][idx],     text, MENU_MAX_LISTITEM_SIZE);
	strpack(g_MenuListitemsInfo[playerid][idx], info, MENU_MAX_LISTITEM_SIZE);

	// Guardar datos del modelo (o marcar como sin modelo)
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_ID]     = modelid;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_RX]     = rx;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_RY]     = ry;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_RZ]     = rz;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_ZOOM]   = zoom;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_COLOR1] = color1;
	g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_COLOR2] = color2;
	strpack(g_MenuListitemsModel[playerid][idx][E_MENU_MODEL_INFO], modelinfo, MENU_MAX_LISTITEM_SIZE);

	MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]++;

	new page      = MenuInfo::E_PLAYER_MENU_PAGE[playerid];
	new pageStart = page * MENU_MAX_LISTITEMS_PERPAGE;
	new pageEnd   = pageStart + MENU_MAX_LISTITEMS_PERPAGE;
	new total     = MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid];

	// Solo actualizar la vista cuando ya tenemos todos los items de la pagina visible
	// (el ultimo item de la pagina) o cuando cambia el numero de paginas.
	// Asi el jugador ve el menu completo de golpe, sin parpadeo ni contador subiendo.
	new bool:ultimoDePaginaVisible = (idx < pageEnd && total == pageEnd);
	new bool:ultimoItemTotal       = (total == MENU_MAX_LISTITEMS);
	new bool:primerDeNuevaPag      = (total > MENU_MAX_LISTITEMS_PERPAGE && (total % MENU_MAX_LISTITEMS_PERPAGE) == 1);

	if(ultimoDePaginaVisible || ultimoItemTotal || primerDeNuevaPag)
		Internal_UpdateListitems(playerid);

	return 1;
}

/*
	AddPlayerMenuItem
	Wrapper de AddPlayerMenuItemModel sin modelo, mantenido por compatibilidad.

	Ejemplo:
		AddPlayerMenuItem(playerid, "Hamburguesa", "Restaura 50HP");
*/
stock AddPlayerMenuItem(playerid, const text[], const info[] = "")
{
	return AddPlayerMenuItemModel(playerid, text, info);
}

/*
	FinalizePlayerMenu
	Obligatoria despues de agregar todos los items con AddPlayerMenuItemModel.
	Redimensiona el fondo al tamano real de items en la pagina y actualiza la vista.

	Ejemplo:
		ShowPlayerMenu(playerid, miMenu, "Tienda");
		for(new i; i < total; i++)
			AddPlayerMenuItemModel(playerid, nombre[i], "", modelo[i]);
		FinalizePlayerMenu(playerid);
*/
stock FinalizePlayerMenu(playerid)
{
	if(playerid < 0 || playerid >= MAX_PLAYERS)  return 0;
	if(!MenuInfo::E_PLAYER_MENU_ID[playerid][0]) return 0;

	Internal_ResizeDynamicTDs(playerid);
	Internal_UpdateListitems(playerid);

	SelectTextDraw(playerid, MENU_CURSOR_COLOR);
	return 1;
}

/*
	RemovePlayerMenuItemModel
	Elimina el modelo de un listitem en tiempo real.
	Si ese item estaba seleccionado, oculta el panel inmediatamente.

	Parametros:
		playerid - ID del jugador
		listitem - Indice del item

	Retorna:
		1 si se elimino correctamente
		0 si playerid invalido, sin menu activo o listitem fuera de rango
*/
stock RemovePlayerMenuItemModel(playerid, listitem)
{
	if(playerid < 0 || playerid >= MAX_PLAYERS)                                       return 0;
	if(!MenuInfo::E_PLAYER_MENU_ID[playerid][0])                                     return 0;
	if(listitem < 0 || listitem >= MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]) return 0;

	g_MenuListitemsModel[playerid][listitem][E_MENU_MODEL_ID]     = -1;
	g_MenuListitemsModel[playerid][listitem][E_MENU_MODEL_INFO][0] = EOS;

	if(listitem == GetPlayerMenuListitem(playerid))
		Internal_UpdateListitemInfo(playerid);

	return 1;
}

/*
	HidePlayerMenu
	Cierra y destruye el menu activo. Restaura el movimiento automaticamente.

	Parametros:
		playerid - ID del jugador

	Retorna:
		1 si se cerro correctamente
		0 si playerid es invalido

	Ejemplo:
		HidePlayerMenu(playerid);
*/
stock HidePlayerMenu(playerid)
{
	if(playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	Internal_DestroyMenu(playerid);
	return 1;
}

/*
	IsPlayerInMenu
	Verifica si el jugador tiene un menu activo.

	Parametros:
		playerid - ID del jugador

	Retorna:
		true si tiene menu abierto
		false si no hay menu o playerid es invalido

	Ejemplo:
		if(IsPlayerInMenu(playerid)) { ... }
*/
stock bool:IsPlayerInMenu(playerid)
{
	if(playerid < 0 || playerid >= MAX_PLAYERS) return false;
	return (MenuInfo::E_PLAYER_MENU_ID[playerid][0] != EOS);
}

/*
	GetPlayerMenuListitem
	Retorna el indice global del item actualmente seleccionado.

	Parametros:
		playerid - ID del jugador

	Retorna:
		Indice global del item seleccionado (0 a MENU_MAX_LISTITEMS-1)
		-1 si no hay menu activo

	Ejemplo:
		new item = GetPlayerMenuListitem(playerid);
*/
stock GetPlayerMenuListitem(playerid)
{
	if(!IsPlayerInMenu(playerid)) return -1;
	return (MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] + (MenuInfo::E_PLAYER_MENU_PAGE[playerid] * MENU_MAX_LISTITEMS_PERPAGE));
}

/*
	GetPlayerMenuPage
	Retorna la pagina actual del menu (0-indexed).

	Parametros:
		playerid - ID del jugador

	Retorna:
		Pagina actual (0-indexed), o -1 si no hay menu activo
*/
stock GetPlayerMenuPage(playerid)
{
	if(!IsPlayerInMenu(playerid)) return -1;
	return MenuInfo::E_PLAYER_MENU_PAGE[playerid];
}

/*
	GetPlayerMenuTotalItems
	Retorna cuantos items tiene el menu actual.

	Parametros:
		playerid - ID del jugador

	Retorna:
		Total de items del menu, o -1 si no hay menu activo
*/
stock GetPlayerMenuTotalItems(playerid)
{
	if(!IsPlayerInMenu(playerid)) return -1;
	return MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid];
}

/*
	GetPlayerMenuID
	Copia el nombre del menu activo a un buffer.

	Parametros:
		playerid - ID del jugador
		dest[]   - Buffer destino
		len      - Tamano del buffer

	Retorna:
		true si hay menu activo y se copio
		false si no hay menu activo
*/
stock bool:GetPlayerMenuID(playerid, dest[], len = sizeof dest)
{
	if(!IsPlayerInMenu(playerid)) return false;
	strmid(dest, MenuInfo::E_PLAYER_MENU_ID[playerid], 0, strlen(MenuInfo::E_PLAYER_MENU_ID[playerid]), len);
	return true;
}

/*
	SetPlayerMenuListitem
	Fuerza la seleccion a un item especifico.
	Util para abrir un menu con un item pre-seleccionado.

	Parametros:
		playerid - ID del jugador
		listitem - Indice global del item a seleccionar

	Retorna:
		true si se aplico correctamente
		false si listitem invalido o sin menu activo

	Ejemplo:
		SetPlayerMenuListitem(playerid, 2); // Seleccionar el 3er item
*/
stock bool:SetPlayerMenuListitem(playerid, listitem)
{
	if(!IsPlayerInMenu(playerid)) return false;
	if(listitem < 0 || listitem >= MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]) return false;

	MenuInfo::E_PLAYER_MENU_PAGE[playerid]     = (listitem / MENU_MAX_LISTITEMS_PERPAGE);
	MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = (listitem % MENU_MAX_LISTITEMS_PERPAGE);

	Internal_ResizeDynamicTDs(playerid);
	Internal_UpdateListitems(playerid);
	return true;
}

/**
 *   Y88b        8888888888                                          888
 *    Y88b       888                                                 888
 *     Y88b      888                                                 888
 *      Y88b     8888888  888  888  .d88b.  88888b.  888888 .d8888b  888
 *      d88P     888      888  888 d8P  Y8b 888 "88b 888    88K      888
 *     d88P      888      Y88  88P 88888888 888  888 888    "Y8888b. 888
 *    d88P        888      Y8bd8P  Y8b.     888  888 Y88b.       X88 888
 *   d88P         8888888   Y88P    "Y8888  888  888  "Y888  88888P' 888
 */

// Inicializa el contador de TDs y limpia el ID de menu al conectarse el jugador.
public OnPlayerConnect(playerid)
{
	g_MenuTDCount[playerid] = 0;
	MenuInfo::E_PLAYER_MENU_ID[playerid][0] = EOS;

	#if defined NMNU_OnPlayerConnect
		return NMNU_OnPlayerConnect(playerid);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerConnect
	#undef OnPlayerConnect
#else
	#define _ALS_OnPlayerConnect
#endif
#define OnPlayerConnect NMNU_OnPlayerConnect
#if defined NMNU_OnPlayerConnect
	forward NMNU_OnPlayerConnect(playerid);
#endif

public OnPlayerDisconnect(playerid, reason)
{
	if(MenuInfo::E_PLAYER_MENU_ID[playerid][0] != EOS)
	{
		Internal_DestroyMenu(playerid);
	}

	#if defined NMNU_OnPlayerDisconnect
		return NMNU_OnPlayerDisconnect(playerid, reason);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerDisconnect
	#undef OnPlayerDisconnect
#else
	#define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect NMNU_OnPlayerDisconnect
#if defined NMNU_OnPlayerDisconnect
	forward NMNU_OnPlayerDisconnect(playerid, reason);
#endif



// Captura ENTER (KEY_SECONDARY_ATTACK) para cerrar el menu
// y SPACE (KEY_SPRINT) para seleccionar el item actual.
// Aplica debounce de 200ms usando E_PLAYER_MENU_TICKCOUNT.
public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
	if(MenuInfo::E_PLAYER_MENU_ID[playerid][0] != EOS)
	{
		// ENTER -> Cerrar menu
		if((newkeys & KEY_SECONDARY_ATTACK) && !(oldkeys & KEY_SECONDARY_ATTACK) && (GetTickCount() - MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid]) >= 200)
		{
			MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid] = GetTickCount();
			PlayerPlaySound(playerid, MENU_SOUND_CLOSE, 0.0, 0.0, 0.0);

			new menuid[32] = "menu_";
			strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);
			new listitem = GetPlayerMenuListitem(playerid);

			Internal_DestroyMenu(playerid);
			CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_CLOSE, listitem);
		}
		// SPACE -> Seleccionar item
		else if((newkeys & KEY_SPRINT) && !(oldkeys & KEY_SPRINT) && (GetTickCount() - MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid]) >= 200)
		{
			MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid] = GetTickCount();
			PlayerPlaySound(playerid, MENU_SOUND_SELECT, 0.0, 0.0, 0.0);

			new menuid[32] = "menu_";
			strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);
			new listitem = GetPlayerMenuListitem(playerid);

			Internal_DestroyMenu(playerid);
			CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_SELECT, listitem);
		}
	}

	#if defined NMNU_OnPlayerKeyStateChange
		return NMNU_OnPlayerKeyStateChange(playerid, newkeys, oldkeys);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerKeyStateChange
	#undef OnPlayerKeyStateChange
#else
	#define _ALS_OnPlayerKeyStateChange
#endif
#define OnPlayerKeyStateChange NMNU_OnPlayerKeyStateChange
#if defined NMNU_OnPlayerKeyStateChange
	forward NMNU_OnPlayerKeyStateChange(playerid, newkeys, oldkeys);
#endif


// Captura navegacion con teclas arriba/abajo. Aplica debounce de 200ms.
// Gestiona ciclo al llegar al ultimo/primer item y cambio automatico de pagina.
public OnPlayerUpdate(playerid)
{
	if(MenuInfo::E_PLAYER_MENU_ID[playerid][0] != EOS)
	{
		new keys, updown, leftright;
		GetPlayerKeys(playerid, keys, updown, leftright);

		#pragma unused leftright
		#pragma unused keys

		if(updown == KEY_DOWN && (GetTickCount() - MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid]) >= 200)
		{
			new listitem = GetPlayerMenuListitem(playerid);

			MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid] = GetTickCount();
			PlayerPlaySound(playerid, MENU_SOUND_UPDOWN, 0.0, 0.0, 0.0);

			// Llego al final -> volver al principio
			if((listitem + 1) == MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid])
			{
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = 0;
				MenuInfo::E_PLAYER_MENU_PAGE[playerid]     = 0;
			}
			// Llego al final de la pagina -> pasar a siguiente pagina
			else if((MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] + 1) == MENU_MAX_LISTITEMS_PERPAGE)
			{
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = 0;
				MenuInfo::E_PLAYER_MENU_PAGE[playerid]++;
			}
			else
			{
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid]++;
			}

			Internal_ResizeDynamicTDs(playerid);
			Internal_UpdateListitems(playerid);

			new menuid[32] = "menu_";
			strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);
			CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_DOWN, GetPlayerMenuListitem(playerid));
		}
		else if(updown == KEY_UP && (GetTickCount() - MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid]) >= 200)
		{
			MenuInfo::E_PLAYER_MENU_TICKCOUNT[playerid] = GetTickCount();
			PlayerPlaySound(playerid, MENU_SOUND_UPDOWN, 0.0, 0.0, 0.0);

			// Esta al principio del slot en la pagina
			if((MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] - 1) == -1)
			{
				// Estaba en la primera pagina -> ir al ultimo item de la ultima pagina
				if(MenuInfo::E_PLAYER_MENU_PAGE[playerid] == 0)
				{
					MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = ((MENU_MAX_LISTITEMS_PERPAGE - ((MENU_COUNT_PAGES(MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid], MENU_MAX_LISTITEMS_PERPAGE) * MENU_MAX_LISTITEMS_PERPAGE) - MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid])) - 1);
					MenuInfo::E_PLAYER_MENU_PAGE[playerid]     = (MENU_COUNT_PAGES(MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid], MENU_MAX_LISTITEMS_PERPAGE) - 1);
				}
				else
				{
					MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = (MENU_MAX_LISTITEMS_PERPAGE - 1);
					MenuInfo::E_PLAYER_MENU_PAGE[playerid]--;
				}
			}
			else
			{
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid]--;
			}

			Internal_ResizeDynamicTDs(playerid);
			Internal_UpdateListitems(playerid);

			new menuid[32] = "menu_";
			strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);
			CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_UP, GetPlayerMenuListitem(playerid));
		}
	}

	#if defined NMNU_OnPlayerUpdate
		return NMNU_OnPlayerUpdate(playerid);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerUpdate
	#undef OnPlayerUpdate
#else
	#define _ALS_OnPlayerUpdate
#endif
#define OnPlayerUpdate NMNU_OnPlayerUpdate
#if defined NMNU_OnPlayerUpdate
	forward NMNU_OnPlayerUpdate(playerid);
#endif


// Captura ESC (INVALID_TEXT_DRAW) para cerrar el menu con MENU_RESPONSE_CLOSE.
// Si no hay menu activo, pasa el evento al siguiente hook.
public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    if(IsPlayerInMenu(playerid))
	{
		// if(playertextid == g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_CLOSE]] || playertextid == PlayerText:INVALID_TEXT_DRAW)
		if(clickedid == Text:INVALID_TEXT_DRAW)
		{
			PlayerPlaySound(playerid, MENU_SOUND_CLOSE, 0.0, 0.0, 0.0);

			new menuid[32] = "menu_";
			strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);
			new listitem = GetPlayerMenuListitem(playerid);

			Internal_DestroyMenu(playerid);
			CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_CLOSE, listitem);
			return 1;
		}
	}

    // Continue ALS chain if not handled
    #if defined NMNU_OnPlayerClickTextDraw
        return NMNU_OnPlayerClickTextDraw(playerid, clickedid);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerClickTextDraw
	#undef OnPlayerClickTextDraw
#else
	#define _ALS_OnPlayerClickTextDraw
#endif
#define OnPlayerClickTextDraw NMNU_OnPlayerClickTextDraw
#if defined NMNU_OnPlayerClickTextDraw
	forward NMNU_OnPlayerClickTextDraw(playerid, Text:clickedid);
#endif


// Captura clics en los TDs del menu: botones de pagina, seleccion por doble clic.
// 1er clic resalta el item; 2do clic sobre el mismo lo ejecuta (MENU_RESPONSE_SELECT).
// Si el TD no pertenece al menu, pasa el evento al siguiente hook.
public OnPlayerClickPlayerTextDraw(playerid, PlayerText:playertextid)
{
    if(IsPlayerInMenu(playerid))
    {
		// Clic en "Siguiente pagina"
		if(playertextid == g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_NEXT_PAGE]])
		{
			new totalPages = MENU_COUNT_PAGES(MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid], MENU_MAX_LISTITEMS_PERPAGE);
			if((MenuInfo::E_PLAYER_MENU_PAGE[playerid] + 1) < totalPages)
			{
				PlayerPlaySound(playerid, MENU_SOUND_UPDOWN, 0.0, 0.0, 0.0);
				MenuInfo::E_PLAYER_MENU_PAGE[playerid]++;
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = 0;
				Internal_ResizeDynamicTDs(playerid);
				Internal_UpdateListitems(playerid);
			}
			return 1;
		}

		// Clic en "Pagina anterior"
		if(playertextid == g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_PREV_PAGE]])
		{
			if(MenuInfo::E_PLAYER_MENU_PAGE[playerid] > 0)
			{
				PlayerPlaySound(playerid, MENU_SOUND_UPDOWN, 0.0, 0.0, 0.0);
				MenuInfo::E_PLAYER_MENU_PAGE[playerid]--;
				MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = 0;
				Internal_ResizeDynamicTDs(playerid);
				Internal_UpdateListitems(playerid);
			}
			return 1;
		}

        for(new i; i < MENU_MAX_LISTITEMS_PERPAGE; i++)
        {
            if(playertextid == g_MenuTD[playerid][g_MenuTDID[playerid][E_MENU_TD_LISTITEMS][i]])
            {
                new listitem = (i + (MenuInfo::E_PLAYER_MENU_PAGE[playerid] * MENU_MAX_LISTITEMS_PERPAGE));

                if(listitem >= MenuInfo::E_PLAYER_MENU_TOTAL_LISTITEMS[playerid]) return 1;

                // 1er clic: si el item clickeado NO es el actualmente seleccionado,
                // solo lo resaltamos (igual que navegar con teclas)
                if(i != MenuInfo::E_PLAYER_MENU_LISTITEM[playerid])
                {
                    PlayerPlaySound(playerid, MENU_SOUND_UPDOWN, 0.0, 0.0, 0.0);
                    MenuInfo::E_PLAYER_MENU_LISTITEM[playerid] = i;
                    Internal_UpdateListitems(playerid);
                    return 1;
                }

                // 2do clic: el item ya estaba seleccionado, lo ejecutamos
                PlayerPlaySound(playerid, MENU_SOUND_SELECT, 0.0, 0.0, 0.0);

                new menuid[32] = "menu_";
                strcat(menuid, MenuInfo::E_PLAYER_MENU_ID[playerid]);

                Internal_DestroyMenu(playerid);
                CallLocalFunction(menuid, "iii", playerid, MENU_RESPONSE_SELECT, listitem);
                return 1;
            }
        }
    }

	#if defined NMNU_OnPlayerClickPlayerTD
		return NMNU_OnPlayerClickPlayerTD(playerid, playertextid);
	#else
		return 0;
	#endif
}
#if defined _ALS_OnPlayerClickPlayerTD
	#undef OnPlayerClickPlayerTextDraw
#else
	#define _ALS_OnPlayerClickPlayerTD
#endif
#define OnPlayerClickPlayerTextDraw NMNU_OnPlayerClickPlayerTD
#if defined NMNU_OnPlayerClickPlayerTD
	forward NMNU_OnPlayerClickPlayerTD(playerid, PlayerText:playertextid);
#endif

// # END :D
