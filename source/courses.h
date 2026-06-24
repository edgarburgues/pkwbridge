/*
 * courses.h - Pokewalker course IDs and English names.
 *
 * The 32 official courses (16 default + 16 unlockable via Wi-Fi events).
 * The walk_data byte at 0x27 stores the courseId; the encounter tables
 * in the tail (0x8FBE..0xB7FF) are filled per-course by HGSS at send
 * time. The courseId byte can be changed from this app, but the tail
 * stays whatever the pweep template had.
 */
#ifndef COURSES_H
#define COURSES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t     id;
    const char *name;
    bool        wifi_only;   /* Mystery Gift / Wi-Fi event course (IDs 17..27) */
} course_t;

extern const course_t COURSES[];
extern const int      COURSES_COUNT;

const char *course_name(uint8_t id);
bool        course_is_wifi(uint8_t id);

#endif
