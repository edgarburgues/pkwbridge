/*
 * names.c - species names (built-in) + item/move/met-location names
 * (loaded at boot from the user's HGSS .nds via the msg-NARC extractor
 * in nds_extract.c).
 */
#include "names.h"
#include "applog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_species[] = {
    "(none)",
    /*   1 */ "Bulbasaur","Ivysaur","Venusaur","Charmander","Charmeleon",
    /*   6 */ "Charizard","Squirtle","Wartortle","Blastoise","Caterpie",
    /*  11 */ "Metapod","Butterfree","Weedle","Kakuna","Beedrill",
    /*  16 */ "Pidgey","Pidgeotto","Pidgeot","Rattata","Raticate",
    /*  21 */ "Spearow","Fearow","Ekans","Arbok","Pikachu",
    /*  26 */ "Raichu","Sandshrew","Sandslash","Nidoran F","Nidorina",
    /*  31 */ "Nidoqueen","Nidoran M","Nidorino","Nidoking","Clefairy",
    /*  36 */ "Clefable","Vulpix","Ninetales","Jigglypuff","Wigglytuff",
    /*  41 */ "Zubat","Golbat","Oddish","Gloom","Vileplume",
    /*  46 */ "Paras","Parasect","Venonat","Venomoth","Diglett",
    /*  51 */ "Dugtrio","Meowth","Persian","Psyduck","Golduck",
    /*  56 */ "Mankey","Primeape","Growlithe","Arcanine","Poliwag",
    /*  61 */ "Poliwhirl","Poliwrath","Abra","Kadabra","Alakazam",
    /*  66 */ "Machop","Machoke","Machamp","Bellsprout","Weepinbell",
    /*  71 */ "Victreebel","Tentacool","Tentacruel","Geodude","Graveler",
    /*  76 */ "Golem","Ponyta","Rapidash","Slowpoke","Slowbro",
    /*  81 */ "Magnemite","Magneton","Farfetch'd","Doduo","Dodrio",
    /*  86 */ "Seel","Dewgong","Grimer","Muk","Shellder",
    /*  91 */ "Cloyster","Gastly","Haunter","Gengar","Onix",
    /*  96 */ "Drowzee","Hypno","Krabby","Kingler","Voltorb",
    /* 101 */ "Electrode","Exeggcute","Exeggutor","Cubone","Marowak",
    /* 106 */ "Hitmonlee","Hitmonchan","Lickitung","Koffing","Weezing",
    /* 111 */ "Rhyhorn","Rhydon","Chansey","Tangela","Kangaskhan",
    /* 116 */ "Horsea","Seadra","Goldeen","Seaking","Staryu",
    /* 121 */ "Starmie","Mr. Mime","Scyther","Jynx","Electabuzz",
    /* 126 */ "Magmar","Pinsir","Tauros","Magikarp","Gyarados",
    /* 131 */ "Lapras","Ditto","Eevee","Vaporeon","Jolteon",
    /* 136 */ "Flareon","Porygon","Omanyte","Omastar","Kabuto",
    /* 141 */ "Kabutops","Aerodactyl","Snorlax","Articuno","Zapdos",
    /* 146 */ "Moltres","Dratini","Dragonair","Dragonite","Mewtwo",
    /* 151 */ "Mew",
    /* 152 */ "Chikorita","Bayleef","Meganium","Cyndaquil","Quilava",
    /* 157 */ "Typhlosion","Totodile","Croconaw","Feraligatr","Sentret",
    /* 162 */ "Furret","Hoothoot","Noctowl","Ledyba","Ledian",
    /* 167 */ "Spinarak","Ariados","Crobat","Chinchou","Lanturn",
    /* 172 */ "Pichu","Cleffa","Igglybuff","Togepi","Togetic",
    /* 177 */ "Natu","Xatu","Mareep","Flaaffy","Ampharos",
    /* 182 */ "Bellossom","Marill","Azumarill","Sudowoodo","Politoed",
    /* 187 */ "Hoppip","Skiploom","Jumpluff","Aipom","Sunkern",
    /* 192 */ "Sunflora","Yanma","Wooper","Quagsire","Espeon",
    /* 197 */ "Umbreon","Murkrow","Slowking","Misdreavus","Unown",
    /* 202 */ "Wobbuffet","Girafarig","Pineco","Forretress","Dunsparce",
    /* 207 */ "Gligar","Steelix","Snubbull","Granbull","Qwilfish",
    /* 212 */ "Scizor","Shuckle","Heracross","Sneasel","Teddiursa",
    /* 217 */ "Ursaring","Slugma","Magcargo","Swinub","Piloswine",
    /* 222 */ "Corsola","Remoraid","Octillery","Delibird","Mantine",
    /* 227 */ "Skarmory","Houndour","Houndoom","Kingdra","Phanpy",
    /* 232 */ "Donphan","Porygon2","Stantler","Smeargle","Tyrogue",
    /* 237 */ "Hitmontop","Smoochum","Elekid","Magby","Miltank",
    /* 242 */ "Blissey","Raikou","Entei","Suicune","Larvitar",
    /* 247 */ "Pupitar","Tyranitar","Lugia","Ho-Oh","Celebi",
    /* 252 */ "Treecko","Grovyle","Sceptile","Torchic","Combusken",
    /* 257 */ "Blaziken","Mudkip","Marshtomp","Swampert","Poochyena",
    /* 262 */ "Mightyena","Zigzagoon","Linoone","Wurmple","Silcoon",
    /* 267 */ "Beautifly","Cascoon","Dustox","Lotad","Lombre",
    /* 272 */ "Ludicolo","Seedot","Nuzleaf","Shiftry","Taillow",
    /* 277 */ "Swellow","Wingull","Pelipper","Ralts","Kirlia",
    /* 282 */ "Gardevoir","Surskit","Masquerain","Shroomish","Breloom",
    /* 287 */ "Slakoth","Vigoroth","Slaking","Nincada","Ninjask",
    /* 292 */ "Shedinja","Whismur","Loudred","Exploud","Makuhita",
    /* 297 */ "Hariyama","Azurill","Nosepass","Skitty","Delcatty",
    /* 302 */ "Sableye","Mawile","Aron","Lairon","Aggron",
    /* 307 */ "Meditite","Medicham","Electrike","Manectric","Plusle",
    /* 312 */ "Minun","Volbeat","Illumise","Roselia","Gulpin",
    /* 317 */ "Swalot","Carvanha","Sharpedo","Wailmer","Wailord",
    /* 322 */ "Numel","Camerupt","Torkoal","Spoink","Grumpig",
    /* 327 */ "Spinda","Trapinch","Vibrava","Flygon","Cacnea",
    /* 332 */ "Cacturne","Swablu","Altaria","Zangoose","Seviper",
    /* 337 */ "Lunatone","Solrock","Barboach","Whiscash","Corphish",
    /* 342 */ "Crawdaunt","Baltoy","Claydol","Lileep","Cradily",
    /* 347 */ "Anorith","Armaldo","Feebas","Milotic","Castform",
    /* 352 */ "Kecleon","Shuppet","Banette","Duskull","Dusclops",
    /* 357 */ "Tropius","Chimecho","Absol","Wynaut","Snorunt",
    /* 362 */ "Glalie","Spheal","Sealeo","Walrein","Clamperl",
    /* 367 */ "Huntail","Gorebyss","Relicanth","Luvdisc","Bagon",
    /* 372 */ "Shelgon","Salamence","Beldum","Metang","Metagross",
    /* 377 */ "Regirock","Regice","Registeel","Latias","Latios",
    /* 382 */ "Kyogre","Groudon","Rayquaza","Jirachi","Deoxys",
    /* 387 */ "Turtwig","Grotle","Torterra","Chimchar","Monferno",
    /* 392 */ "Infernape","Piplup","Prinplup","Empoleon","Starly",
    /* 397 */ "Staravia","Staraptor","Bidoof","Bibarel","Kricketot",
    /* 402 */ "Kricketune","Shinx","Luxio","Luxray","Budew",
    /* 407 */ "Roserade","Cranidos","Rampardos","Shieldon","Bastiodon",
    /* 412 */ "Burmy","Wormadam","Mothim","Combee","Vespiquen",
    /* 417 */ "Pachirisu","Buizel","Floatzel","Cherubi","Cherrim",
    /* 422 */ "Shellos","Gastrodon","Ambipom","Drifloon","Drifblim",
    /* 427 */ "Buneary","Lopunny","Mismagius","Honchkrow","Glameow",
    /* 432 */ "Purugly","Chingling","Stunky","Skuntank","Bronzor",
    /* 437 */ "Bronzong","Bonsly","Mime Jr.","Happiny","Chatot",
    /* 442 */ "Spiritomb","Gible","Gabite","Garchomp","Munchlax",
    /* 447 */ "Riolu","Lucario","Hippopotas","Hippowdon","Skorupi",
    /* 452 */ "Drapion","Croagunk","Toxicroak","Carnivine","Finneon",
    /* 457 */ "Lumineon","Mantyke","Snover","Abomasnow","Weavile",
    /* 462 */ "Magnezone","Lickilicky","Rhyperior","Tangrowth","Electivire",
    /* 467 */ "Magmortar","Togekiss","Yanmega","Leafeon","Glaceon",
    /* 472 */ "Gliscor","Mamoswine","Porygon-Z","Gallade","Probopass",
    /* 477 */ "Dusknoir","Froslass","Rotom","Uxie","Mesprit",
    /* 482 */ "Azelf","Dialga","Palkia","Heatran","Regigigas",
    /* 487 */ "Giratina","Cresselia","Phione","Manaphy","Darkrai",
    /* 491 */ "Shaymin","Arceus",
    /* 493 */ "Arceus",  /* alias, last entry */
};

