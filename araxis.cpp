#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#pragma comment(lib, "shell32.lib")

std::wstring ToLower(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

bool CopySelfToTarget(const wchar_t* targetPath) {
	wchar_t selfPath[MAX_PATH + 1];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH + 1);

	// Try copying
	BOOL ok = CopyFileW(selfPath, targetPath, FALSE);
	return ok == TRUE;
}

void RelaunchElevated() {
	wchar_t selfPath[MAX_PATH + 1];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH + 1);

	SHELLEXECUTEINFOW sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = selfPath;
	sei.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteExW(&sei)) {
		MessageBoxW(nullptr, L"Failed to request elevation.", L"Error", MB_OK | MB_ICONERROR);
	}
}

bool IsRunningAsAdmin() {
	BOOL isAdmin = FALSE;
	HANDLE token = nullptr;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		TOKEN_ELEVATION elevation;
		DWORD size;
		if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
			isAdmin = elevation.TokenIsElevated != 0;
		}
		CloseHandle(token);
	}

	return isAdmin == TRUE;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	const wchar_t* bcompPath = L"D:\\Programs\\Beyond Compare 4\\BCompare.exe";
	const wchar_t* installPath = L"C:\\Program Files\\Araxis\\Araxis Merge\\";
	const wchar_t* compareExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\Compare.exe";
	const wchar_t* mergeExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\Merge.exe";
	const wchar_t* consoleCompareExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\ConsoleCompare.exe";

	// Get own executable full path and filename
	wchar_t selfPath[MAX_PATH + 1];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH + 1);

	// If no arguments: install copies to all 3 exe names
	if (__argc <= 1) {
		SHCreateDirectoryExW(nullptr, installPath, nullptr);
		BOOL ok1 = CopyFileW(selfPath, compareExe, FALSE);
		BOOL ok2 = CopyFileW(selfPath, mergeExe, FALSE);
		BOOL ok3 = CopyFileW(selfPath, consoleCompareExe, FALSE);

		if (ok1 && ok2 && ok3) {
			MessageBoxW(nullptr, L"Araxis => BComp Stub installed successfully as Compare.exe, Merge.exe, and ConsoleCompare.exe.", L"Success", MB_OK);
			return 0;
		} else {
			if (!IsRunningAsAdmin()) {
				// Not elevated, request elevation once
				RelaunchElevated();
			} else {
				MessageBoxW(nullptr, L"Failed to install stub even with admin rights.", L"Error", MB_OK | MB_ICONERROR);
			}
			return 1;
		}
	}

	// Parse command line args again
	LPWSTR* szArgList;
	int argCount;
	szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);

	std::wstring args = L"";
	bool isMerge = false;
	for (int i = 1; i < argCount; ++i) {
		std::wstring arg = szArgList[i];
		std::transform(arg.begin(), arg.end(), arg.begin(), ::towlower);
		if (arg == L"-merge" || arg == L"/merge" || arg == L"-3") {
			isMerge = true;
			break;
		}
	}
	int numParams = isMerge ? 4 : 2;

	std::vector<std::wstring> filesFound;
	for (int i = 1; i < argCount && filesFound.size() < numParams; ++i)
	{
		std::wstring arg = szArgList[i];
		if (arg.empty() || arg[0] == L'-' || arg[0] == L'/') {
			// Skip flags
			continue;
		}

		filesFound.push_back(arg);
	}

	if (filesFound.size() < numParams) {
		MessageBoxW(nullptr, L"Not enough file paths provided in arguments.", L"Error", MB_OK | MB_ICONERROR);
		LocalFree(szArgList);
		return 1;
	}

	if (isMerge &&
		filesFound[1].find(L"BASE") != std::wstring::npos &&
		filesFound[2].find(L"REMOTE") != std::wstring::npos)
	{
		std::swap(filesFound[1], filesFound[2]);
	}

	for (int i = 0; i < filesFound.size(); ++i)
	{
		args += L" \"";
		args += filesFound[i];
		args += L"\"";
	}

	// Add Beyond Compare flags
	args += L" /wait";

	/*
	std::wstringstream debugMsg;
	debugMsg << L"Command line:\n" << GetCommandLineW() << L"\n\n";
	debugMsg << L"Arg count: " << argCount << L"\n\n";
	debugMsg << L"Final args:\n" << args << L"\n\n";
	debugMsg << L"Self path:\n" << selfPath << L"\n\n";
	for (int i = 0; i < argCount; ++i)
	{
		debugMsg << L"[" << i << "] " << szArgList[i] << L"\n";
	}
	MessageBoxW(nullptr, debugMsg.str().c_str(), L"Debug Info", MB_OK);
	//*/

	LocalFree(szArgList);
	
	// Launch Beyond Compare with constructed args
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	BOOL success = CreateProcessW(
		bcompPath,
		&args[0],
		nullptr, nullptr, FALSE,
		0, nullptr, nullptr,
		&si, &pi
	);

	if (success) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 0;
	}
	else {
		MessageBoxW(nullptr, L"Failed to launch Beyond Compare.", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
}
