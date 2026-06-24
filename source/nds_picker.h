/*
 * nds_picker.h — UI to locate a supported .nds dump anywhere on SD.
 *
 * Used by data_loader when the runtime .bin files are missing and there
 * is no NDS at the well-known sdmc:/3ds/pokeStride/data/.
 *
 * The scanner walks a handful of common SD locations recursively (with
 * a small depth limit so it doesn't spelunk the entire card) and lists
 * any .nds it finds whose 4-byte game code is recognised by any
 * registered game_profile_t. Unrecognised dumps are filtered out so
 * the user only sees viable choices.
 */
#ifndef NDS_PICKER_H
#define NDS_PICKER_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

#include "game_profile.h"

#define NDS_PICKER_MAX       32
#define NDS_PICKER_DIRLEN   192
#define NDS_PICKER_NAMELEN   96

typedef struct {
    char                    dir[NDS_PICKER_DIRLEN];
    char                    name[NDS_PICKER_NAMELEN];
    char                    gamecode[5];   /* 4 bytes + NUL */
    off_t                   size;
    const game_profile_t   *profile;       /* matched at scan time */
} nds_entry_t;

typedef struct {
    nds_entry_t entries[NDS_PICKER_MAX];
    int         count;
    int         cursor;
    int         scroll;
} nds_list_t;

/* Walk known SD locations and populate `l` with HGSS .nds candidates. */
void nds_picker_scan(nds_list_t *l);

/* Run the picker UI. On A: writes the selected full path to path_out
 * and returns true. On B / START: returns false. */
bool nds_picker_run(nds_list_t *l, char *path_out, size_t cap);

/* Convenience: draw a "scanning" splash, run the scan, then run the
 * picker UI. Returns true if the user selected a file. */
bool nds_picker_pick(char *path_out, size_t cap);

#endif