#define SPECIES_COUNT  ((int)(sizeof(k_species) / sizeof(k_species[0])))

const char *species_name(uint16_t species) {
    if (species == 0 || species >= SPECIES_COUNT) return NULL;
    return k_species[species];
}

void species_label(uint16_t species, char *out, int cap) {
    const char *n = species_name(species);
    if (n) snprintf(out, cap, "#%03u %s", species, n);
    else   snprintf(out, cap, "species %u", species);
}

/* ============================================================
 * Name tables loaded at boot from sdmc:/3ds/pokeStride/data/.
 *
 * Each .bin is a tiny self-contained format produced by
 * extract_msg_strings() in nds_extract.c:
 *
 *   u32  magic = 'NMSG'  (0x47534D4E)
 *   u32  count
 *   u32  offsets[count]  ; byte offset from start of file, 0 = missing
 *   u8[] string_data     ; concatenated NUL-terminated UTF-8 strings
 *
 * We mmap-style load the whole file once. Lookups just return
 * buf + offsets[id] without copying — the buffer lives for the
 * lifetime of the process. */

#define NMSG_MAGIC 0x47534D4Eu

typedef struct {
    uint8_t  *buf;       /* whole file in memory; NULL = not loaded */
    size_t    len;
    uint32_t  count;
    const uint32_t *offsets;     /* points into buf */
} name_table_t;

