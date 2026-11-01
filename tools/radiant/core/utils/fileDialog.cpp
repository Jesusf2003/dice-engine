//==============================
//	fileDialog.cpp
//==============================

#include "core/utils/fileDialog.h"
#include "core/common/log.h"
#include "ui/wndMain.h"

#include <SDL.h>
#include <SDL_syswm.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>	// IFileOpenDialog, for folders

static std::wstring Widen(const std::string &s)
{
	if (s.empty())
		return std::wstring();
	int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
	return w;
}

static std::string Narrow(const wchar_t *w)
{
	int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
	if (n <= 1)
		return std::string();
	std::string s(n - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
	return s;
}

static HWND MainHwnd()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (g_sdlMain && SDL_GetWindowWMInfo(g_sdlMain, &info))
		return info.info.win.window;
	return nullptr;
}

bool FileDialog::OpenFile(std::string &outPath, const char *title, const char *filterName, const char *pattern,
	const std::string &initialDir)
{
	// "Quake map (*.map)\0*.map\0All files\0*.*\0\0"
	std::wstring filter = Widen(std::string(filterName) + " (" + pattern + ")");
	filter.push_back(L'\0');
	filter += Widen(pattern);
	filter.push_back(L'\0');
	filter += L"All files";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');

	wchar_t file[MAX_PATH * 4] = L"";
	std::wstring wTitle = Widen(title);
	std::wstring wDir = Widen(initialDir);

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = MainHwnd();
	ofn.lpstrFilter = filter.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = file;
	ofn.nMaxFile = (DWORD)(sizeof(file) / sizeof(file[0]));
	ofn.lpstrInitialDir = wDir.empty() ? nullptr : wDir.c_str();
	ofn.lpstrTitle = wTitle.c_str();
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (!GetOpenFileNameW(&ofn))
	{
		DWORD err = CommDlgExtendedError();
		if (err)
			Log::Error("Open file dialog failed (0x%lx)", err);
		return false;
	}
	outPath = Narrow(file);
	return true;
}

/*
==================
FileDialog::OpenFolder

the vista style dialog in folder mode, GetOpenFileName can't pick folders
==================
*/
bool FileDialog::OpenFolder(std::string &outPath, const char *title, const std::string &initialDir)
{
	// SDL may have set up COM already, in whatever mode: the dialog works either way
	HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool picked = false;

	IFileOpenDialog *dlg = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg))))
	{
		DWORD opts = 0;
		dlg->GetOptions(&opts);
		dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
		dlg->SetTitle(Widen(title).c_str());

		IShellItem *start = nullptr;
		if (!initialDir.empty() && SUCCEEDED(SHCreateItemFromParsingName(Widen(initialDir).c_str(), nullptr, IID_PPV_ARGS(&start))))
		{
			dlg->SetFolder(start);
			start->Release();
		}

		IShellItem *item = nullptr;
		if (SUCCEEDED(dlg->Show(MainHwnd())) && SUCCEEDED(dlg->GetResult(&item)))
		{
			PWSTR path = nullptr;
			if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
			{
				outPath = Narrow(path);
				picked = true;
				CoTaskMemFree(path);
			}
			item->Release();
		}
		dlg->Release();
	}
	else
		Log::Error("Couldn't create the folder dialog");

	if (SUCCEEDED(init))
		CoUninitialize();
	return picked;
}

#else

bool FileDialog::OpenFile(std::string &, const char *, const char *, const char *, const std::string &)
{
	Log::Warning("No file dialog on this platform yet, open the map from the command line: radiant <file.map>");
	return false;
}

bool FileDialog::OpenFolder(std::string &, const char *, const std::string &)
{
	Log::Warning("No folder dialog on this platform yet, type the path");
	return false;
}

#endif
