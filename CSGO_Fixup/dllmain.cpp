// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "SDK/bitbuf.h"
#include "SDK/networkstringtable.h"
#include "Thirdparty/MemoryTools/MemoryTools.h"
#include "Thirdparty/MinHook/MinHook.h"
#include "Thirdparty/MinHook/hde/hde32.h"
#include <sstream>
#include <iomanip>
#include <Psapi.h>
#include <exception>




typedef void(__thiscall* DeleteAllStringsFunc_t)(INetworkStringTable*);
typedef int (__thiscall* CNetworkStringTable_AddStringFunc_t)(
    INetworkStringTable* string_table,
    bool bIsServer,
    const char* string,
    int length /* = -1*/,
    const void* userdata /*= NULL*/);

#if 1


int CNetworkStringTable_DataChanged(INetworkStringTable* _this, int stringNumber, INetworkStringTableItem* item)
{
    typedef int(__thiscall* OnDataChangedFunc_t)(INetworkStringTable* _this, int stringNumber, INetworkStringTableItem* item);
    static OnDataChangedFunc_t _OnDataChanged = (OnDataChangedFunc_t)MemoryTools::PatternScanModule("engine.dll", "55 8B EC 8B 45 0C 57 8B F9 85 C0 74 4E 8B 4F 18 8B 57 14");
    return _OnDataChanged(_this, stringNumber, item);
    
#if 0
    Assert(item);
    _this->m_nLastChangedTick = _this->m_nTickCount;
    if (_this->m_changeFunc != NULL)
    {
        int userDataSize;
        const void* pUserData = item->GetUserData(&userDataSize);
        (*_this->m_changeFunc)(_this->m_pObject, _this, stringNumber, _this->GetString(stringNumber), pUserData);
    }
    return 0;
#endif
}
#define SHARED_NET_STRING_TABLES
int Rebuilt_CNetworkStringTable_AddString(INetworkStringTable* _this, bool bIsServer, const char* string, int length /*= -1*/, const void* userdata /*= NULL*/)
{
    typedef bool(__thiscall* SetUserDataFunc_t)(void* _this, int tick, int length, const void* userData);
    static SetUserDataFunc_t SetUserData = (SetUserDataFunc_t)MemoryTools::PatternScanModule("engine.dll", "55 8B EC 56 8B F1 83 7E 10 00 74 13 FF 75 10 FF 75 0C FF 75 08 E8 ? ? ? ? 5E");
    
    typedef void(__thiscall* SetChangeHistoryFunc_t)(INetworkStringTableItem* _this, bool bEnableOrDisable);
    static SetChangeHistoryFunc_t SetChangeHistory = (SetChangeHistoryFunc_t)MemoryTools::PatternScanModule("engine.dll", "55 8B EC 56 8B F1 8B 56 10 85 D2 0F 84 ? ? ? ? 80 7D 08 00");

    bool bHasChanged;
    INetworkStringTableItem* item;

    if (!string)
    {
        Assert(string);
        printf("Warning:  Can't add NULL string to table %s\n", _this->GetTableName());
        return INVALID_STRING_INDEX;
    }

#if 0
    if (_this->m_bLocked)
    {
        printf("Warning! CNetworkStringTable::AddString: adding '%s' while locked.\n", string);
    }
#endif

    int i = _this->m_pItems->Find(string);
    if (!bIsServer)
    {
        if (_this->m_pItems->IsValidIndex(i) && !_this->m_pItemsClientSide)
        {
            bIsServer = true;
        }
        else if (!_this->m_pItemsClientSide)
        {
            // NOTE:  YWB 9/11/2008
            // This case is probably a bug the since the server "owns" the state of the string table and if the client adds 
            // some extra value in and then the server comes along and uses that slot, then all hell breaks loose (probably).  
            // SetAllowClientSideAddString was added to allow for client side only precaching to be in a separate chunk of the table -- it should be used in this case.
            // TODO:  Make this a Sys_Error?
            printf("CNetworkStringTable::AddString:  client added string which server didn't put into table (consider SetAllowClientSideAddString?): %s %s\n", _this->GetTableName(), string);
        }
    }

    if (!bIsServer && _this->m_pItemsClientSide)
    {
        i = _this->m_pItemsClientSide->Find(string);

        if (!_this->m_pItemsClientSide->IsValidIndex(i))
        {
            // not in list yet, create it now
            if (_this->m_pItemsClientSide->Count() >= (unsigned int)_this->GetMaxStrings())
            {
                // Too many strings, FIXME: Print warning message
                printf("Warning:  Table %s is full, can't add %s\n", _this->GetTableName(), string);
                return INVALID_STRING_INDEX;
            }

            // create new item
            {
                //MEM_ALLOC_CREDIT();
                i = _this->m_pItemsClientSide->Insert(string);
            }

            item = &_this->m_pItemsClientSide->Element(i);

            // set changed ticks

            item->m_nTickChanged = _this->m_nTickCount;

#ifndef	SHARED_NET_STRING_TABLES
            //item->m_nTickCreated = _this->m_nTickCount;

            if ((*(BYTE*)(_this + 32) & 1) != 0)
            {
                auto v20 = *(DWORD*)(_this + 24);
                if (v20 != *(DWORD*)(_this + 20))
                {
                    *(DWORD*)(_this + 28) = v20;
                    *(DWORD*)(_this + 24) = *(DWORD*)(_this + 20);
                    return -i;
                //item->EnableChangeHistory();
                SetChangeHistory(item, true);
                //__debugbreak();
                //item->EnableChangeHistory();
            }
#endif

            bHasChanged = true;
        }
        else
        {
            item = &_this->m_pItemsClientSide->Element(i); // item already exists
            bHasChanged = false;  // not changed yet
        }

        if (length > -1)
        {
            if (SetUserData(item, *(DWORD*)(_this + 0x20), length, userdata))
            {
                bHasChanged = true;
            }
        }

        if (bHasChanged && !_this->m_bChangeHistoryEnabled)
        {
            CNetworkStringTable_DataChanged(_this, -i, item);
        }

        // Negate i for returning to client
        i = -i;
    }
    else
    {
        // See if it's already there
        if (!_this->m_pItems->IsValidIndex(i))
        {
            // not in list yet, create it now
            if (_this->m_pItems->Count() >= (unsigned int)_this->GetMaxStrings())
            {
                // Too many strings, FIXME: Print warning message
                printf("Warning:  Table %s is full, can't add %s\n", _this->GetTableName(), string);
                return INVALID_STRING_INDEX;
            }

            // create new item
            {
                //MEM_ALLOC_CREDIT();
                i = _this->m_pItems->Insert(string);
            }

            item = &_this->m_pItems->Element(i);

            // set changed ticks
            item->m_nTickChanged = _this->m_nTickCount;

#ifndef	SHARED_NET_STRING_TABLES
            //item->m_nTickCreated = _this->m_nTickCount;

            if (_this->m_bChangeHistoryEnabled)
            {
                SetChangeHistory(item, true);
            }
#endif

            bHasChanged = true;
        }
        else
        {
            item = &_this->m_pItems->Element(i); // item already exists
            bHasChanged = false;  // not changed yet
        }

        if (length > -1)
        {
            if (SetUserData(item, _this->m_nTickCount, length, userdata))
            {
                bHasChanged = true;
            }
        }


        if (bHasChanged && !_this->m_bChangeHistoryEnabled)
        {
            CNetworkStringTable_DataChanged(_this, i, item);
        }
    }
   
    return i;
}
#endif
bool Rebuilt_ReadStringTable(INetworkStringTable* _this, bf_read& buf)
{
    static DeleteAllStringsFunc_t DeleteAllStrings = (DeleteAllStringsFunc_t)MemoryTools::PatternScanModule("engine.dll", "56 8B F1 57 8B 4E 40 85 C9 74 06 8B 01 6A 01 FF 10 A1 ?? ?? ?? ?? 6A 24"); 
    static CNetworkStringTable_AddStringFunc_t AddString = (CNetworkStringTable_AddStringFunc_t)MemoryTools::PatternScanModule("engine.dll", "55 8B EC 53 8B 5D 0C 57 8B F9 85 DB 75 1F");
         
    DeleteAllStrings(_this);

    int numstrings = buf.ReadWord();
    for (int i = 0; i < numstrings; i++)
    {
        char stringname[4096];

        buf.ReadString(stringname, sizeof(stringname));
#ifdef _DEBUG
        if(strstr(_this->m_pszTableName, "GameRulesCreation"))
            printf(" -> Reading GameRulesCreation Table Item %s\n", stringname);
#endif
        Assert(strlen(stringname) < 100);

        if (buf.ReadOneBit() == 1)
        {
            int userDataSize = (int)buf.ReadWord();
            Assert(userDataSize > 0);
            byte* data = new byte[userDataSize + 4];
            Assert(data);

            buf.ReadBytes(data, userDataSize);

            if (strstr(_this->m_pszTableName, "GameRulesCreation"))
                __debugbreak();

            AddString(_this, true, stringname, userDataSize, data);
            //Rebuilt_CNetworkStringTable_AddString(_this, true, stringname, userDataSize, data);

            delete[] data;

            Assert(buf.GetNumBytesLeft() > 10);

        }
        else
        {
                //if (strstr(_this->m_pszTableName, "GameRulesCreation"))
                //    __debugbreak();

                //Rebuilt_CNetworkStringTable_AddString(_this, true, stringname, -1, NULL);
                AddString(_this, true, stringname, -1, NULL); // Here!

        }
    }

    // Client side stuff
    if (buf.ReadOneBit() == 1)
    {
        int numstrings = buf.ReadWord();
        for (int i = 0; i < numstrings; i++)
        {
            char stringname[4096];

            buf.ReadString(stringname, sizeof(stringname));

            if (buf.ReadOneBit() == 1)
            {
                int userDataSize = (int)buf.ReadWord();
                Assert(userDataSize > 0);
                byte* data = new byte[userDataSize + 4];
                Assert(data);

                buf.ReadBytes(data, userDataSize);

                if (i >= 2)
                {
                    AddString(_this, false, stringname, userDataSize, data);
                    //Rebuilt_CNetworkStringTable_AddString(_this, false, stringname, userDataSize, data);
                }

                delete[] data;

            }
            else
            {
                if (i >= 2)
                {
                    //Rebuilt_CNetworkStringTable_AddString(_this, false, stringname, -1, NULL);
                    AddString(_this, false, stringname, -1, NULL);
                }
            }
        }
    }

    return true;
}





bool __fastcall ReadStringTable(INetworkStringTable* _this, void* edx, bf_read* buf);
bool __fastcall hk_ReadStringTables(CNetworkStringTableContainer* _this, void* edx, bf_read* buf)
{
    typedef char(__thiscall* SetAllowClientSideAddStringFunc_t)(INetworkStringTable* _this, bool bState);
    static SetAllowClientSideAddStringFunc_t SetAllowClientSideAddString = (SetAllowClientSideAddStringFunc_t)MemoryTools::PatternScanModule("engine.dll", "55 8B EC 8A 55 08 56 8B F1 8A 4E 20 8A C1 C0 E8 02 24 01 3A D0");

    static bool bExit = false;
    if (bExit) {  bExit = false; return true; }
        
    printf("Reading String Tables (%d Bytes)....\n ", buf->m_nDataBytes);

    int numTables = buf->ReadByte();
    for (int i = 0; i < numTables; i++)
    {
        char tablename[256];
        buf->ReadString(tablename, sizeof(tablename));

#ifdef _DEBUG
        printf("Reading NetworkingStringTable : %s\n", tablename);
#endif

        // Find this table by name
        INetworkStringTable* table = (INetworkStringTable*)(*(int(__thiscall**)(CNetworkStringTableContainer*, char*))((*(DWORD*)_this) + 12))(_this, tablename);
            
        Assert(table);
        // Now read the data for the table
        if (!ReadStringTable(table, edx, buf))
        {
            printf("Error reading string table %s\n", tablename);
            bExit = true;
            return false;
        }
    }
    return true;
}




void* pOriginalReadStringTable = nullptr;
bool __fastcall ReadStringTable(INetworkStringTable* _this, void* edx, bf_read* buf)
{
    auto pReturnAddress = _ReturnAddress();
    try {
        return Rebuilt_ReadStringTable(_this, std::ref(*buf));
        //return reinterpret_cast<decltype(&ReadStringTable)>(pOriginalReadStringTable)(_this, edx, buf);
    }
    catch (std::exception& e)
    {
        // Catch Error and hook issue function. It seems valve's compiler duplicated ReadStringTable(s) multiple times
        // So in Order to Properly Find it, rather than sig it, its better to get the return address here and hook it
        // by searching for the function prologue. 
        printf("Tripped Exception When Reading bf_read %s\n", buf->m_pDebugName);

        static bool bInited = [&]()->bool {
            auto pReadStringTablesAddress = MemoryTools::FindFunctionPrologueFromReturnAddressx86(pReturnAddress);
            void* pDiscardOriginal = 0; // Rebuilt the function, so it doesn't matter....
            if (MH_CreateHook(pReadStringTablesAddress, &hk_ReadStringTables, &pDiscardOriginal) == MH_OK)
                printf("Hooked CNetworkStringTableContainer::ReadTables at %p\n", pReadStringTablesAddress);
            else
                __debugbreak();

            MH_EnableHook(MH_ALL_HOOKS);
            return true;
        }();



        return false; // Return false, trip the Host_Error and don't crash the game!
    }
}

void* pCrashHereOriginal = nullptr;
int __fastcall hk_CrashHere(unsigned __int8* is_nullptr, unsigned __int8* a2)
{ // Q_stricmp (where it would normally crash!
    return reinterpret_cast<decltype(&hk_CrashHere)>(pCrashHereOriginal)(is_nullptr, a2);
}

void* pOriginalOnGameRulesCreationStringChanged = nullptr;
void __cdecl hk_OnGameRulesCreationStringChanged(
    void* object,
    INetworkStringTable* stringTable,
    int stringNumber,
    const char* newString,
    void const* newData)
{
    printf("  -------> Reading : %s in Q_strcmpi, %s in newString\n", (const char*)newData, newString);
#if 0
    printf("OnGameRulesCreationStringChanged(%p, %p, %d, %s, %s)\n", object, stringTable, stringNumber, newString, newData);
#endif

    if (!newData)
    {
        // This is called by OnDataChanged and references the nullptr that is set to the
        // newData value, just throw an exception up to ReadStringTable so we can handle it 
        // (hook function, trip Host_Error). After it is tripped we can load the demo normally!
        throw std::exception("bad read!\n");
        throw std::exception("bad read!\n");
    }
    
    reinterpret_cast<decltype(&hk_OnGameRulesCreationStringChanged)>
        (pOriginalOnGameRulesCreationStringChanged)
        (object, stringTable, stringNumber, newString, newData);
}


char (__cdecl* AppendModuleInfoToUserExtraData)(const char* a1, int a2);
char (__cdecl* CheckIfCallIsFromCheat)(LPCSTR lpModuleName, int a2, int a3);


void InitializeFixup()
{
    // Give us a Console To Work With
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);

    MH_Initialize();


    //CheckIfCallIsFromCheat = (decltype(CheckIfCallIsFromCheat))MemoryTools::PatternScanModule("engine.dll", "55 8B EC A1 ? ? ? ? 85 C0 75 28 A1 ? ? ? ? 68 ? ? ? ? 8B 08");
    //if (!CheckIfCallIsFromCheat)
    //    __debugbreak();

    void* pFunc = MemoryTools::PatternScanModule("client.dll", "56 57 8B FA 8B F1 3B F7 75 06 5F 33 C0 5E C3 90");
    if (!pFunc || MH_OK != MH_CreateHook(pFunc, &hk_CrashHere, (void**)&pCrashHereOriginal))
        __debugbreak();

    void* pReadStringTableAddr = MemoryTools::PatternScanModule("engine.dll", "55 8B EC B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 53 56 57 8B F9 89 7D F8");
    if (!pReadStringTableAddr || MH_OK != MH_CreateHook(pReadStringTableAddr, &ReadStringTable, (void**)&pOriginalReadStringTable))
        __debugbreak();

    void* pOnGameRulesCreationStringChangedAddr = MemoryTools::PatternScanModule("client.dll", "55 8B EC 8B 0D ?? ?? ?? ?? 85 C9 74 07 8B 01 6A 01 FF ?? ?? 56");
    if (!pOnGameRulesCreationStringChangedAddr || MH_OK != MH_CreateHook(pOnGameRulesCreationStringChangedAddr, &hk_OnGameRulesCreationStringChanged, (void**)&pOriginalOnGameRulesCreationStringChanged))
        __debugbreak();

    MH_EnableHook(MH_ALL_HOOKS);

    printf("CSGO Demo Fixup Initialized!\n");

#if 0
    FARPROC bSecureAllowed = GetProcAddress(GetModuleHandleA("csgo.exe"), "BSecureAllowed");
    typedef int(__stdcall* bSecureAllowedFunc_t)(const char* szStr, size_t szSize, bool bFullReport);

    for (int i = 0; i < 9999; i++)
    {
        char buf[81920];
        char buf2[81920];
    
        ((bSecureAllowedFunc_t)bSecureAllowed)(buf2, ARRAYSIZE(buf2), true);

        if (strcmp(buf, buf2))
        {
            if (strcmp(buf, buf2))
            {
                printf("Difference Found!\n");
                printf("%s", buf2);
                printf("Difference Found!\n");
            }


            ((bSecureAllowedFunc_t)bSecureAllowed)(buf, ARRAYSIZE(buf), true);
            MessageBoxA(NULL, "Dif Found!", "Dif Found", MB_OK);

        }

        Sleep(2000);
    }
#endif

#ifdef _DEBUG
    dump_offsets();
#endif

}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitializeFixup();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:


        break;
    }
    return TRUE;
}