static name_table_t g_items;
static name_table_t g_moves;
static name_table_t g_locations;

static bool load_table(const char *path, name_table_t *t) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        pkw_log("names: missing %s\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 8) { fclose(f); return false; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return false; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return false; }

    uint32_t magic = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                     ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    if (magic != NMSG_MAGIC) {
        pkw_log("names: bad magic in %s (0x%08X)\n", path, (unsigned)magic);
        free(buf);
        return false;
    }
    uint32_t count = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) |
                     ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
    if (8 + (size_t)count * 4 > (size_t)sz) {
        pkw_log("names: %s table truncated\n", path);
        free(buf);
        return false;
    }
    t->buf     = buf;
    t->len     = (size_t)sz;
    t->count   = count;
    t->offsets = (const uint32_t *)(buf + 8);
    pkw_log("names: loaded %s (%u entries, %ld B)\n",
            path, (unsigned)count, sz);
    return true;
}

static const char *lookup(const name_table_t *t, uint16_t id) {
    if (!t->buf || id >= t->count) return NULL;
    uint32_t off = t->offsets[id];
    if (off == 0 || off >= t->len) return NULL;
    return (const char *)(t->buf + off);
}

bool names_init(void) {
    bool a = load_table("sdmc:/3ds/pokeStride/data/names_items.bin",
                        &g_items);
    bool b = load_table("sdmc:/3ds/pokeStride/data/names_moves.bin",
                        &g_moves);
    bool c = load_table("sdmc:/3ds/pokeStride/data/names_locations.bin",
                        &g_locations);
    return a && b && c;
}

const char *item_name(uint16_t id) { return lookup(&g_items, id); }
const char *move_name(uint16_t id) { return lookup(&g_moves, id); }

const char *met_location_name(uint16_t loc) {
    static char buf[16];
    const char *s = lookup(&g_locations, loc);
    if (s) return s;
    if (loc == 233) return "Pokewalker";
    if (loc == 0)   return "(none)";
    snprintf(buf, sizeof(buf), "loc %u", loc);
    return buf;
}
