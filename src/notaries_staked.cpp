
#include "notaries_staked.h"
#include "crosschain.h"
#include "cc/CCinclude.h"
#include "komodo_defs.h"
#include <cstring>

extern pthread_mutex_t LABS_mutex;

int8_t is_LABSCHAIN(const char *chain_name) 
{
    static int8_t LABS,doneinit;
    if ( chain_name[0] == 0 )
        return(0);
    if (doneinit == 1 && ASSETCHAINS_SYMBOL[0] != 0)
        return(LABS);
    else LABS = 0;
    if ( (strcmp(chain_name, "LABS") == 0) ) 
        LABS = 1; // These chains are allowed coin emissions.
    else if ( (strncmp(chain_name, "LABS", 4) == 0) ) 
        LABS = 2; // These chains have no coin emission, block subsidy is always 0, and comission is 0. Notary pay is allowed.
    else if ( (strcmp(chain_name, "CFEK") == 0) || (strncmp(chain_name, "CFEK", 4) == 0) )
        LABS = 3; // These chains have no speical rules at all.
    else if ( (strcmp(chain_name, "TEST") == 0) || (strncmp(chain_name, "TEST", 4) == 0) )
        LABS = 4; // These chains are for testing consensus to create a chain etc. Not meant to be actually used for anything important.
    else if ( (strcmp(chain_name, "THIS_CHAIN_IS_BANNED") == 0) )
        LABS = 255; // Any chain added to this group is banned, no notarisations are valid, as a consensus rule. Can be used to remove a chain from cluster if needed.
    doneinit = 1;
    return(LABS);
};

int32_t get_LABS_ERA(int timestamp)
{
    int8_t era = 0;
    if (timestamp <= LABS_NOTARIES_TIMESTAMP[0])
        return(1);
    for (int32_t i = 1; i < NUM_LABS_ERAS; i++)
    {
        if (timestamp <= LABS_NOTARIES_TIMESTAMP[i] && timestamp >= (LABS_NOTARIES_TIMESTAMP[i-1] + LABS_ERA_GAP))
            return(i+1);
    }
  // if we are in a gap, return era 0, this allows to invalidate notarizations when in GAP.
  return(0);
};

int8_t LABS_NotaryID(std::string &notaryname, char *Raddress) {
    if ( LABS_ERA != 0 )
    {
        for (int8_t i = 0; i < num_notaries_LABS[LABS_ERA-1]; i++) {
            if ( strcmp(Raddress,NOTARYADDRS[i]) == 0 ) {
                notaryname.assign(notaries_LABS[LABS_ERA-1][i][0]);
                return(i);
            }
        }
    }
    return(-1);
}

int8_t num_LABSNotaries(uint8_t pubkeys[64][33],int8_t era) {
    int i; int8_t retval = 0;
    static uint8_t staked_pubkeys[NUM_LABS_ERAS][64][33],didinit[NUM_LABS_ERAS];
    static char ChainName[65];

    if ( ChainName[0] == 0 )
    {
        if ( ASSETCHAINS_SYMBOL[0] == 0 )
            strcpy(ChainName,"KMD");
        else
            strcpy(ChainName,ASSETCHAINS_SYMBOL);
    }

    if ( era == 0 )
    {
        // era is zero so we need to null out the pubkeys.
        memset(pubkeys,0,64 * 33);
        //printf("%s is a LABS chain and is in an ERA GAP.\n",ChainName);
        return(64);
    }
    else
    {
        if ( didinit[era-1] == 0 )
        {
            for (i=0; i<num_notaries_LABS[era-1]; i++) {
                decode_hex(staked_pubkeys[era-1][i],33,(char *)notaries_LABS[era-1][i][1]);
            }
            didinit[era-1] = 1;
            //printf("%s is a LABS chain in era %i \n",ChainName,era);
        }
        memcpy(pubkeys,staked_pubkeys[era-1],num_notaries_LABS[era-1] * 33);
        retval = num_notaries_LABS[era-1];
    }
    return(retval);
}

void UpdateLABSNotaryAddrs(uint8_t pubkeys[64][33],int8_t numNotaries) 
{
    static int didinit;
    if ( didinit == 0 ) {
        pthread_mutex_init(&LABS_mutex,NULL);
        didinit = 1;
    }
    if ( pubkeys[0][0] == 0 )
    {
        // null pubkeys, era 0.
        pthread_mutex_lock(&LABS_mutex);
        memset(NOTARYADDRS,0,sizeof(NOTARYADDRS));
        pthread_mutex_unlock(&LABS_mutex);
    }
    else
    {
        // labs era is set.
        pthread_mutex_lock(&LABS_mutex);
        for (int i = 0; i<numNotaries; i++)
        {
            pubkey2addr((char *)NOTARYADDRS[i],(uint8_t *)pubkeys[i]);
            if ( memcmp(NOTARY_PUBKEY33,pubkeys[i],33) == 0 )
            {
                NOTARY_ADDRESS.assign(NOTARYADDRS[i]);
                IS_LABS_NOTARY = i;
                IS_KOMODO_NOTARY = 0;
            }
        }
        pthread_mutex_unlock(&LABS_mutex);
    }
}

int32_t LABSMINSIGS(int32_t numSN, uint32_t timestamp)
{
    /* 
        This will need some kind of height or timestamp activation to work properly! 
        Leaving min sigs at 7 for now is the safest,
    */
    int32_t minsigs;
    if (numSN/5 > overrideMinSigs )
        minsigs = (numSN / 5) + 1;
    else
        minsigs = overrideMinSigs; 
    return numSN/5;
}

CrosschainAuthority Choose_Crosschain_auth(int32_t chosen_era) {
  CrosschainAuthority auth;
  auth.requiredSigs = (num_notaries_LABS[chosen_era-1] / 5);
  auth.size = num_notaries_LABS[chosen_era-1];
  for (int n=0; n<auth.size; n++)
      for (size_t i=0; i<33; i++)
          sscanf(notaries_LABS[chosen_era-1][n][1]+(i*2), "%2hhx", auth.notaries[n]+i);
  return auth;
};
