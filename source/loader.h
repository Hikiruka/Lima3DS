#pragma once

#include "common.h"
#include "draw.h"
#include "loader.h"
#include "fatfs/ff.h"

unsigned char loader_bin[] = {
  0x50, 0x10, 0x9f, 0xe5, 0x50, 0x20, 0x9f, 0xe5, 0x50, 0x30, 0x9f, 0xe5,
  0x04, 0x00, 0xb3, 0xe5, 0x04, 0x00, 0xa2, 0xe5, 0x01, 0x00, 0x53, 0xe1,
  0xfb, 0xff, 0xff, 0x1a, 0x00, 0x10, 0xa0, 0xe3, 0x00, 0x00, 0xa0, 0xe3,
  0x00, 0x20, 0x81, 0xe1, 0x5e, 0x2f, 0x07, 0xee, 0x20, 0x00, 0x80, 0xe2,
  0x01, 0x0b, 0x50, 0xe3, 0xfa, 0xff, 0xff, 0x1a, 0x01, 0x11, 0x81, 0xe2,
  0x00, 0x00, 0x51, 0xe3, 0xf6, 0xff, 0xff, 0x1a, 0x00, 0x00, 0xa0, 0xe3,
  0x15, 0x0f, 0x07, 0xee, 0x9a, 0x0f, 0x07, 0xee, 0x0c, 0x30, 0x9f, 0xe5,
  0x13, 0xff, 0x2f, 0xe1, 0xfc, 0xff, 0xf7, 0x24, 0xfc, 0xff, 0xef, 0x23,
  0xfc, 0xff, 0xef, 0x24, 0x00, 0x00, 0xf0, 0x23
};
unsigned int loader_bin_len = 104;

// TODO: Optimize memsearch, possibly search every 2/4 bytes instead of 1
u8 *memsearch(u8 *search_start, const u32 search_len, const u8 *search_pattern, const u32 pattern_len)
{
    if (!search_start || !search_len || !search_pattern || !pattern_len || (search_len < pattern_len))
        return NULL;

    for (u32 i = 0; i <= (search_len - pattern_len); i++)
        if (memcmp(search_start + i, search_pattern, pattern_len) == 0)
            return (search_start + i);

    return NULL;
}

s32 luma_reboot_patch(u32 *buf, u32 len)
{
    const u32 luma_signature = 0xEA000001; // b (pc + 0xC) + 2 placeholders
    const u8 sdmc_path_pat[]  = {0x73, 0x00, 0x64, 0x00, 0x6D, 0x00, 0x63, 0x00, 0x3A, 0x00}; // "sdmc:"

    if (!(buf[0] == luma_signature && (buf[1] | buf[2]) == 0))
    	return 0;

    u8 *sdmc_loc = memsearch((u8*)buf, len, sdmc_path_pat, sizeof(sdmc_path_pat));

    if (!sdmc_loc) // Reboot path not found
        return -1;

    sdmc_loc += sizeof(sdmc_path_pat); // Skip "sdmc:"

    const char payload_path[] = PAYLOAD_PATH;
    const u32 path_len = strlen(payload_path);

    // Zero out the original path
    memset(sdmc_loc, 0, 74);

    // Copy the current path to memory
    for (u32 i = 0; i < path_len; i++)
        sdmc_loc[i * 2] = payload_path[i];

    return 0;
}

void chainload()
{
    // loader itself gets loaded @ 0x25F00000, the payload gets (temporarily) loaded @ 0x24F00000
    u8 *loader_addr = (u8*)0x25F00000;
    u32 *payload_addr = (u32*)0x24F00000;
    memcpy(loader_addr, loader_bin, loader_bin_len);

    FIL payload;
    size_t payload_br;

    if (f_open(&payload, PAYLOAD_PATH, FA_READ) != FR_OK)
        Debug("Missing " PAYLOAD_PATH);

    // Loader copies 512KB of data from payload_addr to 0x23F00000 and executes is
    if (f_read(&payload, payload_addr, f_size(&payload), &payload_br))
        Debug("Couldn't read " PAYLOAD_PATH);

    f_close(&payload);

    // Patch payloads that use Luma-styled reboot patches (Luma itself, Mizuki, Salt, etc)
    // This is completely non-standard and I wish it could be avoided
    if (luma_reboot_patch(payload_addr, payload_br))
        Debug("Luma patching failed!");
        // This can only happen if the payload is detected as a Luma payload but the pattern could not be found

    f_mount(NULL, "0:", 0); // Unmount SD

    // Jump to the loader
    ((void(*)())(u32)loader_addr)();
}

int BootPayload()
{
    chainload();
    return 0;
}