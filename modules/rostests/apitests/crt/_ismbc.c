/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for _ismbc* functions
 * COPYRIGHT:   Copyright 2025 Doug Lyons <douglyons@douglyons.com>
 *              Copyright 2025 Stanislav Motylkov <x86corez@gmail.com>
 */

#include <apitest.h>
#include <mbctype.h>
#include <mbstring.h>
#define WIN32_NO_STATUS

typedef int (__cdecl *PFN_chk)(unsigned int c, int cjk);
typedef int (__cdecl *PFN__ismbc)(unsigned int c);
static USHORT ver;

#ifndef TEST_STATIC_CRT
#ifndef TEST_CRTDLL
static PFN__ismbc p__ismbcalnum;
static PFN__ismbc p__ismbcgraph;
static PFN__ismbc p__ismbcpunct;
#endif
static PFN__ismbc p__ismbcalpha;
static PFN__ismbc p__ismbcdigit;
static PFN__ismbc p__ismbcprint;
static PFN__ismbc p__ismbcsymbol;
static PFN__ismbc p__ismbcspace;

static BOOL Init()
{
    int fails = 0;
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);

#ifndef TEST_CRTDLL
    p__ismbcalnum = (PFN__ismbc)GetProcAddress(hdll, "_ismbcalnum");
    ok(p__ismbcalnum != NULL, "Failed to load _ismbcalnum from %s\n", TEST_DLL_NAME);
    if (p__ismbcalnum == NULL) fails++;

    p__ismbcgraph = (PFN__ismbc)GetProcAddress(hdll, "_ismbcgraph");
    ok(p__ismbcgraph != NULL, "Failed to load _ismbcgraph from %s\n", TEST_DLL_NAME);
    if (p__ismbcgraph == NULL) fails++;

    p__ismbcpunct = (PFN__ismbc)GetProcAddress(hdll, "_ismbcpunct");
    ok(p__ismbcpunct != NULL, "Failed to load _ismbcpunct from %s\n", TEST_DLL_NAME);
    if (p__ismbcpunct == NULL) fails++;
#endif

    p__ismbcalpha = (PFN__ismbc)GetProcAddress(hdll, "_ismbcalpha");
    ok(p__ismbcalpha != NULL, "Failed to load _ismbcalpha from %s\n", TEST_DLL_NAME);
    if (p__ismbcalpha == NULL) fails++;

    p__ismbcdigit = (PFN__ismbc)GetProcAddress(hdll, "_ismbcdigit");
    ok(p__ismbcdigit != NULL, "Failed to load _ismbcdigit from %s\n", TEST_DLL_NAME);
    if (p__ismbcdigit == NULL) fails++;

    p__ismbcprint = (PFN__ismbc)GetProcAddress(hdll, "_ismbcprint");
    ok(p__ismbcprint != NULL, "Failed to load _ismbcprint from %s\n", TEST_DLL_NAME);
    if (p__ismbcprint == NULL) fails++;

    p__ismbcsymbol = (PFN__ismbc)GetProcAddress(hdll, "_ismbcsymbol");
    ok(p__ismbcsymbol != NULL, "Failed to load _ismbcsymbol from %s\n", TEST_DLL_NAME);
    if (p__ismbcsymbol == NULL) fails++;

    p__ismbcspace = (PFN__ismbc)GetProcAddress(hdll, "_ismbcspace");
    ok(p__ismbcspace != NULL, "Failed to load _ismbcspace from %s\n", TEST_DLL_NAME);
    if (p__ismbcspace == NULL) fails++;

    return (fails == 0);
}
#define _ismbcalnum p__ismbcalnum
#define _ismbcalpha p__ismbcalpha
#define _ismbcdigit p__ismbcdigit
#define _ismbcprint p__ismbcprint
#define _ismbcsymbol p__ismbcsymbol
#define _ismbcspace p__ismbcspace
#define _ismbcgraph p__ismbcgraph
#define _ismbcpunct p__ismbcpunct
#endif

static USHORT GetWinVersion(VOID)
{
    return ((GetVersion() & 0xFF) << 8) |
           ((GetVersion() >> 8) & 0xFF);
}

