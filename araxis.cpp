#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <algorithm>
#pragma comment(lib, "shell32.lib")

std::wstring ToLower(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

bool CopySelfToTarget(const wchar_t* targetPath) {
	wchar_t selfPath[MAX_PATH];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

	// Try copying
	BOOL ok = CopyFileW(selfPath, targetPath, FALSE);
	return ok == TRUE;
}

void RelaunchElevated() {
	wchar_t selfPath[MAX_PATH];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

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
	const wchar_t* bcompPath = L"D:\\Programs\\Beyond Compare 4\\bcomp.exe";
	const wchar_t* installPath = L"C:\\Program Files\\Araxis\\Araxis Merge\\";
	const wchar_t* compareExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\Compare.exe";
	const wchar_t* mergeExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\Merge.exe";
	const wchar_t* consoleCompareExe = L"C:\\Program Files\\Araxis\\Araxis Merge\\ConsoleCompare.exe";

	// Get own executable full path and filename
	wchar_t selfPath[MAX_PATH];
	GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

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

	// Determine mode based on executable name (lowercase)
	std::wstring selfName = selfPath;
	selfName = selfName.substr(selfName.find_last_of(L"\\/") + 1);
	std::wstring mode = ToLower(selfName);

	std::wstring args = L"\"";
	args += bcompPath;
	args += L"\" ";

	// Parse command line args again
	LPWSTR* szArgList;
	int argCount;
	szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);

	// Build args based on mode
	if (mode.find(L"merge") != std::wstring::npos) {
		// Merge.exe: expect 3 or 4 files for 3-way merge
		if (argCount < 4) {
			MessageBoxW(nullptr, L"Araxis Merge stub expects at least 3 arguments.", L"Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		// Typical merge args: local, remote, base, merged (4 args)
		// Append those files explicitly
		for (int i = 1; i < argCount; ++i) {
			args += L" \"";
			args += szArgList[i];
			args += L"\"";
		}
	}
	else {
		// Compare.exe or ConsoleCompare.exe: expect 2 files (local, remote)
		if (argCount < 3) {
			MessageBoxW(nullptr, L"Araxis Compare stub expects at least 2 arguments.", L"Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		// Only pass first two arguments
		args += L" \"";
		args += szArgList[1];
		args += L"\" \"";
		args += szArgList[2];
		args += L"\"";
	}

	LocalFree(szArgList);

	// Add Beyond Compare flags
	args += L" /wait /solo";

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
