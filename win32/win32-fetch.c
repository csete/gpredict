// Fetch URL content to a specified handle using Wininet as no libcurl
// dev libs available for Win32/64.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include "win32-fetch.h"

int win32_fetch(char *url, FILE *file, char *proxy, char *ua)
{
    HINTERNET hInt;
    if (proxy != NULL)
        hInt = InternetOpen(ua, INTERNET_OPEN_TYPE_PROXY, proxy, "<local>", 0);
    else
        hInt = InternetOpen(ua, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (NULL == hInt)
        return 0x80000000 | (int)GetLastError();

    HINTERNET hUrl = InternetOpenUrl(hInt, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (NULL == hUrl)
    {
        InternetCloseHandle(hInt);
        return 0x40000000 | (int)GetLastError();
    }

    char buf[8192];
    DWORD nRead;
    while (InternetReadFile(hUrl, buf, 8192, &nRead))
    {
        if (nRead > 0)
            fwrite(buf, nRead, 1, file);
        else
            break;
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInt);
    return 0;
}

