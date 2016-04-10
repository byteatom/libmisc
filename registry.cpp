#include "registry.h"
#include <stdexcept>

Registry::Registry(HKEY baseKey, LPCTSTR subKey, bool create, 
	LPTSTR keyClass, DWORD option, REGSAM sam, LPSECURITY_ATTRIBUTES security) {

	LONG ret;
	if (create) {
		ret = RegCreateKeyEx(baseKey, subKey, 0, keyClass, option, sam, 
								security, &key, nullptr);
	} else {
		ret = RegOpenKeyEx(baseKey, subKey, option, sam, &key);
	}

	if (ERROR_SUCCESS != ret) throw std::invalid_argument("");
}

Registry::~Registry() {
	RegFlushKey(key);
	if (HKEY_CLASSES_ROOT != key &&
		HKEY_CURRENT_CONFIG != key &&
		HKEY_CURRENT_USER != key &&
		HKEY_LOCAL_MACHINE != key &&
		HKEY_USERS != key) {		
		RegCloseKey(key);
	}
}

LONG Registry::NullTerminate(LONG ret, DWORD bufLen, LPDWORD  type, 
	LPBYTE  data, LPDWORD  dataLen) {
	
	if (ERROR_SUCCESS == ret && nullptr != type && nullptr != data 
		&& nullptr != dataLen && 0 < *dataLen) {
		
		if (REG_SZ == *type || REG_EXPAND_SZ == *type) {
			if (0 != data[*dataLen - 1]) {
				if (bufLen > *dataLen) 
					data[*dataLen]= 0;
				else
					return ERROR_MORE_DATA;
			}
		} else if (REG_MULTI_SZ == *type && 1 < *dataLen) {
			if (0 != data[*dataLen - 1]) {
				if (bufLen > *dataLen + 1) {
					data[*dataLen]= 0;
					data[*dataLen + 1]= 0;
				}
				else
					return ERROR_MORE_DATA;
			} else if (0 != data[*dataLen - 2]) {
				if (bufLen > *dataLen) 
					data[*dataLen]= 0;
				else
					return ERROR_MORE_DATA;
			}
		}
	}

	return ret;
}


/*
return: 
ERROR_SUCCESS
ERROR_INVALID_PARAMETER
ERROR_MORE_DATA: buffer too small
*/
LONG Registry::getKeyInfo(LPTSTR keyClass, LPDWORD keyClassLen, 
	LPDWORD subKeyCount, LPDWORD maxSubKeyLen, LPDWORD maxSubKeyClassLen, 
	LPDWORD valueCount, LPDWORD maxValueNameLen, LPDWORD maxValueLen, 
	LPDWORD security, PFILETIME lastWirteTime) {

	return RegQueryInfoKey(key, keyClass, keyClassLen, nullptr, subKeyCount, maxSubKeyLen, 
		maxSubKeyClassLen, valueCount, maxValueNameLen, maxValueLen, security,
		lastWirteTime);
}


/*
return: 
ERROR_SUCCESS
ERROR_MORE_DATA: ERROR_MORE_DATA: buffer too small
ERROR_FILE_NOT_FOUND
other system error code

para:
name: null or "": unnamed or default value.
*/
LONG Registry::getValueInfo(LPCTSTR name, LPDWORD  type, LPBYTE  data, 
	LPDWORD  dataLen) {
	
	DWORD bufLen = (nullptr == dataLen ? 0 : *dataLen);
	LONG ret = RegQueryValueEx(key, name, nullptr, type, data, dataLen);
	return NullTerminate(ret, bufLen, type, data, dataLen);
}

/*
return: 
ERROR_SUCCESS
ERROR_MORE_DATA: ERROR_MORE_DATA: buffer too small
other system error code

para:
name: null or "": unnamed or default value.
*/
LONG Registry::getSubKeyValue(LPCTSTR subKey, LPCTSTR name, DWORD  flag, 
	LPDWORD type, PVOID data, LPDWORD dataLen) {
	
	DWORD bufLen = (nullptr == dataLen ? 0 : *dataLen);
	LONG ret = RegGetValue(key, subKey, name, flag, type, data, dataLen);
	return NullTerminate(ret, bufLen, type, (LPBYTE)data, dataLen);
}


/*
return: 
ERROR_SUCCESS
ERROR_NO_MORE_ITEMS
ERROR_MORE_DATA: buffer too small
other system error code
*/
LONG Registry::enumSubKey(DWORD index, LPTSTR name, LPDWORD nameLen, 
	LPTSTR subKeyClass, LPDWORD subKeyClassLen,	PFILETIME lastWriteTime) {

	return RegEnumKeyEx(key, index, name, nameLen, nullptr, subKeyClass, 
		subKeyClassLen, lastWriteTime);
}

/*
return: 
ERROR_SUCCESS
ERROR_NO_MORE_ITEMS
ERROR_MORE_DATA: buffer too small
other system error code
*/
LONG Registry::enumValue(DWORD index, LPTSTR name, LPDWORD nameLen, 
	LPDWORD  type, LPBYTE data, LPDWORD dataLen) {

	DWORD bufLen = (nullptr == dataLen ? 0 : *dataLen);
	LONG ret = RegEnumValue(key, index, name, nameLen, nullptr, type, data, dataLen);
	return NullTerminate(ret, bufLen, type, data, dataLen);
}

/*
return: 
ERROR_SUCCESS
nonzero error code
*/
LONG Registry::DeleteSubKey(LPCTSTR subKey, REGSAM sam) {
	return RegDeleteKeyEx(key, subKey, sam, 0);
}

/*
return: 
ERROR_SUCCESS
nonzero error code
*/
LONG Registry::DeleteValue(LPCTSTR name) {
	return RegDeleteValue(key, name);
}

#if _WIN32_WINNT >= _WIN32_WINNT_WIN6
/*
return: 
ERROR_SUCCESS
nonzero error code
*/
LONG Registry::DeleteSubKeyValue(LPCTSTR subKey, LPCTSTR  name) {
	return RegDeleteKeyValue(key, subKey, name);
}

/*
return: 
ERROR_SUCCESS
nonzero error code
*/
LONG Registry::DeleteTree(LPCTSTR subKey) {
	return RegDeleteTree(key, subKey);
}

/*
return: 
ERROR_SUCCESS
nonzero error code
para:
name: null or "": unnamed or default value.
data: backslash preceded by another backslash as an escape character, null valid
*/
LONG Registry::SetValue(LPCTSTR name, DWORD type, const BYTE *data, 
	DWORD  dataLen) {
	
	return RegSetValueEx(key, name, 0, type, data, dataLen);
}

/*
return: 
ERROR_SUCCESS
nonzero error code
para:
name: null or "": unnamed or default value.
data: backslash preceded by another backslash as an escape character, null valid
*/
LONG Registry::SetSubKeyValue(LPCTSTR subKey, LPCTSTR name, DWORD type, 
	LPCVOID data, DWORD dataLen) {
	
	return RegSetKeyValue(key, subKey, name, type, data, dataLen);
}
#endif

/*
para:
filter:
REG_NOTIFY_CHANGE_NAME
REG_NOTIFY_CHANGE_ATTRIBUTES
REG_NOTIFY_CHANGE_LAST_SET
REG_NOTIFY_CHANGE_SECURITY
REG_NOTIFY_THREAD_AGNOSTIC
*/
LONG Registry::WaitChange(BOOL subTree, DWORD  filter, HANDLE event, 
	BOOL async) {
	
	return RegNotifyChangeKeyValue(key, subTree, filter, event, async);
}

