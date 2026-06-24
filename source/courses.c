#include "courses.h"

#include <stdio.h>

/* HGSS canonical course IDs are 1-based (1..27). These IDs are the SAME
 * in JP and Western HGSS — only the displayed name is localised; the
 * binary encounter tables and IDs are region-agnostic. The walk_data
 * byte at +0x27 stores this value directly and HGSS uses it to look up
 * its own localised course name in the msg archive. */
const course_t COURSES[] = {
    /* Default 16 (always available without Wi-Fi unlock) */
    {  1, "Refreshing Field",  false },
    {  2, "Noisy Forest",      false },
    {  3, "Rugged Road",       false },
    {  4, "Beautiful Beach",   false },
    {  5, "Suburban Area",     false },
    {  6, "Dim Cave",          false },
    {  7, "Blue Lake",         false },
    {  8, "Town Outskirts",    false },
    {  9, "Hoenn Field",       false },
    { 10, "Warm Beach",        false },
    { 11, "Volcano Path",      false },
    { 12, "Treehouse",         false },
    { 13, "Scary Cave",        false },
    { 14, "Sinnoh Field",      false },
    { 15, "Icy Mountain Rd.",  false },
    { 16, "Big Forest",        false },
    /* Wi-Fi event courses (must unlock via Mystery Gift in HGSS) */
    { 17, "White Lake",        true  },
    { 18, "Stormy Beach",      true  },
    { 19, "Resort",            true  },
    { 20, "Quiet Cave",        true  },
    { 21, "Beyond the Sea",    true  },
    { 22, "Night Sky's Edge",  true  },
    { 23, "Yellow Forest",     true  },
    { 24, "Rally",             true  },
    { 25, "Sightseeing",       true  },
    { 26, "Winner's Path",     true  },
    { 27, "Amity Meadow",      true  },
};
const int COURSES_COUNT = sizeof(COURSES) / sizeof(COURSES[0]);

const char *course_name(uint8_t id) {
    for (int i = 0; i < COURSES_COUNT; i++)
        if (COURSES[i].id == id) return COURSES[i].name;
    static char buf[16];
    snprintf(buf, sizeof buf, "course 0x%02X", id);
    return buf;
}

bool course_is_wifi(uint8_t id) {
    for (int i = 0; i < COURSES_COUNT; i++)
        if (COURSES[i].id == id) return COURSES[i].wifi_only;
    return false;
}
