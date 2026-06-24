/*
 * app_state.h - active pweep + sav for the whole app session.
 *
 * A single active pair is picked once at session start and used by every
 * flow, so a pweep and sav from different points in time can't be mixed
 * (which would desync the WalkerPair slot against pweep.has_pokemon).
 */
#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>

#include "hgss_sav.h"
#include "pweep.h"

bool        app_state_set_pweep(const char *path);
bool        app_state_set_sav(const char *path);

const char *app_state_pweep_path(void);
const char *app_state_sav_path(void);
const char *app_state_pweep_basename(void);   /* "pweep.rom" or "(none)" */
const char *app_state_sav_basename(void);

pweep_t    *app_state_pweep(void);
hgss_sav_t *app_state_sav(void);

bool app_state_pweep_loaded(void);
bool app_state_sav_loaded(void);
bool app_state_both_loaded(void);

/* Re-read the active files from disk (use after backup_ui restores). */
void app_state_reload(void);

/* Load the previous-session paths from the SD config file and try to
 * open them. If both load, the user can skip the pickers. */
bool app_state_load_session(void);

/* Persist current active paths to SD so next boot can auto-load. */
void app_state_save_session(void);

/* Show a picker; pre-position cursor on canonical name. Returns true if
 * the user picked AND the load succeeded. */
bool app_state_pick_pweep(void);
bool app_state_pick_sav(void);

/* "Change active files" main-menu flow: pweep picker then sav picker. */
void app_state_change_files_flow(void);

/* Convenience: prompt for files if either is missing. Returns true if
 * both are loaded after this call. */
bool app_state_ensure_loaded(void);

#endif
