//--------------------------------------------------------------------------//
/// Copyright 2023 Milos Tosic. All Rights Reserved.                       ///
/// License: http://www.opensource.org/licenses/BSD-2-Clause               ///
//--------------------------------------------------------------------------//

#include <MTunerInject_pch.h>

#if RTM_PLATFORM_WINDOWS
#include <Shlobj.h>
#include <shellapi.h>

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

void err(const char* _message = 0)
{
	rtm::Console::error(_message ? _message : "This program is internal to MTuner. Do not call it directly!");
	exit(0);
}

const char* findMTunerInjectExe(const char* _string)
{
	uint32_t len = (uint32_t)strlen(_string);
	const char*		exePos = rtm::strStr<rtm::toUpper>(_string, len, "MTUNERINJECT_DEBUG");	// handle running from debugger
	if (!exePos)	exePos = rtm::strStr<rtm::toUpper>(_string, len, "MTUNERINJECT");
	return exePos;
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

int main(int argc, const char** /*argv*/)
{
#if !RTM_PLATFORM_WINDOWS
	(void)argc;
	err("Only Windows supported currently!");
	return 0;
#else

	// prepare DLL path
	wchar_t dllPathWide[512];
	GetModuleFileNameW(NULL, dllPathWide, 512); 
	rtm::WideToMulti dllPathMulti(dllPathWide);

	rtm::pathCanonicalize(dllPathMulti);

	const char* exePos = findMTunerInjectExe(dllPathMulti);
	if (!exePos)
		err("Could not locate executable!");

#if RTM_64BIT
	strcpy(const_cast<char*>(exePos), "MTunerDLL64.dll");
#else
	strcpy(const_cast<char*>(exePos), "MTunerDLL32.dll");
#endif

	// prepare storage path
    char captureDir[512];
    getStoragePath(captureDir);
    strcat(captureDir,"\\MTuner");

    CreateDirectoryW(rtm::MultiToWide(captureDir), NULL);

	if (argc == 1)
		err();

	LPWSTR cmd = GetCommandLineW();

	wchar_t* end = wcsstr(cmd,L"#23#");
	if (!end)
		err();

	end[-1] = L'\0';

	// executable
	cmd = end+4;
	end = wcsstr(cmd,L"#23#");
	*end = L'\0';

	wchar_t executable[1024];
	wcscpy(executable, cmd);

	// CMD line arguments
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
		wcscpy(workingDir, executable);
		size_t len = wcslen(workingDir);
		while ((workingDir[len] != L'/') && (workingDir[len] != L'\\')) --len;
		workingDir[len+1] = L'\0';
	}

	if (!SetCurrentDirectoryW(workingDir))
		err("Could not change working directory!");

	FILE* checkExists = _wfopen(executable, L"rb");
	if (!checkExists)
		err("Executable doesn't exist!");
	else
		fclose(checkExists);

	uint32_t pid = 0;
	if (!rdebug::processInjectDLL(	rtm::WideToMulti(executable),
									dllPathMulti,
									rtm::WideToMulti(cmdLine),
									rtm::WideToMulti(workingDir),
									&pid))
	{
		err();
	}
	
	return pid;
#endif // RTM_PLATFORM_WINDOWS
}
