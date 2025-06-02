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


//==========================================================================================================
// All SDP related functions
//==========================================================================================================

#include <ifaddrs.h>

#include "sdp.h"

// This is the interface over which HLC occurs
std::string network_interface = "qca0";

// Flag to check if SDP is initialized
int SDP_init = 0;

// This is the port number for SDP
int SDP_port = 15118;

// This is the maximum SDP message size
#define MAX_MSG_SIZE 100
#define SDP_REQUEST_LENGTH 10 
#define SDP_RESPONSE_LENGTH 28 // payload = 16 IPv6 address, 2 for port, 1 for transport, 1 for security, 8 for V2GTP header

// This is the byte array that holds the SDP header and payload
uint8_t v2gtp_outStream[MAX_MSG_SIZE];
uint8_t v2gtp_inStream[MAX_MSG_SIZE];

// These are the V2GTP constants
#define V2GTP_VERSION           0x01
#define V2GTP_VERSION_INV       0xFE
#define V2GTP_EXI_TYPE          0x8001
#define V2GTP_SDP_REQUEST_TYPE  0x9000
#define V2GTP_SDP_RESPONSE_TYPE 0x9001
#define V2GTP_HEADER_LENGTH     8

enum
{
	Secured_w_TLS=0x00,
	No_Transport_Layer_Security=0x10
};

enum
{
	SDP_TCP=0x00,
	SDP_UDP=0x10
};


// Global SECC variables
uint16_t SECC_port;
struct sockaddr_in6 SECCAddr;
char SECC_ipv6addr[INET6_ADDRSTRLEN];
std::string ipv6_str;

int get_ephemeral_port(int sd);

