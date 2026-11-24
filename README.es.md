# not_menu

> **Idioma:** [English](README.md) | Español

Librería interactiva de menús mediante **PlayerTextDraws** para **SA-MP** y **open.mp**, con navegación por teclado y ratón, paginación automática, caja de información dinámica y panel de vista previa 3D.

## Características

- **Navegación dual:** Arriba/Abajo/Espacio/Enter con teclado y soporte de ratón (1er clic resalta, 2do clic selecciona).
- **Paginación automática:** Botones clicables `Next >>` / `<< Back` y cambio de página automático con teclado.
- **Caja de información dinámica:** Ajusta su altura según el wrapping real usando la tabla de anchos de fuente de GTA:SA.
- **Panel de vista previa 3D:** Panel lateral opcional (`Font 5`) con rotación, zoom, colores de vehículo y descripción por ítem.
- **Sintaxis limpia:** Macro `MENU:miMenu(playerid, response, listitem)`.
- **Sin dependencias externas:** Solo requiere `<a_samp>` y usa hooks ALS estándar compatibles con YSI (`y_hooks`).

## Instalación

1. Copia `not_menu.inc` en tu carpeta `pawno/include/` (o `qawno/include/`).
2. Inclúyelo después de `<a_samp>`:

```pawn
#include <a_samp>

// Configuración opcional (definir antes del #include):
#define MENU_MAX_LISTITEMS          24
#define MENU_MAX_LISTITEM_SIZE      128
#define MENU_MAX_LISTITEMS_PERPAGE  10
#define MENU_CURSOR_COLOR           0x5A6FE7FF
#define MENU_PREVIEW_TITLE          "Vista Previa"

#include <not_menu>
```

## Uso rápido

```pawn
stock AbrirTienda(playerid)
{
    ShowPlayerMenu(playerid, menuTienda, "Tienda", .type = "COMPRAR", .captionBoxColor = 0xAC3E36BB);
    AddPlayerMenuItemModel(playerid, "Hamburguesa", "Restaura 50 HP");
    AddPlayerMenuItemModel(playerid, "AK-47", "Rifle de asalto", 355, .rz = -45.0, .modelinfo = "Dano: Alto");
    FinalizePlayerMenu(playerid);
}

MENU:menuTienda(playerid, response, listitem)
{
    if(response == MENU_RESPONSE_SELECT)
    {
        // listitem es el índice global (0-based)
    }
    else if(response == MENU_RESPONSE_CLOSE)
    {
        // El jugador cerró el menú
    }
    return 1;
}
```

## Resumen de API

- `ShowPlayerMenu(playerid, menuid, const caption[], ...)`
- `AddPlayerMenuItemModel(playerid, const text[], const info[] = "", modelid = -1, ...)`
- `AddPlayerMenuItem(playerid, const text[], const info[] = "")`
- `FinalizePlayerMenu(playerid)`
- `RemovePlayerMenuItemModel(playerid, listitem)`
- `HidePlayerMenu(playerid)`
- `IsPlayerInMenu(playerid)`
- `GetPlayerMenuListitem(playerid)`, `GetPlayerMenuPage(playerid)`, `GetPlayerMenuTotalItems(playerid)`, `GetPlayerMenuID(playerid)`, `SetPlayerMenuListitem(playerid, listitem)`

## Créditos

- Escrito y refactorizado por **not**.
- Tabla de anchos de caracteres por **Nero_3D** (`td-string-width`).
- Estilo y arquitectura de menús inspirada en **Gammix**.

## Licencia

Distribuido bajo la [Licencia MIT](LICENSE).
