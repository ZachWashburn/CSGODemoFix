#pragma once
#include "Platform.h"
#include "basetypes.h"
#include "bitbuf.h"
#include <stdio.h>

class INetworkStringDict;
class INetworkStringTableItem
{
public:
#if 1
    virtual void blah() = 0;
    virtual void blah1() = 0;
    virtual void blah2() = 0;
    virtual void blah3() = 0;
    virtual void blah4() = 0;
    virtual void blah5() = 0;
    virtual void blah6() = 0;
    virtual void blah7() = 0;
    virtual void blah8() = 0;
    virtual const void* GetUserData(int* length = 0) = 0;

#if 0
    virtual void			EnableChangeHistory(void) = 0;
    virtual void 			UpdateChangeList(int tick, int length, const void* userData) = 0;
    virtual int				RestoreTick(int tick) = 0;
    inline int		GetTickCreated(void) const { return m_nTickCreated; }
    virtual bool			SetUserData(int tick, int length, const void* userdata) = 0;

    inline int		GetUserDataLength() const { return m_nUserDataLength; }

    // Used by server only
    // void			SetTickCount( int count ) ;
    inline int		GetTickChanged(void) const { return m_nTickChanged; }
#endif
#endif
    unsigned char* m_pUserData;
    int				m_nUserDataLength;
    int				m_nTickChanged;
    //int				m_nTickCreated;
    void* m_pChangeList = nullptr;
};
class INetworkStringTable;

typedef void (*pfnStringChanged)(void* object, INetworkStringTable* stringTable, int stringNumber, char const* newString, void const* newData);
class INetworkStringDict
{
public:
    virtual ~INetworkStringDict() {}

    virtual unsigned int Count() = 0;
    virtual void Purge() = 0;
    virtual const char* String(int index) = 0;
    virtual bool IsValidIndex(int index) = 0;
    virtual int Insert(const char* pString) = 0;
    virtual int Find(const char* pString) = 0;
    virtual void UpdateDictionary(int index) = 0;
    virtual int DictionaryIndex(int index) = 0;
    virtual INetworkStringTableItem& Element(int index) = 0;
    virtual const INetworkStringTableItem& Element(int index) const = 0;
};

class INetworkStringTable
{
public:
    enum ConstEnum_t { MIRROR_TABLE_MAX_COUNT = 2 };

    // Table Info
    virtual void deconstructor() = 0;
    virtual const char* GetTableName(void) const = 0;
    virtual int			GetTableId(void) const = 0;
    virtual int				GetNumStrings(void) const = 0;
    virtual int				GetMaxStrings(void) const = 0;
    virtual int				GetEntryBits(void) const = 0;
    // Networking
    virtual void			SetTick(int tick) = 0;
    virtual bool			ChangedSinceTick(int tick) const = 0;
    // Accessors (length -1 means don't change user data if string already exits)
    virtual int				AddString(bool bIsServer, const char* value, int length = -1, const void* userdata = 0) = 0;
    virtual const char* GetString(int stringNumber) const = 0;
    virtual void			SetStringUserData(int stringNumber, int length, const void* userdata) = 0;
    virtual const void* GetStringUserData(int stringNumber, int* length) const = 0;
    virtual int				FindStringIndex(char const* string) = 0; // returns INVALID_STRING_INDEX if not found
    // Callbacks
    virtual void			SetStringChangedCallback(void* object, pfnStringChanged changeFunc) = 0;


    int					m_id;
    char* m_pszTableName;
    // Must be a power of 2, so encoding can determine # of bits to use based on log2
    int						m_nMaxEntries;
    int						m_nEntryBits;
    int						m_nTickCount;
    int						m_nLastChangedTick;
    char pad[4];

    bool					m_bChangeHistoryEnabled;// : 1;
    bool					m_bLocked;// : 1;
    bool					m_bAllowClientSideAddString;// : 1;
    bool					m_bUserDataFixedSize;// : 1;

    int						m_nFlags; // NSF_*

    int						m_nUserDataSize;
    int						m_nUserDataSizeBits;

    // Change function callback
    pfnStringChanged		m_changeFunc;
    // Optional context/object
    void* m_pObject;

    // pointer to local backdoor table 
    INetworkStringTable* m_pMirrorTable[MIRROR_TABLE_MAX_COUNT];
    //char pad2[4];
    INetworkStringDict* m_pItems;
    INetworkStringDict* m_pItemsClientSide;	 // For m_bAllowClientSideAddString, these items are non-networked and are referenced by a negative string index!!!
};

inline void dump_offsets()
{
    printf("INetworkStringTable: \n");
    printf("m_pItems %d\n", offsetof(INetworkStringTable, m_pItems));
    printf("m_changeFunc %d\n", offsetof(INetworkStringTable, m_changeFunc));
    printf("m_pObject %d\n", offsetof(INetworkStringTable, m_pObject));
    printf("m_nTickCount %d\n", offsetof(INetworkStringTable, m_nTickCount));
    printf("m_nLastChangedTick %d\n", offsetof(INetworkStringTable, m_nLastChangedTick));
    printf("INetworkStringTableItem: \n");
    printf("m_pChangeList %d\n", offsetof(INetworkStringTableItem, m_pChangeList));
    //printf("m_bChangeHistoryEnabled %d\n", offsetof(INetworkStringTable, m_bChangeHistoryEnabled));

}


class INetworkStringTableItem;
class CNetworkStringTableContainer
{


public:
    // implemenatation INetworkStringTableContainer
    virtual void constructor() = 0;
    virtual INetworkStringTable* CreateStringTable(const char* tableName, int maxentries) = 0;
    virtual void				RemoveAllTables(void) = 0;

    virtual INetworkStringTable* FindTable(const char* tableName) const = 0;
    virtual INetworkStringTable* GetTable(int stringTable) const = 0;
    virtual int					GetNumTables(void) const = 0;

    // Print contents to console
    virtual void					Dump(void);

    virtual void					WriteStringTables(void* buf) = 0;
    virtual bool					ReadStringTables(void* buf) = 0;
};

#define INVALID_STRING_INDEX 0xFFFF