// -----------------------------------------------------------------------------
// create_v2gtp_header() - Implements the V2GTP header based on DIN 70121 (pg 82)
// -----------------------------------------------------------------------------
int create_v2gtp_header(uint8_t* v2gtp_message, uint16_t payload_type, uint32_t payload_length)
{
    // First two bytes are the protocol version and its bitwise inverse
    v2gtp_message[0] = V2GTP_VERSION;
    v2gtp_message[1] = V2GTP_VERSION_INV;

    // Next two bytes are the Payload type
    v2gtp_message[3] = payload_type & 0xFF;
    v2gtp_message[2] = (payload_type >> 8) & 0xFF;

    // Last four bytes are the payload length
    v2gtp_message[7] = payload_length & 0xFF;
    v2gtp_message[6] = (payload_length >> 8) & 0xFF;
    v2gtp_message[5] = (payload_length >> 16) & 0xFF;
    v2gtp_message[4] = (payload_length >> 24) & 0xFF;

    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// create_SDP_request() - This will create an SDP request sent by EV
// -----------------------------------------------------------------------------
int create_SDP_request(uint8_t* v2gtp_message, int security_type, int comm_protocol_type)
{
    // Define the payload length 
    int SDP_payload_length = 2;

    // Create the header
    create_v2gtp_header(v2gtp_message, V2GTP_SDP_REQUEST_TYPE, SDP_payload_length);
    
    // Create rest of the payload
    v2gtp_message[8] = security_type;
    v2gtp_message[9] = comm_protocol_type;

    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// get_IPv6() - This will get the IPv6 of a particular network interface
// -----------------------------------------------------------------------------
int get_IPv6(const char *iface_name, char *ipv6add)
{
    

    struct ifaddrs *ifaddr, *ifa;
    int family, s, i;

    // Get the list of network interfaces
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Walk through the linked list of network interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Skip interfaces that are not IPv6 or do not match the given interface name
        if (ifa->ifa_addr->sa_family != AF_INET6 || strcmp(ifa->ifa_name, iface_name) != 0)
            continue;

        family = ifa->ifa_addr->sa_family;

        // For an IPv6 interface address, retrieve the address
        if (family == AF_INET6 && strcmp(ifa->ifa_name, iface_name) == 0)
        {
            s = getnameinfo(ifa->ifa_addr,
                            sizeof(struct sockaddr_in6),
                            ipv6add, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
            }

            // Remove the '%' and interface index from the IPv6 address
            for (i = 0; i <= strlen(ipv6add); ++i) {
                if (ipv6add[i] == '%') {
                    break;
                }
            }
            ipv6add[i] = '\0';
            printf("\nSECC IPv6 address: %s\n", ipv6add);
        }
    }

    // Free the linked list of network interfaces
    freeifaddrs(ifaddr);

    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// create_SDP_response() - This will create an SDP response sent by EVSE
// -----------------------------------------------------------------------------
int create_SDP_response(uint8_t* v2gtp_message)
{
    // This is the IPv6 address of the device
    char ipv6str[INET6_ADDRSTRLEN];

    // Define the payload length 
    int SDP_payload_length = 20; // 16 IPv6 bytes, 2 port bytes, 1 security, 1 comm type

    // Create the header
    create_v2gtp_header(v2gtp_message, V2GTP_SDP_RESPONSE_TYPE, SDP_payload_length);
    
    // Get IPv6 address of the EVSE
    if(get_IPv6((char *)network_interface.c_str(), (char *)&ipv6str) != 0)
	{
		fprintf(stderr, "Could not read %s interface IPv6 address\n", network_interface.c_str());
        return -1;
	}
	else
	{

		// Convert IP from numbers and dots to binary notation and save to SECC ipv6 address to global structure
		if(inet_pton(AF_INET6,ipv6str, (void *)&SECCAddr.sin6_addr.s6_addr) <= 0)
        {
			fprintf(stderr, "Bad IPv6 Address ");
            return -1;
		}
	}

    // Write the IPv6 address to the SDP response payload
	for(int i=0;i<=15;i++)
	{
		v2gtp_outStream[V2GTP_HEADER_LENGTH+i] = SECCAddr.sin6_addr.s6_addr[i];
	}

    // Write the port number for the SECC TCP/IP server
    v2gtp_outStream[24] = (uint8_t)(TCP_port & 0xFF); // Port 65535
    v2gtp_outStream[25] = (uint8_t)((TCP_port >> 8) & 0xFF);

    v2gtp_outStream[26] = No_Transport_Layer_Security;
    v2gtp_outStream[27] = SDP_TCP;

    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// parse_SDP_message() - Parse SDP message based on device type
// -----------------------------------------------------------------------------
int parse_SDP_message()
{
    uint16_t payload_type;
    uint32_t payload_length;
    uint8_t security, transport;
    uint16_t SECCPort;
    uint8_t ipv6[16];
    char ipv6str[INET6_ADDRSTRLEN];

    if (config.device_type == "EVSE")
    {
        // Check if we support this v2gtp version
	    if(v2gtp_inStream[0] != V2GTP_VERSION && v2gtp_inStream[1] != V2GTP_VERSION_INV)
		    return -1;

        // Check if we support this payload type
        payload_type = v2gtp_inStream[2];
        payload_type = (payload_type << 8 | v2gtp_inStream[3]);
        if (payload_type != V2GTP_SDP_REQUEST_TYPE) return -1;

        // Check if the payload length is correct
        payload_length = v2gtp_inStream[4];
	    payload_length = (payload_length << 8 | v2gtp_inStream[5]);
	    payload_length = (payload_length << 16 | v2gtp_inStream[6]);
	    payload_length = (payload_length << 24 | v2gtp_inStream[7]);
        if (payload_length != 2) return -1;

        // Now parse the security and transport settings
        security = v2gtp_inStream[8];
	    transport = v2gtp_inStream[9];
        printf("Security: %d\n", security); // for debugging
        if ((security == No_Transport_Layer_Security) && (transport == SDP_TCP))
        {
            printf("\nValid DC Charging SDP Request\n");
            printf("Security byte = %d\n",security);
            printf("Transport byte = %d\n",transport);
        }
        else
        {
            printf("\nInvalid DC Charging SDP Request\n");
        }
    }

    else if (config.device_type == "EV")
    {
        // Check if we support this v2gtp version
	    if(v2gtp_inStream[0] != V2GTP_VERSION && v2gtp_inStream[1] != V2GTP_VERSION_INV)
		    return -1;

        // Check if we support this payload type
        payload_type = v2gtp_inStream[2];
        payload_type = (payload_type << 8 | v2gtp_inStream[3]);
        if (payload_type != V2GTP_SDP_RESPONSE_TYPE) return -1;

        // Check if the payload length is correct
        payload_length = v2gtp_inStream[4];
	    payload_length = (payload_length << 8 | v2gtp_inStream[5]);
	    payload_length = (payload_length << 16 | v2gtp_inStream[6]);
	    payload_length = (payload_length << 24 | v2gtp_inStream[7]);
        if (payload_length != 20) return -1;

        // Parse the security and transport settings
        security = v2gtp_inStream[26];
	    transport = v2gtp_inStream[27];
        if ((security == No_Transport_Layer_Security) && (transport == SDP_TCP))
        {
            printf("\nValid SDP Response\n");
            printf("Security byte = %d\n",security);
            printf("Transport byte = %d\n",transport);
        }
        else
        {
            printf("\nInvalid DC Charging SDP Response\n");
            return -1;
        }

        // Parse the SECC IPv6 port
        SECCPort = v2gtp_inStream[24] << 8;
	    SECCPort = SECCPort | v2gtp_inStream[25];

        // Save SECC port to global variable
        SECC_port = SECCPort;

        // Parse the SECC IPv6 address
        for(int i=0;i<=15;i++)
        {
            ipv6[i] = v2gtp_inStream[V2GTP_HEADER_LENGTH+i];
        }

        // Convert the IPv6 address from bytes to dotted notation
        if(inet_ntop(AF_INET6, (void *)ipv6, ipv6str, INET6_ADDRSTRLEN) == NULL)
        {
            fprintf(stderr, "Could not convert byte to address\n");
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }

        // Convert IP from numbers and dots to binary notation and save to SECC ipv6 address to global structure
	    if(inet_pton(AF_INET6,ipv6str, (void *)&SECCAddr.sin6_addr.s6_addr) <= 0)
        {
		    fprintf(stderr, "Bad IPv6 Address ");
            return -1;
	    }
        
        // Convert char array to string
        ipv6_str = std::string(ipv6str);
        printf("SECC IPv6 address: %s\n", ipv6_str.c_str());
    }

    return 0;
}
// -----------------------------------------------------------------------------





// -----------------------------------------------------------------------------
// perform_SDP() - Setup and perform Service Discovery based on device type
// -----------------------------------------------------------------------------
int perform_SDP()
{   
    printf(BOLD_YELLOW "\nNow performing SDP .. \n\n" RESET);

    // Buffer to hold received UDP messages
    // char buffer[1024];

    // Set UDP server and client based on device type
    if (config.device_type == "EVSE")
    {
        printf("Waiting for SDP request message from client..\n");
        std::string udp_client_IP = "";
        int client_port;

        // Wait here until a UDP message is received
        if (secc_udp.receive(v2gtp_inStream, sizeof(v2gtp_inStream), &udp_client_IP, &client_port)) 
        {
            printf("Message received on server from %s on port %d\n", udp_client_IP.c_str(), client_port);
            
            // Parse message to make sure it is correct
            if (parse_SDP_message() < 0)
            {
                printf("Parsing SDP request message failed\n");
                return -1;
            }

            // Create a sender to transmit messages to client
            if (!secc_udp.create_sender(client_port, udp_client_IP, AF_INET6, network_interface, SDP_port, 10000))
            {
                fprintf(stderr, "Can't create sender on UDP port %i\n", client_port);
                exit(0);
            }

            // Create the SDP response packet and send to client
            create_SDP_response(v2gtp_outStream);
            printf("Sending response to client %s..\n", udp_client_IP.c_str());
            secc_udp.send(v2gtp_outStream, SDP_RESPONSE_LENGTH);
        }
    }

    else if (config.device_type == "EV")
    {
        // Create the sender to broadcast messages
        if (!evcc_udp.create_sender(SDP_port, "FF02::1", AF_INET6, network_interface, 0, 10000))
        {
            fprintf(stderr, "Can't create sender on UDP port %i\n", SDP_port);
            exit(0);
        }

        // Create the SDP request packet to send to server
        create_SDP_request(v2gtp_outStream, No_Transport_Layer_Security, SDP_TCP);
        
        printf("Sending SDP request to server.. \n");
        evcc_udp.send(v2gtp_outStream, SDP_REQUEST_LENGTH);
        int e_port = get_ephemeral_port(evcc_udp.get_sd());
        printf("Message sent over port %d\n", e_port);

        // Create a listener to listen to responses
        if (!evcc_udp.create_server(e_port, "::", AF_INET6, 10000))
        {
            fprintf(stderr, "Can't create listener on UDP port %i\n", e_port);
            exit(0);
        }

        printf("Waiting for response from server..\n");
        std::string udp_server_IP = "";

        if (evcc_udp.receive(v2gtp_inStream, sizeof(v2gtp_inStream), &udp_server_IP)) 
        {
            printf("Message received on client from IP %s\n", udp_server_IP.c_str());
        }

        // Parse message to make sure it is correct
        if (parse_SDP_message() < 0)
        {
            printf(BOLD_RED "Parsing SDP response message failed\n" RESET);
            return -1;
        }

        printf("SDP successful!\n");
    }
    
    // If we get here, SDP successful
    return 0;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// get_ephemeral_port() - This function returns back the ephemeral port assigned by the OS
//                        when using sendto() to send data over the specified socket
// -----------------------------------------------------------------------------
int get_ephemeral_port(int sd)
{
    // Define a struct for the address of the socket (IPv6)
    struct sockaddr_in6 my_addr;

    // Zero out the memory for the struct to avoid any garbage values
    memset(&my_addr, 0, sizeof(my_addr));

    // Define a variable to hold the length of the address struct
    socklen_t len = sizeof(my_addr);

    // Retrieve the local address and port assigned to the socket identified by sd
    if (getsockname(sd, (struct sockaddr *) &my_addr, &len) < 0)
    {
        perror("getsockname failed");
        return 0;
    }

    // Convert the port number from network byte order to host byte order and return
    return (ntohs(my_addr.sin6_port));
}
// -----------------------------------------------------------------------------
