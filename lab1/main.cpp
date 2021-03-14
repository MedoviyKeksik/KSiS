#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>

LANA_ENUM getAdapterList() {
    LANA_ENUM res;
    NCB ncb;
    ZeroMemory(&ncb, sizeof(ncb));
    ncb.ncb_command = NCBENUM;
    ncb.ncb_buffer = (PUCHAR)&res;
    ncb.ncb_length = sizeof(res);
    Netbios(&ncb);
    return res;
}

BOOL resetAdapter(UCHAR AdapterNumber) {
    NCB ncb;
    ZeroMemory(&ncb, sizeof(ncb));
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lana_num = AdapterNumber;
    if (Netbios(&ncb) == NRC_GOODRET) {
        return TRUE;
    }
    return FALSE;
}

PSTR getAdapterMacAddress(UCHAR adapterNumber) {
    PCHAR buff = (PCHAR)malloc(20);
    if (resetAdapter(adapterNumber)) {
        NCB ncb;
        ZeroMemory(&ncb, sizeof(ncb));
        ncb.ncb_command = NCBASTAT;
        ncb.ncb_lana_num = adapterNumber;
        ADAPTER_STATUS adapterStatus;
        ncb.ncb_buffer = (PUCHAR)&adapterStatus;
        ncb.ncb_length = sizeof(adapterStatus);
        strcpy((char *)ncb.ncb_callname, "*");

        if (NRC_GOODRET == Netbios(&ncb)) {
            PSTR buffer = (PCHAR)malloc(18);
            sprintf(buffer, "%02X-%02X-%02X-%02X-%02X-%02X",
                    adapterStatus.adapter_address[0],
                    adapterStatus.adapter_address[1],
                    adapterStatus.adapter_address[2],
                    adapterStatus.adapter_address[3],
                    adapterStatus.adapter_address[4],
                    adapterStatus.adapter_address[5]);
            return buffer;
        }
    }
    return NULL;
}

void printMacAddresses() {
    LANA_ENUM adapters = getAdapterList();
    PSTR tmp = NULL;
    for (UCHAR i = 0; i < adapters.length; i++) {
        tmp = getAdapterMacAddress(adapters.lana[i]);
        if (tmp) {
            std::string now(tmp);
            std::cout << (int)i << ": " << tmp << '\n';
        } else {
            std::cerr << (int)i << ": ERROR\n";
        }
    }
}

void DisplayStruct(int i, LPNETRESOURCE lpnrLocal) {
    printf("NETRESOURCE[%d] Scope: ", i);
    switch (lpnrLocal->dwScope) {
        case (RESOURCE_CONNECTED):
            printf("connected\n");
            break;
        case (RESOURCE_GLOBALNET):
            printf("all resources\n");
            break;
        case (RESOURCE_REMEMBERED):
            printf("remembered\n");
            break;
        default:
            printf("unknown scope %d\n", lpnrLocal->dwScope);
            break;
    }
    printf("NETRESOURCE[%d] Type: ", i);
    switch (lpnrLocal->dwType) {
        case (RESOURCETYPE_ANY):
            printf("any\n");
            break;
        case (RESOURCETYPE_DISK):
            printf("disk\n");
            break;
        case (RESOURCETYPE_PRINT):
            printf("print\n");
            break;
        default:
            printf("unknown type %d\n", lpnrLocal->dwType);
            break;
    }
    printf("NETRESOURCE[%d] DisplayType: ", i);
    switch (lpnrLocal->dwDisplayType)
    {
        case (RESOURCEDISPLAYTYPE_GENERIC):
            printf("generic\n");
            break;
        case (RESOURCEDISPLAYTYPE_DOMAIN):
            printf("domain\n");
            break;
        case (RESOURCEDISPLAYTYPE_SERVER):
            printf("server\n");
            break;
        case (RESOURCEDISPLAYTYPE_SHARE):
            printf("share\n");
            break;
        case (RESOURCEDISPLAYTYPE_FILE):
            printf("file\n");
            break;
        case (RESOURCEDISPLAYTYPE_GROUP):
            printf("group\n");
            break;
        case (RESOURCEDISPLAYTYPE_NETWORK):
            printf("network\n");
            break;
        default:
            printf("unknown display type %d\n", lpnrLocal->dwDisplayType);
            break;
    }
    printf("NETRESOURCE[%d] Usage: 0x%x = ", i, lpnrLocal->dwUsage);
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONNECTABLE)
    {
        printf("connectable ");
    }
    if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONTAINER)
    {
        printf("container ");
    }
    printf("\n");
    printf("NETRESOURCE[%d] Localname: %S\n", i, lpnrLocal->lpLocalName);
    printf("NETRESOURCE[%d] Remotename: %S\n", i, lpnrLocal->lpRemoteName);
    printf("NETRESOURCE[%d] Comment: %S\n", i, lpnrLocal->lpComment);
    printf("NETRESOURCE[%d] Provider: %S\n", i, lpnrLocal->lpProvider);
    printf("\n");
}

BOOL WINAPI EnumerateFunc(LPNETRESOURCE lpnr) {
    DWORD dwResult, dwResultEnum;
    HANDLE hEnum;
    DWORD cbBuffer = 16384;
    DWORD cEntries = -1;
    LPNETRESOURCE lpnrLocal;
    DWORD i;

    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET,
                            RESOURCETYPE_ANY,
                            0,
                            lpnr,
                            &hEnum);

    if (dwResult != NO_ERROR) {
        printf("WnetOpenEnum failed with error %d\n", dwResult);
        return FALSE;
    }
    lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL) {
        printf("WnetOpenEnum failed with error %d\n", dwResult);
        return FALSE;
    }
    do {
        ZeroMemory(lpnrLocal, cbBuffer);
        dwResultEnum = WNetEnumResource(hEnum,
                                        &cEntries,
                                        lpnrLocal,
                                        &cbBuffer);

        if (dwResultEnum == NO_ERROR) {
            for (i = 0; i < cEntries; i++) {
                DisplayStruct(i, &lpnrLocal[i]);
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER)) {
                    if (!EnumerateFunc(&lpnrLocal[i])) {
                        printf("EnumerateFunc returned FALSE\n");
                    }
                }
            }
        } else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
            printf("WNetEnumResource failed with error %d\n", dwResultEnum);
            break;
        }
    } while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    GlobalFree((HGLOBAL)lpnrLocal);
    dwResult = WNetCloseEnum(hEnum);

    if (dwResult != NO_ERROR) {
        printf("WNetCloseEnum failed with error %d\n", dwResult);
        return FALSE;
    }
    return TRUE;
}

int main() {
    std::cout << "MAC Addresses:\n";
    printMacAddresses();

    std::cout << "\nNetResources:\n";
    LPNETRESOURCE lpnr = NULL;
    if (EnumerateFunc(lpnr) == FALSE) {
        std::cout << "Call to EnumerateFunc failed" << std::endl;
    }
    return 0;
}