/*
 * migrate.h - one-shot migration of legacy backups.
 *
 * Earlier builds wrote *.<timestamp>.rom and *.rom.bak next to the input
 * file (inside /3ds/pokeStride and /roms/nds/saves), cluttering those
 * directories. Current builds put them in /3ds/pkwbridge/backups/. This
 * function moves any legacy backups it finds to the new location at app
 * boot. Idempotent — running it again on an already-migrated layout is
 * a no-op.
 */
#ifndef MIGRATE_H
#define MIGRATE_H
void migrate_legacy_backups(void);
#endif
