#ifndef CLAIM_UI_H
#define CLAIM_UI_H

/* HGSS exposes claim and return as independent actions when you sync
 * the walker; we mirror that. claim_flow() does both in one go for
 * convenience after a full walk session. */
void claim_flow(void);          /* Claim caught + Return walker Pokemon */
void claim_caught_flow(void);   /* Only deposit caught + sync watts/steps */
void return_pokemon_flow(void); /* Only return WalkerPair to a box */

#endif
