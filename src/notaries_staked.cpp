
#include "notaries_staked.h"
#include "crosschain.h"
#include "cc/CCinclude.h"
#include "komodo_defs.h"
#include <cstring>

uint8_t is_LABSCHAIN(const char *chain_name) 
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
}

int32_t get_LABS_ERA(uint32_t timestamp)
{
    int32_t i,era = 0;
    if (timestamp <= LABS_NOTARIES_TIMESTAMP[0])
        return(0);
    for (i = 1; i < NUM_LABS_ERAS; i++)
    {
        if (timestamp <= LABS_NOTARIES_TIMESTAMP[i] && timestamp > (LABS_NOTARIES_TIMESTAMP[i-1] + LABS_ERA_GAP))
            return(i);
    }
    return(-1);
}

int8_t num_LABSNotaries(uint8_t* pubkeysp, int32_t era) 
{
    int32_t i, num = 0;
    static uint8_t labs_pubkeys[NUM_LABS_ERAS][64][33],didinit[NUM_LABS_ERAS];
    if ( era < 0 )
    {
        memset(pubkeysp,0,64 * 33);
        return(64);
    }
    else if ( era < NUM_LABS_ERAS )
    {
        if ( didinit[era] == 0 )
        {
            for (i=0; i<num_notaries_LABS[era]; i++) 
                decode_hex(labs_pubkeys[era][i],33,(char *)notaries_LABS[era][i][1]);
            didinit[era] = 1;
        }
        memcpy(pubkeysp,labs_pubkeys[era],num_notaries_LABS[era] * 33);
        num = num_notaries_LABS[era];
    }
    return(num);
}

int32_t LABSMINSIGS2(int32_t num_notaries, int32_t era)
{
    int32_t n, minsigs = 0;
    if ( era >= 0 )
    {
        n = num_notaries != 0 ? num_notaries : num_notaries_LABS[era];
        minsigs = n/5;
        if ( ++minsigs < LABS_MinSigs[era] )
            return(LABS_MinSigs[era]);
    }
    return minsigs;
}

int32_t LABSMINSIGS(int32_t num_notaries, uint32_t timestamp)
{
    if (num_notaries == 64) // notary pay, calls this for KMD notaries if notarypay is used. 
        return(num_notaries/5);
    return(LABSMINSIGS2(num_notaries, get_LABS_ERA(timestamp)));
}

CrosschainAuthority Choose_Crosschain_auth(int32_t era) 
{
  CrosschainAuthority auth;
  if ( era >= 0 )
  {
      auth.requiredSigs = LABSMINSIGS2(0,era);
      auth.size = num_notaries_LABS[era];
      for (int n=0; n<auth.size; n++)
          for (size_t i=0; i<33; i++)
              sscanf(notaries_LABS[era][n][1]+(i*2), "%2hhx", auth.notaries[n]+i);
  }
  return auth;
};
