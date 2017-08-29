//--------------------------------------------------------------------------//
/// Copyright (c) 2017 Milos Tosic. All Rights Reserved.                   ///
/// License: http://www.opensource.org/licenses/BSD-2-Clause               ///
//--------------------------------------------------------------------------//

#include <MTunerInject_pch.h>

#if RTM_PLATFORM_WINDOWS
#include <Shlobj.h>

bool windowsVersionGreaterOrEqual(DWORD majorVersion)
{
    OSVERSIONINFOEX osVersionInfo;
    ::ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.dwMajorVersion = majorVersion;
    ULONGLONG maskCondition = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    return ::VerifyVersionInfo(&osVersionInfo, VER_MAJORVERSION, maskCondition) ? true : false;
}

#endif

static const char* g_banner = "Copyright (c) 2017 Milos Tosic. All rights reserved.\n";

void err(const char* _msg = 0)
{
	rtm::Console::error(_msg ? _msg : "This program is internal to MTuner. Do not call it directly!");
	exit(EXIT_FAILURE);
} 

void getStoragePath(char _path[512])
{
#if RTM_PLATFORM_WINDOWS
	bool vistaOrHigher = windowsVersionGreaterOrEqual(6);
	bool pathRetrieved = false;

	if (vistaOrHigher)
	{
		wchar_t* folderPath;
		HRESULT hr = SHGetKnownFolderPath( FOLDERID_RoamingAppData, 0, NULL, &folderPath);
		if (hr == S_OK)
		{
			strcpy(_path, rtm::WideToMulti(folderPath));
			CoTaskMemFree(static_cast<void*>(folderPath));
			pathRetrieved = true;
		}
	}
	else
	{
		wchar_t path[512];
		HRESULT hr = SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path);
		if (hr == S_OK)
		{
			strcpy(_path, rtm::WideToMulti(path));
			pathRetrieved = true;
		}
	}

	if (!pathRetrieved)
	{
		// fall back on env variable
		wchar_t* path = _wgetenv(L"APPDATA");
		if (path)
			strcpy(_path, rtm::WideToMulti(path));
		else
			strcpy(_path, "");  // nothing worked ;(
	}

#else
	strcpy(_path, "");
#endif
}

int main(int argc, const char**)
{
	RTM_UNUSED(argc);

	rtm::Console::info("%s\n",g_banner);

    char captureDir[512];
    getStoragePath(captureDir);
    strcat(captureDir,"\\MTuner");

#if !RTM_PLATFORM_WINDOWS
	err("Only Windows supported currently!");
#else

    CreateDirectoryW(rtm::MultiToWide(captureDir), NULL);

	if (argc == 1)
		err();

	LPWSTR cmd = GetCommandLineW();

	wchar_t* end = wcsstr(cmd,L"#23#");
	if (!end)
		err();

	end[-1] = L'\0';

	wchar_t dllPath[1024];

#if RTM_PLATFORM_WINDOWS
	GetModuleFileNameW(GetModuleHandle(0), dllPath, 1024);
	rtm::WideToMulti dllPathM(dllPath);
	rtm::pathRemoveRelative(dllPathM);
	wcscpy(dllPath, rtm::MultiToWide(dllPathM));
#else
	wcscpy(dllPath,cmd);
#endif

	size_t len = wcslen(dllPath);
	while ((dllPath[len] != L'/') && (dllPath[len] != L'\\')) --len;

#if RTM_64BIT
	wcscpy(&dllPath[len+1],L"MTunerDLL64.dll");
#else
	wcscpy(&dllPath[len+1],L"MTunerDLL32.dll");
#endif

	if (dllPath[0] == L'\"')
		wcscat(dllPath, L"\"");

	// EXE
	cmd = end+4;
	end = wcsstr(cmd,L"#23#");
	*end = L'\0';

	wchar_t exePath[1024];
	wcscpy(exePath,cmd);

	FILE* checkExists = _wfopen(exePath,L"rb");
	if (!checkExists)
		err();
	else
		fclose(checkExists);
	
	// CMD line
	cmd = end+5;
	if (wcsncmp(cmd,L"#23#",4) != 0)
		err();
	cmd += 4;
	end = wcsstr(cmd,L"#23#");
	if (!end)
		err();
	*end = L'\0';

	wchar_t cmdLine[1024];
	wcscpy(cmdLine,cmd);

	// Working dir
	cmd = end+5;
	if (wcsncmp(cmd,L"#23#",4) != 0)
		err();
	cmd += 4;
	end = wcsstr(cmd,L"#23#");
	*end = L'\0';

	wchar_t workingDir[1024];
	wcscpy(workingDir,cmd);
	size_t sz = wcslen(workingDir);
	if (sz == 0)
	{
		wcscpy(workingDir,exePath);
		len = wcslen(workingDir);
		while ((workingDir[len] != L'/') && (workingDir[len] != L'\\')) --len;
		workingDir[len+1] = L'\0';
	}

	SetCurrentDirectoryW(workingDir);

	if (!rdebug::processInjectDLL(	rtm::WideToMulti(exePath),
									rtm::WideToMulti(dllPath),
									rtm::WideToMulti(cmdLine),
									rtm::WideToMulti(workingDir)))
	{
		err();
	}

#endif // RTM_PLATFORM_WINDOWS

	return 0;
}
