#ifndef __REGISTRY__H_
#define __REGISTRY__H_

#include <windows.h>

/* 	baseKey:
	HKEY_CLASSES_ROOT
	HKEY_CURRENT_CONFIG
	HKEY_CURRENT_USER
	HKEY_LOCAL_MACHINE
	HKEY_USERS
	

	all string length input should include the terminating null character,
	all string length returned not including the terminating null character

	value type:
	REG_BINARY
	REG_DWORD
	REG_DWORD_LITTLE_ENDIAN
	REG_DWORD_BIG_ENDIAN
	REG_EXPAND_SZ
	REG_LINK
	REG_MULTI_SZ
	REG_NONE
	REG_QWORD
	REG_QWORD_LITTLE_ENDIAN
	REG_SZ
*/
	

class Registry {
public: 
	HKEY key;		

	Registry(HKEY baseKey, LPCTSTR subKey, 
				bool create = false, LPTSTR keyClass = nullptr, 
				DWORD option = 0, REGSAM sam = KEY_ALL_ACCESS, 
				LPSECURITY_ATTRIBUTES security = nullptr);
	~Registry();

	LONG NullTerminate(LONG ret, DWORD bufLen, LPDWORD  type, 
			LPBYTE  data, LPDWORD  dataLen);
	
	LONG getKeyInfo(LPTSTR keyClass = nullptr, LPDWORD keyClassLen = nullptr, 
		LPDWORD subKeyCount = nullptr, LPDWORD maxSubKeyLen = nullptr, 
		LPDWORD maxSubKeyClassLen = nullptr, LPDWORD valueCount = nullptr, 
		LPDWORD maxValueNameLen = nullptr, LPDWORD maxValueLen = nullptr, 
		LPDWORD security= nullptr, PFILETIME lastWirteTime = nullptr);

	LONG getValueInfo(LPCTSTR name, LPDWORD  type, LPBYTE  data, 
		LPDWORD  dataLen);

	LONG getSubKeyValue(LPCTSTR subKey, LPCTSTR name, DWORD  flag = 0, 
		LPDWORD type = nullptr, PVOID data = nullptr, LPDWORD dataLen = nullptr);

	LONG enumSubKey(DWORD index, LPTSTR name, LPDWORD nameLen, 
		LPTSTR subKeyClass = nullptr, LPDWORD subKeyClassLen = nullptr,
		PFILETIME lastWriteTime = nullptr);

	LONG enumValue(DWORD index, LPTSTR name, LPDWORD nameLen, 
		LPDWORD  type = nullptr, LPBYTE  data = nullptr, LPDWORD  dataLen = nullptr);

	LONG DeleteSubKey(LPCTSTR subKey, REGSAM sam = 0);

	LONG DeleteValue(LPCTSTR name);

#if _WIN32_WINNT >= _WIN32_WINNT_WIN6
	LONG DeleteSubKeyValue(LPCTSTR subKey, LPCTSTR  name);

	LONG DeleteTree(LPCTSTR subKey);

	LONG SetValue(LPCTSTR name, DWORD type, const BYTE *data = nullptr, DWORD  dataLen = 0);
	
	LONG SetSubKeyValue(LPCTSTR subKey, LPCTSTR name, 
		DWORD type, LPCVOID data = nullptr, DWORD dataLen = 0);
#endif

	LONG WaitChange(BOOL subTree, DWORD  filter, 
		HANDLE event = nullptr, BOOL async = false);
};
#endif
