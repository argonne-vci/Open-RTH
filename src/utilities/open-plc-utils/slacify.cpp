/* 
 * Copyright © 2025, UChicago Argonne, LLC
 * All Rights Reserved
 * Software Name: Remote Test Harness
 * By: Argonne National Laboratory
 * 
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 * Copyright © 2007 Free Software Foundation, Inc. <https://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
 * 
 * See the LICENSE file for the full license text.
 */


#include <time.h>
#include <cstring>
#include <stdlib.h>
#include <limits.h>
#include "slacify.h"

// The following contains C code -----------------------------------------------
extern "C" {
#include "../open-plc-utils/plc/plc.h"
#include "../open-plc-utils/slac/slac.h"
#include "../open-plc-utils/tools/error.h"
#include "../open-plc-utils/tools/flags.h"
#include "../open-plc-utils/tools/memory.h"
#include "../open-plc-utils/tools/number.h"
#include "../open-plc-utils/tools/config.h"

static signed evse_identifier (struct session * session, struct channel * channel);
static signed pev_identifier (struct session * session, struct channel * channel);
void CreateNMK(void);
void CreateNID(void);
static void pev_initialize (struct session * session, char const * profile, char const * section);
static void evse_initialize (struct session * session, char const * profile, char const * section);
static void configure ();
static void UnoccupiedState (struct session * session, struct channel * channel, struct message * message);
static void UnmatchedState (struct session * session, struct channel * channel, struct message * message);
static void MatchedState (struct session * session, struct channel * channel, struct message * message);
int EVSESlac (signed c);
static void PEV_DisconnectedState (struct session * session, struct channel * channel, struct message * message);
static void PEV_UnmatchedState (struct session * session, struct channel * channel, struct message * message);
static void PEV_MatchedState (struct session * session, struct channel * channel, struct message * message);
int PEVSlac(signed c);

// Define structs to hold session details
extern struct channel channel;
struct session Session;
struct message Message;

// PLC constants
#define PLCDEVICE    "PLC"
#define EVSE_PROFILE "evse.ini"
#define PEV_PROFILE  "pev.ini"
#define SECTION      "default"
#define STATION      ""

#define PEV_VID      "0000000000000000000000000000000000" /* VehicleIdentifier*/
#define PEV_NMK      "50D3E4933F855B7040784DF815AA8DB7"   /* HomePlugAV*/
#define PEV_NID      "B0F2E695666B03"		     /* HomePlugAV*/

char const *profile;
char const *section;

// Define EVSE and PEV states to use in respective state machines
enum evse_state_t
{
    EVSE_STATE_UNAVAILABLE = 0,
    EVSE_STATE_UNOCCUPIED  = 1,
    EVSE_STATE_UNMATCHED   = 2,
    EVSE_STATE_MATCHED     = 3
};

enum pev_state_t
{
    PEV_STATE_DISCONNECTED = 1,
    PEV_STATE_UNMATCHED    = 2,
    PEV_STATE_MATCHED      = 3,
    PEV_STATE_UNAVAILABLE  = 4
};

uint8_t EVSE_NMK[HPAVKEY_NMK_LEN];
uint8_t EVSE_NID[HPAVKEY_NID_LEN];

uint8_t C_sequ_retry;
int slac_timeout_sec, num_retries;
int m_slac_limit;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// Identifier functions for EVSE and PEV. These functions will copy channel host
// address and run ID to session EVSE/PEV MAC addresses.
// -----------------------------------------------------------------------------
static signed evse_identifier (struct session * session, struct channel * channel)

{
	memcpy (session->EVSE_MAC, channel->host, sizeof (session->EVSE_MAC));
	return (0);
}

static signed pev_identifier (struct session * session, struct channel * channel)

{
	time_t now;
	time (&now);
	memset (session->RunID, 0, sizeof (session->RunID));
	memcpy (session->RunID, channel->host, ETHER_ADDR_LEN);
	memcpy (session->PEV_MAC, channel->host, sizeof (session->PEV_MAC));
	return (0);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// NMK and keygeneration functions for EVSE
// -----------------------------------------------------------------------------
void CreateNMK(void)
{
    const char *cmd = "date +%s | sha256sum | base64 | head -c 32 ; echo";  //randomish seed
    char buf[35];
    FILE *ptr;

	if ((ptr = popen(cmd, "r")) != NULL)
	{
		while (fgets(buf, sizeof(buf), ptr) != NULL)
		{
			//printf("%s", buf);
		}
		pclose(ptr);
	}

	HPAVKeyNMK(EVSE_NMK, buf);
	/*for (i = 0;  i < HPAVKEY_NMK_LEN; i++)
	{
	printf ("%02x", EVSE_NMK[i]);
	printf ("\n");
	}
	printf ("\n");*/
}

void CreateNID(void)
{
	HPAVKeyNID(EVSE_NID, EVSE_NMK, 0);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// These functions read EVSE/PEV-HLE configuration profiles and set session variable
// -----------------------------------------------------------------------------
static void pev_initialize (struct session * session, char const * profile, char const * section)
{
	session->next = session->prev = session;
	hexencode (session->PEV_ID, sizeof (session->PEV_ID), configstring (profile, section, "VehicleIdentifier", PEV_VID));
	hexencode (session->NMK, sizeof (session->NMK), configstring (profile, section, "NetworkMembershipKey", PEV_NMK));
	hexencode (session->NID, sizeof (session->NID), configstring (profile, section, "NetworkIdentifier", PEV_NID));
	session->limit = confignumber (profile, section, "AttenuationThreshold", m_slac_limit);
	session->pause = confignumber (profile, section, "MSoundPause", SLAC_PAUSE);
	session->state = PEV_STATE_DISCONNECTED;
	memcpy (session->original_nmk, session->NMK, sizeof (session->original_nmk));
	memcpy (session->original_nid, session->NID, sizeof (session->original_nid));
	slac_session (session);
	return;
}


static void evse_initialize (struct session * session, char const * profile, char const * section)
{
	char nmk[(HPAVKEY_NMK_LEN*2)+1];
	char nid[(HPAVKEY_NID_LEN*2)+1];
	int i;

	CreateNMK();
	CreateNID();

	for (i = 0;  i < HPAVKEY_NMK_LEN; i++)
	{
		sprintf(&nmk[i*2],"%02x", EVSE_NMK[i]); /*convert uint8_t array to string*/
	}
	for (i = 0;  i < HPAVKEY_NID_LEN; i++)
	{
		sprintf(&nid[i*2],"%02x", EVSE_NID[i]); /*convert uint8_t array to string*/
	}
	session->next = session->prev = session;
	hexencode (session->NMK, sizeof (session->NMK), configstring (profile, section, "NetworkMembershipKey", nmk));
	hexencode (session->NID, sizeof (session->NID), configstring (profile, section, "NetworkIdentifier", nid));
	session->NUM_SOUNDS = confignumber (profile, section, "NumberOfSounds", SLAC_MSOUNDS);
	session->TIME_OUT = confignumber (profile, section, "TimeToSound", SLAC_TIMETOSOUND);
	session->RESP_TYPE = confignumber (profile, section, "ResponseType", SLAC_RESPONSE_TYPE);
	session->state = EVSE_STATE_UNOCCUPIED;
	slac_session (session);

	return;
}
// -----------------------------------------------------------------------------









// #############################################################################
// #############################################################################
//                        EVSE state machine functions
// #############################################################################
// #############################################################################


/*====================================================================*
 *
 *   void configure ()
 *
 *   print default EVSE-HLE configuration file on stdout so that the
 *   profile, section and element names match;
 *
 *--------------------------------------------------------------------*/

static void configure ()

{
	int i;
	/*printf ("# file: %s\n", PROFILE);*/
	printf ("# ====================================================================\n");
	printf ("# EVSE-HLE initiaization;\n");
	printf ("# --------------------------------------------------------------------\n");
	printf ("[%s]\n", SECTION);
	printf ("network membership key = ");
	for (i = 0;  i < HPAVKEY_NMK_LEN; i++)
		{
			printf ("%02x", EVSE_NMK[i]);
		}
		printf ("\n");
	printf ("network identifier = ");
	for (i = 0;  i < HPAVKEY_NID_LEN; i++)
		{
			printf ("%02x", EVSE_NID[i]);
		}
		printf ("\n");
	printf ("number of sounds = %d\n", SLAC_MSOUNDS);
	printf ("time to sound = %d\n", SLAC_TIMETOSOUND);
	printf ("response type = %d\n", SLAC_RESPONSE_TYPE);
	return;
}


/*====================================================================*
 *
 *   void UnoccupiedState (struct session * session, struct channel * channel, struct message * message);
 *
 *--------------------------------------------------------------------*/

static void UnoccupiedState (struct session * session, struct channel * channel, struct message * message)
{
	 __time_t CurrentTime, DeltaTime, InitTime;
	slac_session (session);
	debug (0, __func__, "Listening ...");

	InitTime = time(0);

	while (evse_cm_slac_param (session, channel, message))
	{
		CurrentTime = time(0);
		DeltaTime = CurrentTime - InitTime;
		if(DeltaTime >= slac_timeout_sec) /*allow EVSE 20 Seconds to receive CM_SLAC_PARAM.REQ*/
		{
			debug (0, __func__, "CM_SLAC_PARAM.REQ TIMEOUT (20 sec)!");
            debug (0, __func__, "Retrying ... ");
			sleep(5);
			if(C_sequ_retry >= num_retries)
			{
				session->state = EVSE_STATE_UNAVAILABLE;
				C_sequ_retry = 0; /*clear flag*/
			}
			else
			{
			  session->state = EVSE_STATE_UNOCCUPIED;
			  C_sequ_retry++;
			}
			return;
		}
	}

	session->state = EVSE_STATE_UNMATCHED;
	return;
}


/*====================================================================*
 *
 *   void MatchingState (struct session * session, struct channel * channel, struct message * message);
 *
 *   the cm_start_atten_char message establishes msound count and
 *   timeout;
 *
 *--------------------------------------------------------------------*/

static void UnmatchedState (struct session * session, struct channel * channel, struct message * message)
{
	slac_session (session);
	debug (0, __func__, "Sounding ...");
	if (evse_cm_start_atten_char (session, channel, message))
	{
		session->state = EVSE_STATE_UNOCCUPIED;
		return;
	}
	if (evse_cm_mnbc_sound (session, channel, message))
	{
		session->state = EVSE_STATE_UNOCCUPIED;
		return;
	}
	if (evse_cm_atten_char (session, channel, message))
	{
		session->state = EVSE_STATE_UNOCCUPIED;
		return;
	}
	debug (0, __func__, "Matching ...");
	if (evse_cm_slac_match (session, channel, message))
	{
		session->state = EVSE_STATE_UNOCCUPIED;
		return;
	}
	session->state = EVSE_STATE_MATCHED;
	return;
}


/*====================================================================*
 *
 *   void MatchedState (struct session * session, struct channel * channel, struct message * message);
 *
 *--------------------------------------------------------------------*/

static void MatchedState (struct session * session, struct channel * channel, struct message * message)
{
	__time_t CurrentTime, DeltaTime, InitTime;
	struct plc plc;
	debug (0, __func__, "Connecting ...");
	InitTime = time(0);  /*start TT_match_join timer (12 sec)*/

	/*determine if logical network has been established*/
	_setbits (plc.flags, PLC_NETWORK);
	plc.channel = channel;
	plc.message = message;
	while (NetInfo2 (&plc))
	{
		CurrentTime = time(0);
		DeltaTime = CurrentTime - InitTime;
		if(DeltaTime >= slac_timeout_sec) /*allow PEV 12 Seconds to join AVLN*/
		{
			session->state = EVSE_STATE_UNOCCUPIED; /*timer has expired before logical network reset state machine*/
			debug (0, __func__, "Timeout: PEV did not join logical network (30 sec)!");
			return;
		}
	}

	return;
}



/*====================================================================*
 *
 *   int EVSESlac (signed c);
 *
 *--------------------------------------------------------------------*/

int EVSESlac (signed c)
{
	channel.timeout = SLAC_TIMEOUT;
	if (getenv (PLCDEVICE))
	{
		channel.ifname = strdup (getenv (PLCDEVICE));
	}
	switch (c)
		{
		case 'c':
			configure ();
			return (0);
		case 'C':
			_setbits (Session.flags, SLAC_COMPARE);
			break;
		case 'd':
			_setbits (Session.flags, (SLAC_VERBOSE | SLAC_SESSION));
			break;
		case 'i':
			channel.ifname = optarg;
			break;
		case 'p':
			profile = optarg;
			break;
		case 's':
			section = optarg;
			break;
		case 'q':
			_setbits (channel.flags, CHANNEL_SILENCE);
			break;
		case 't':
			channel.timeout = (signed)(uintspec (optarg, 0, UINT_MAX));
			break;
		case 'v':
			_setbits (channel.flags, CHANNEL_VERBOSE);
			break;
		case 'x':
			Session.exit = Session.exit? 0: 1;
			break;
		default:
			break;
	}

	while (Session.state)
	{
		if (Session.state == EVSE_STATE_UNOCCUPIED)
		{
			UnoccupiedState (&Session, &channel, &Message);
			continue;
		}
		if (Session.state == EVSE_STATE_UNMATCHED)
		{
			UnmatchedState (&Session, &channel, &Message);
			continue;
		}
		if (Session.state == EVSE_STATE_MATCHED)
		{
			MatchedState (&Session, &channel, &Message);
			break;
		}
		debug (1, __func__, "Illegal state!");
	}
	closechannel (&channel);

	if (Session.state == EVSE_STATE_MATCHED)
	{
	    return (0);
	}
	else
	{
		return 1;
	}
}








// #############################################################################
// #############################################################################
//                         PEV state machine functions
// #############################################################################
// #############################################################################





/*====================================================================*
 *
 *   void DisconnectedState (struct session * session, struct channel * channel, struct message * message);
 *
 *--------------------------------------------------------------------*/

static void PEV_DisconnectedState (struct session * session, struct channel * channel, struct message * message)
{
	__time_t CurrentTime, DeltaTime, InitTime;

	slac_session (session);
	debug (0, __func__, "Probing ...");
	memincr (session->RunID, sizeof (session->RunID));

	InitTime = time(0);

	while (pev_cm_slac_param (session, channel, message))
	{
			CurrentTime = time(0);
			DeltaTime = CurrentTime - InitTime;

			if(DeltaTime >= slac_timeout_sec) /*allow PEV 20 Seconds to receive CM_SLAC_PARAM.CONF*/
			{
				debug (0, __func__, "CM_SLAC_PARAM.CONF TIMEOUT (20 sec)!");
				if(C_sequ_retry >= num_retries)
				{
					session->state = PEV_STATE_UNAVAILABLE;
					C_sequ_retry = 0; /*clear flag*/
					/*Show Message on EVSE Screen*/
				}
				else
				{
				  session->state = PEV_STATE_DISCONNECTED;
				  C_sequ_retry++;
				}
				return;
			}
		}
	session->state = PEV_STATE_UNMATCHED;
	return;
}


/*====================================================================*
 *
 *   void MatchingState (struct session * session, struct channel * channel, struct message * message);
 *
 *   The PEV-EVSE perform GreenPPEA protocol in this state;
 *
 *   the cm_start_atten_char and cm_mnbc_sound messages are sent
 *   broadcast; the application may receive multiple cm_atten_char
 *   messages before sending the cm_slac_match message;
 *
 *--------------------------------------------------------------------*/

static void PEV_UnmatchedState (struct session * session, struct channel * channel, struct message * message)
{
	slac_session (session);
	debug (0, __func__, "Sounding ...");
	/*Send 3 CM_START_ATTEN_CHAR.IND*/
	if (pev_cm_start_atten_char (session, channel, message))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	/*Send the number of sounding packets requested by EVSE*/
	if (pev_cm_mnbc_sound (session, channel, message))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	/*Wait to receive the results of the sounds (CM_ATTEN_CHAR.IND)*/
	if (pev_cm_atten_char (session, channel, message))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	/*Determine if we are connected to this EVSE*/
	if (slac_connect (session))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	/*Request NMK info*/
	debug (0, __func__, "Matching ...");
	if (pev_cm_slac_match (session, channel, message))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	session->state = PEV_STATE_MATCHED;
	return;
}


/*====================================================================*
 *
 *   void MatchedState (struct session * session, struct channel * channel, struct message * message);
 *
 *   charge vehicle; restore original NMK/NID and disconnect; loop
 *   if SLAC_CONTINUE is set;
 *
 *--------------------------------------------------------------------*/

static void PEV_MatchedState (struct session * session, struct channel * channel, struct message * message)
{
	// unsigned state = 0;
	slac_session (session);
	struct plc plc;

#if SLAC_FORMAVLN

	debug (0, __func__, "Connecting ...");
	if (pev_cm_set_key (session, channel, message))
	{
		session->state = PEV_STATE_DISCONNECTED;
		return;
	}
	// sleep (session->settletime); /*wait here for adapters to form AVLN*/
	/*determine if logical network has been established*/
	_setbits (plc.flags, PLC_NETWORK);
	plc.channel = channel;
	plc.message = message;
	while (NetInfo2 (&plc))
	{
		// if (PollSockets() == -1)
		//   {
		// 	break;  /*close application*/
		//   }
	}

	debug (0, __func__, "AVLN Formed.");
#endif

	session->state = 0;
	return;
}



/*====================================================================*
 *
 *   int main (int argc, char * argv[]);
 *
 *
 *--------------------------------------------------------------------*/

int PEVSlac(signed c)
{
	channel.timeout = SLAC_TIMEOUT;
	if (getenv (PLCDEVICE))
	{
		channel.ifname = strdup (getenv (PLCDEVICE));
	}

	switch (c)
		{
		case 'c':
			configure ();
			return (0);
		case 'C':
			_setbits (Session.flags, SLAC_COMPARE);
			break;
		case 'd':
			_setbits (Session.flags, (SLAC_VERBOSE | SLAC_SESSION));
			break;
		case 'i':
			channel.ifname = optarg;
			break;
		case 'l':
			Session.state = PEV_STATE_DISCONNECTED;
			break;
		case 'p':
			profile = optarg;
			break;
		case 's':
			section = optarg;
			break;
		case 'q':
			_clrbits (channel.flags, CHANNEL_SILENCE);
			break;
		case 't':
			channel.timeout = (unsigned)(uintspec (optarg, 0, UINT_MAX));
			break;
		case 'v':
			_setbits (channel.flags, CHANNEL_VERBOSE);
			break;
		case 'x':
			Session.exit = Session.exit? 0: 1;
			break;
		default:
			break;
		}

	while (Session.state)
	{
		if (Session.state == PEV_STATE_DISCONNECTED)
		{
			PEV_DisconnectedState (&Session, &channel, &Message);
			continue;
		}
		if (Session.state == PEV_STATE_UNMATCHED)
		{
			PEV_UnmatchedState (&Session, &channel, &Message);
			continue;
		}
		if (Session.state == PEV_STATE_MATCHED)
		{
			PEV_MatchedState (&Session, &channel, &Message);
			break;
		}
		if (Session.state == PEV_STATE_UNAVAILABLE)
		{
			break;
		}

		debug (1, __func__, "Illegal state!");
	}
	closechannel (&channel);
	if (Session.state == PEV_STATE_UNAVAILABLE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}



// #############################################################################
// #############################################################################
//                         slacify class functions
// #############################################################################
// #############################################################################




// -----------------------------------------------------------------------------
// init() - Call this to initialize SLAC based on device type and SLAC parameter
//          Returns 1 if succesful, and 0 if not.
// -----------------------------------------------------------------------------
int CSLACify::init(int device_type, signed slac_setting, int slac_limit, int timeout, int retries)
{
    memset (&Session, 0, sizeof (Session));
    memset (&Message, 0, sizeof (Message));

    // Save the device type, debug setting, timeout and retries to use where needed
    m_device_type = device_type;
    m_setting = slac_setting;
	m_slac_limit = slac_limit;
    slac_timeout_sec = timeout;
    num_retries = retries;
	
    // Open a raw Ethernet channel
    openchannel (&channel);

    // Initialize settings based on device type
    if (m_device_type == EVSE)
    {
        profile = EVSE_PROFILE;
        evse_identifier (&Session, &channel);
	    evse_initialize (&Session, profile, section);
        if (evse_cm_set_key (&Session, &channel, &Message))
        {
            debug (0, __func__, "Can't set EVSE key.");
            return 0;
        }

	    return 1;
    }

    else if (m_device_type == PEV)
    {
        profile = PEV_PROFILE;
        pev_identifier (&Session, &channel);
	    pev_initialize (&Session, profile, section);
		
        return 1;
    }

	else
		return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// connect() - Call this to attempt SLAC and creating a logical network
// -----------------------------------------------------------------------------
int CSLACify::connect()
{
	// Call the appropriate SLAC functions based on device type
    if (m_device_type == EVSE)
    {
		if (EVSESlac(m_setting) == 0)
		{
			printf ("EVSE SLAC success - Connection established\n");
			return 1;
		}
		else
		{
			printf("EVSE SLAC failed\n");
			return 0;
		}  
    }

    if (m_device_type == PEV)
    {
		if(PEVSlac(m_setting) == 0)
		{
			printf ("PEV SLAC success - Connection established\n");
			return 1;
		}
		else
		{
			printf("PEV SLAC failed\n");
			return 0;
		}
    }

	// If we get here, device type is wrong
	return -1;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// close() - Call this to disconnect an established AVLN
// -----------------------------------------------------------------------------
void CSLACify::close()
{
    closechannel (&channel);
}
// -----------------------------------------------------------------------------



//==========================================================================================================