int CHK_ismbcalpha(unsigned int c, int cjk)
{
    int ret = 0;
    if (!cjk)
        return ((c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a));

    ret = CHK_ismbcalpha(c, 0) || (c >= 0xa6 && c <= 0xdf);
    if (ver >= 0x602)
        ret |= (
            (c >= 0x8152 && c <= 0x8155) || (c >= 0x8157 && c <= 0x815b) || c == 0x81f0 ||
            (c >= 0x8260 && c <= 0x8279) || (c >= 0x8281 && c <= 0x829a) || (c >= 0x829f && c <= 0x82f1) ||
            (c >= 0x8340 && c <= 0x837e) || (c >= 0x8380 && c <= 0x8396) || (c >= 0x839f && c <= 0x83b6) || (c >= 0x83bf && c <= 0x83d6) ||
            (c >= 0x8440 && c <= 0x8460) || (c >= 0x8470 && c <= 0x847e) || (c >= 0x8480 && c <= 0x8491) ||
            (c >= 0x8754 && c <= 0x875d) ||
            (c >= 0x889f && c <= 0x88fc) ||
            (((c >> 8) >= 0x89 && (c >> 8) <= 0x97) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (((c >> 8) == 0x98) && (((c & 255) >= 0x40 && (c & 255) <= 0x72) || ((c & 255) >= 0x9f && (c & 255) <= 0xfc))) ||
            (((c >> 8) >= 0x99 && (c >> 8) <= 0x9f) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (((c >> 8) >= 0xe0 && (c >> 8) <= 0xe9) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (((c >> 8) == 0xea) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xa4))) ||
            (((c >> 8) == 0xed) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (((c >> 8) == 0xee) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xec) || ((c & 255) >= 0xef && (c & 255) <= 0xf8))) ||
            (((c >> 8) == 0xfa) && (((c & 255) >= 0x40 && (c & 255) <= 0x53) || ((c & 255) >= 0x5c && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (((c >> 8) == 0xfb) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
            (c >= 0xfc40 && c <= 0xfc4b));
    return ret;
}

int CHK_ismbcdigit(unsigned int c, int cjk)
{
    if (!cjk || ver < 0x602)
        return (c >= 0x30 && c <= 0x39);
    else
        return (CHK_ismbcdigit(c, 0) || (c >= 0x824f && c <= 0x8258));
}

int CHK_ismbcalnum(unsigned int c, int cjk)
{
    return CHK_ismbcalpha(c, cjk) || CHK_ismbcdigit(c, cjk);
}

int CHK_ismbcsymbol(unsigned int c, int cjk)
{
    if (!cjk)
#ifndef TEST_CRTDLL
        return 0;
#else
        return CHK_ismbcsymbol(c, 1);
#endif
    else
        return (c >= 0x8141 && c <= 0x817e) || (c >= 0x8180 && c <= 0x81ac);
}

int CHK_ismbcspace(unsigned int c, int cjk)
{
    if (!cjk || ver < 0x602)
        return (c == 0x20 || (c >= 0x09 && c <= 0x0D));
    else
        return (CHK_ismbcspace(c, 0) || c == 0x8140);
}

int CHK_ismbcpunct(unsigned int c, int cjk)
{
    int ret = 0;
    if (!cjk)
        return (c >= 0x21 && c <= 0x2f) || (c >= 0x3a && c <= 0x40) || (c >= 0x5b && c <= 0x60) || (c >= 0x7b && c <= 0x7e);

    ret = CHK_ismbcpunct(c, 0) || (c >= 0xa1 && c <= 0xa5);
    if (ver >= 0x602)
        ret |= (c >= 0x8141 && c <= 0x8149) || c == 0x814c || c == 0x814e || c == 0x8151 || c == 0x8156 || (c >= 0x815c && c <= 0x815f) || (c >= 0x8163 && c <= 0x817a) || (c >= 0x817c && c <= 0x817e) ||
               c == 0x8180 || (c >= 0x818b && c <= 0x818d) || (c >= 0x8193 && c <= 0x8198) || c == 0x81a6 || c == 0x81f1 || (c >= 0x81f5 && c <= 0x81f7) ||
               (c >= 0x8780 && c <= 0x8781) ||
               (c >= 0xeefb && c <= 0xeefc) ||
               (c >= 0xfa56 && c <= 0xfa57);
    return ret;
}

int CHK_ismbcgraph(unsigned int c, int cjk)
{
    return CHK_ismbcalpha(c, cjk) || CHK_ismbcdigit(c, cjk) || CHK_ismbcpunct(c, cjk);
}

int CHK_ismbcprint(unsigned int c, int cjk)
{
    int ret = 0;
    if (!cjk)
#ifndef TEST_CRTDLL
        return c == 0x20 ||
               CHK_ismbcsymbol(c, cjk) || CHK_ismbcgraph(c, cjk);
#else
    {
        ret |= (c >= 0xa6 && c <= 0xdf) ||
               (((c >> 8) >= 0x81 && (c >> 8) <= 0x9f) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
               (((c >> 8) >= 0xe0 && (c >> 8) <= 0xef) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
               (((c >> 8) >= 0xfa && (c >> 8) <= 0xfc) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
               CHK_ismbcprint(c, 1);
        if (ver < 0x602)
            ret |= (((c >> 8) >= 0xf0 && (c >> 8) <= 0xf9) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc)));
        return ret;
    }
#endif

    ret = c == 0x20 || CHK_ismbcgraph(c, cjk);
    if (ver >= 0x602)
        ret |= c == 0x8140 ||
               (c >= 0x81b8 && c <= 0x81bf) || (c >= 0x81c8 && c <= 0x81ce) || (c >= 0x81da && c <= 0x81e8) || (c >= 0x81f2 && c <= 0x81f7) || c == 0x81fc ||
               (c >= 0x849f && c <= 0x84be) ||
               (c >= 0x8740 && c <= 0x875d) || (c >= 0x875f && c <= 0x8775) || c == 0x877e || (c >= 0x8782 && c <= 0x879c) ||
               (c >= 0xeef9 && c <= 0xeefa) ||
               (((c >> 8) >= 0xf0 && (c >> 8) <= 0xf9) && (((c & 255) >= 0x40 && (c & 255) <= 0x7e) || ((c & 255) >= 0x80 && (c & 255) <= 0xfc))) ||
               (c >= 0xfa54 && c <= 0xfa5b) ||
               CHK_ismbcsymbol(c, cjk);
    return ret;
}

void TEST_ismbc(_In_opt_ PFN__ismbc func, _In_ LPCSTR name, _In_ PFN_chk chk)
{
    int i, res, prev_cp = _getmbcp();

    /* SBCS codepage */
    _setmbcp(1252);

    for (i = 0; i < 0x10000; i++)
    {
        res = func(i);

        if (chk(i, 0))
        {
            ok(res, "%s: Expected the character 0x%x to return TRUE, got FALSE\n", name, i);
        }
        else
        {
            ok(!res, "%s: Expected the character 0x%x to return FALSE, got TRUE\n", name, i);
        }
    }

    /* MBCS (Japanese) codepage */
    _setmbcp(932);

#ifndef TEST_CRTDLL
    /* crtdll doesn't export _getmbcp/_setmbcp, and doesn't seem to handle code pages.
     * setlocale and SetThreadLocale are also being ignored. */
    for (i = 0; i < 0x10000; i++)
    {
        res = func(i);

        if (chk(i, 1))
        {
            ok(res, "%s: Expected the character 0x%x to return TRUE, got FALSE\n", name, i);
        }
        else
        {
            ok(!res, "%s: Expected the character 0x%x to return FALSE, got TRUE\n", name, i);
        }
    }
#endif

    _setmbcp(prev_cp);
}

START_TEST(_ismbc)
{
#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because some functions are not available\n");
        return;
    }
#endif
    ver = GetWinVersion();

#ifndef TEST_CRTDLL
    TEST_ismbc(_ismbcalnum, "_ismbcalnum", CHK_ismbcalnum);
#endif
    TEST_ismbc(_ismbcalpha, "_ismbcalpha", CHK_ismbcalpha);
    TEST_ismbc(_ismbcdigit, "_ismbcdigit", CHK_ismbcdigit);
    TEST_ismbc(_ismbcprint, "_ismbcprint", CHK_ismbcprint);
    TEST_ismbc(_ismbcsymbol, "_ismbcsymbol", CHK_ismbcsymbol);
    TEST_ismbc(_ismbcspace, "_ismbcspace", CHK_ismbcspace);
    // _ismbclegal() - covered by msvcrt:string
    // _ismbcl0() - covered by msvcrt:string
    // _ismbcl1() - covered by msvcrt:string
    // _ismbcl2() - covered by msvcrt:string
#ifndef TEST_CRTDLL
    TEST_ismbc(_ismbcgraph, "_ismbcgraph", CHK_ismbcgraph);
    TEST_ismbc(_ismbcpunct, "_ismbcpunct", CHK_ismbcpunct);
#endif
}
