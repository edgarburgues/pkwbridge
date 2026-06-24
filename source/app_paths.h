/*
 * app_paths.h - SD card paths the app reads/writes.
 *
 * The user keeps their pokeStride emulator at /3ds/pokeStride/ (which is
 * where the walker EEPROM dump lives) and their HGSS .sav next to the
 * NDS ROM under /roms/nds/saves/ (twilight-menu / nds-bootstrap layout).
 * Fallbacks are checked in order.
 */
#ifndef APP_PATHS_H
#define APP_PATHS_H

#define PWEEP_DIR_PRIMARY  "sdmc:/3ds/pokeStride"
#define SAV_DIR_PRIMARY    "sdmc:/roms/nds/saves"
#define APP_DIR            "sdmc:/3ds/pkwbridge"
#define SDMC_ROOT          "sdmc:"

#define LOG_PATH           APP_DIR "/pkwbridge.log"
#define SESSION_PATH       APP_DIR "/last_session.cfg"

/* Backups live under /3ds/pkwbridge so the user's pokeStride and
 * roms/nds/saves dirs only show a single canonical file each. */
#define BACKUP_DIR         APP_DIR "/backups"
#define BACKUP_PWEEP_DIR   BACKUP_DIR "/pweep"
#define BACKUP_SAV_DIR     BACKUP_DIR "/sav"

#endif
