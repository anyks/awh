// g++ --std=c++11 arp.cpp -o ./arp -lws2_32 -lIphlpapi -lstdc++

#include <Winsock2.h>
#include <iphlpapi.h>
#include <cstdio>

//#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "ws2_32.lib")

bool get_name(unsigned char* name, char dest[32])
{
    struct in_addr destip;
    struct hostent* info;

    destip.s_addr = inet_addr(dest);

    info = gethostbyaddr((char*)&destip, 4, AF_INET);

    if (info != NULL)
    {
        strcpy((char*)name, info->h_name);
    }
    else
    {
        return false;
    }

    return true;
}

bool get_mac(unsigned char* mac , char dest[32])
{
    struct in_addr destip;
    ULONG mac_address[2];
    ULONG mac_address_len = 6;

    destip.s_addr = inet_addr(dest);

    SendARP((IPAddr)destip.S_un.S_addr, 0, mac_address, &mac_address_len);

    if (mac_address_len)
    {
        BYTE* mac_address_buffer = (BYTE*)&mac_address;
        for (int i = 0; i < (int)mac_address_len; i++)
        {
            mac[i] = (char)mac_address_buffer[i];
        }
    }
    else
    {
        return false;
    }

    return true;
}

int main()
{
    char address[][32] = {{"192.168.1.1"}, {"192.168.1.2"}, {"192.168.1.3"}, {"192.168.1.4"}};
    WSADATA sock;

    if (WSAStartup(MAKEWORD(2,2), &sock) != 0)
    {
        printf("Failed to initialise winsock. (%d)\n", WSAGetLastError());
        return 1;
    }

    for (int i = 0; i < (int)sizeof(address)/32; i++)
    {
        unsigned char mac[6] = {'\0'};
        unsigned char name[100] = {'\0'};

        if (get_mac(mac, address[i]))
        {
            printf("%s : %s : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", address[i], (get_name(name, address[i])) ? (char*)name : "-", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            fflush(stdout);
        }
    }

    printf("\nDone.\n");
    fflush(stdout);

    return 0;
}
