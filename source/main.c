#include "loader.h"
#include "common.h"
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "menu.h"
#include "platform.h"
#include "i2c.h"
#include "decryptor/keys.h"
#include "decryptor/game.h"
#include "decryptor/nand.h"
#include "decryptor/nandfat.h"
#include "decryptor/titlekey.h"
#include "decryptor/selftest.h"
#include "decryptor/xorpad.h"
#include "decryptor/transfer.h"

#define SUBMENU_START 5

MenuInfo menu[] =
{
	{ 
		"Boot Firmware", 1,             
		{
			{ "Boot",                    	  &BootPayload,          0 }
		}
	},
    {
        "Gamecart Dumper Options", 3,
        {
            { "Dump Cart",                    &DumpGameCart,          0 },
            { "Dump Cart to CIA",             &DumpGameCart,          CD_DECRYPT | CD_MAKECIA },
            { "Dump Private Header",          &DumpPrivateHeader,     0 }
        }
    },
    {
        "Manage GBA VC Save", 2, 
        {
            { "GBA VC Save Dump",             &DumpGbaVcSave,         0 },
            { "GBA VC Save Inject",           &InjectGbaVcSave,       N_NANDWRITE }
        }
    },
    {
        "NAND Options", 4,
        {
            { "NAND Backup",                  &DumpNand,              0 },
            { "NAND Restore",                 &RestoreNand,           N_NANDWRITE | NR_KEEPA9LH },
            { "emuNAND Backup",               &DumpNand,              N_EMUNAND },
            { "emuNAND Restore",              &RestoreNand,           N_NANDWRITE | N_EMUNAND | N_FORCEEMU }
        }
    },
    {
        "FBI Installation", 2, 
        {
            { "Install on sysNAND",    NULL,                   SUBMENU_START +  0},
            { "Install on emuNAND",    NULL,                   SUBMENU_START +  1}		
        }
    },
    // everything below is not contained in the main menu
    {
        "Install FBI (sysNAND)", 2, // ID 0
        {            
            { "Health&Safety Dump",           &DumpHealthAndSafety,   N_EMUNAND },
            { "Install FBI",                  &InjectHealthAndSafety, N_NANDWRITE | N_EMUNAND }
        }
    },
    {
        "Install FBI (EmuNAND)", 2, // ID 1
        {            
            { "Health&Safety Dump",           &DumpHealthAndSafety,   0 },
            { "Install FBI",                  &InjectHealthAndSafety, N_NANDWRITE }
        }
    },
    {
        NULL, 0, { { 0 } } // empty menu to signal end
    }
};


void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


void PowerOff()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while (true);
}


int main()
{
    ClearScreenFull(true, true);
    InitFS();
    FileGetData("lima/botbkg.bin", BOT_SCREEN0, 320 * 240 * 3, 0);
    memcpy(BOT_SCREEN1, BOT_SCREEN0, 320 * 240 * 3);
    u32 menu_exit = ProcessMenu(menu, SUBMENU_START);
    
    DeinitFS();
    (menu_exit == MENU_EXIT_REBOOT) ? Reboot() : PowerOff();
    return 0;
}