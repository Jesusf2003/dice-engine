/*Copyright Spoike, license is GPLv2+ */
//Dear ImGui frontend for fteqcc (Win32 + legacy OpenGL), a replacement for the Qt one in qccguiqt.cpp.
//Shares qccguistuff.c (options, config, GoToDefinition, Grep) with the other frontends.

/*Todo (in no particular order):
	debugger (qccguiqt.cpp talks to the engine through a QProcess)
	jump to the line on EditFile/clicking an error
	syntax highlighting, autocomplete (GenAutoCompleteList)
	options dialog
	decompiler output saving
*/

#include <imgui.h>
#include <imgui_internal.h>	//DockBuilder
#include <imgui_stdlib.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_win32.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <GL/gl.h>
#include <sys/stat.h>

#include <string>
#include <vector>

extern "C"
{
#include "qcc.h"
#include "gui.h"

extern int qcc_compileactive;
};

static char *cmdlineargs;

static progfuncs_t guiprogfuncs;
static progexterns_t guiprogexterns;

#undef Sys_Error

void RunCompiler(const char *args, pbool quick);
void AddSourceFile(const char *parentpath, const char *filename);

//---------------------------------------------------------------------------
//gui state

struct document_s
{
	std::string name;
	std::string text;
	bool dirty = false;
};
static std::vector<document_s> docs;
static int docs_select = -1;	//tab to bring to front next frame

struct sourcefile_s
{
	std::string parent;
	std::string name;
};
static std::vector<sourcefile_s> sourcefiles;

struct logline_s
{
	std::string text;
	ImU32 colour;
};
static std::vector<logline_s> loglines;
static std::string logpartial;
static bool log_scrolltobottom;

static bool want_compile;
static bool want_quit;

//---------------------------------------------------------------------------
//file io, mostly the same as qccguiqt.cpp

void Sys_Error(const char *text, ...)
{
	va_list argptr;
	static char msg[2048];

	va_start (argptr,text);
	QC_vsnprintf (msg,sizeof(msg)-1, text,argptr);
	va_end (argptr);

	QCC_Error(ERR_INTERNAL, "%s", msg);
}

static void *QCC_ReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size)
{
	size_t len;
	unsigned char *buffer;

	vfile_t *v = QCC_FindVFile(fname);
	if (v)
	{
		len = v->size;
		if (buf_get)
			buffer = buf_get(buf_ctx, len+1);
		else
			buffer = static_cast<unsigned char *>(malloc(len+1));
		if (!buffer)
			return NULL;
		buffer[len] = 0;
		memcpy(buffer, v->file, len);
		if (out_size)
			*out_size = len;
		return buffer;
	}

	FILE *f = fopen(fname, "rb");
	if (!f)
	{
		if (out_size)
			*out_size = 0;
		return nullptr;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (buf_get)
		buffer = buf_get(buf_ctx, len+1);
	else
		buffer = static_cast<unsigned char *>(malloc(len+1));
	buffer[len] = 0;
	if (len != fread(buffer, 1, len, f))
	{
		if (!buf_get)
			free(buffer);
		buffer = nullptr;
	}
	fclose(f);

	if (out_size)
		*out_size = len;
	return buffer;
}
static int PDECL QCC_FileSize (const char *fname)
{
	vfile_t *v = QCC_FindVFile(fname);
	if (v)
		return v->size;

	long    length;
	FILE *f = fopen(fname, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fclose(f);
	return length;
}
static int PDECL QCC_PopFileSize (const char *fname)
{	//populates the file list, as well as returning the size
	int len = QCC_FileSize(fname);
	if (len >= 0 && qcc_compileactive)
		AddSourceFile(compilingrootfile, fname);
	return len;
}

pbool PDECL QCC_WriteFile (const char *name, void *data, int len)
{
	if (QCC_FindVFile(name))
		return !!QCC_AddVFile(name, data, len);

	FILE *f = fopen(name, "wb");
	if (!f)
		return false;
	size_t length = fwrite(data, 1, len, f);
	fclose(f);
	return length == (size_t)len;
}

void *GUIReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile)
{
	if (issourcefile)
		AddSourceFile(compilingrootfile, fname);
	return QCC_ReadFile(fname, buf_get, buf_ctx, out_size);
}
int GUIFileSize(const char *fname)
{
	return QCC_PopFileSize(fname);
}

//---------------------------------------------------------------------------
//hooks that qccguistuff.c / the compiler expect from the frontend

static bool SaveDocument(document_s &d)
{
	if (!QCC_WriteFile(d.name.c_str(), &d.text[0], (int)d.text.size()))
	{
		GUIprintf("Unable to write %s\n", d.name.c_str());
		return false;
	}
	d.dirty = false;
	return true;
}
static void SaveAllDocuments(void)
{
	for (auto &d : docs)
		if (d.dirty)
			SaveDocument(d);
}

void EditFile(const char *name, int line, pbool setcontrol)
{
	(void)line;	//fixme: scroll to it
	(void)setcontrol;
	for (size_t i = 0; i < docs.size(); i++)
	{
		if (docs[i].name == name)
		{
			docs_select = (int)i;
			return;
		}
	}

	size_t size;
	char *buffer = static_cast<char*>(QCC_ReadFile(name, nullptr, nullptr, &size));
	if (!buffer)
	{
		GUIprintf("Unable to open %s\n", name);
		return;
	}
	document_s d;
	d.name = name;
	d.text.assign(buffer, size);
	free(buffer);
	docs.push_back(std::move(d));
	docs_select = (int)docs.size()-1;
}

void AddSourceFile(const char *parentpath, const char *filename)
{
	std::string parent = parentpath?parentpath:"";
	if (parent == filename)
		return;	//the compiler reports the root file as including itself
	for (auto &s : sourcefiles)
		if (s.name == filename && s.parent == parent)
			return;
	sourcefiles.push_back({parent, filename});
}

void GUI_DialogPrint(const char *title, const char *text)
{
	MessageBoxA(nullptr, text, title, MB_OK);
}

void compilecb(void)
{
}

int GUIprintf(const char *msg, ...)
{
	va_list va;
	char buf[4096];

	if (!*msg)
	{	//starting a compile or something.
		loglines.clear();
		logpartial.clear();
		return 0;
	}

	va_start(va, msg);
	QC_vsnprintf(buf, sizeof(buf), msg, va);
	va_end(va);
	logpartial += buf;

	for (size_t idx; (idx = logpartial.find('\n')) != std::string::npos; )
	{
		std::string s = logpartial.substr(0, idx);
		logpartial.erase(0, idx+1);

		if (!s.compare(0, 6, "code: "))
			continue;	//annotations, the qt gui shows them inline.

		ImU32 colour;
		if (s.find(": error") != std::string::npos || s.find(": werror") != std::string::npos || !QC_strncasecmp(s.c_str(), "error", 5))
			colour = IM_COL32(255, 96, 96, 255);
		else if (s.find(": warning") != std::string::npos)
			colour = IM_COL32(230, 200, 80, 255);
		else
			colour = IM_COL32(220, 220, 220, 255);
		loglines.push_back({std::move(s), colour});
	}
	log_scrolltobottom = true;
	return 0;
}

static int Dummyprintf(const char *msg, ...){return 0;}

void RunCompiler(const char *args, pbool quick)
{
	static FILE *logfile;
	const char *argv[256];
	int argc;

	SaveAllDocuments();

	memset(&guiprogfuncs, 0, sizeof(guiprogfuncs));
	guiprogfuncs.funcs.parms = &guiprogexterns;
	memset(&guiprogexterns, 0, sizeof(guiprogexterns));
	guiprogexterns.ReadFile = GUIReadFile;
	guiprogexterns.FileSize = GUIFileSize;
	guiprogexterns.WriteFile = QCC_WriteFile;
	guiprogexterns.Sys_Error = Sys_Error;

	if (quick)
		guiprogexterns.Printf = Dummyprintf;
	else
	{
		guiprogexterns.Printf = GUIprintf;
		GUIprintf("");
	}
	guiprogexterns.DPrintf = guiprogexterns.Printf;

	if (logfile)
		fclose(logfile);
	if (fl_log && !quick)
		logfile = fopen("fteqcc.log", "wb");
	else
		logfile = NULL;

	argc = GUI_BuildParms(args, argv, sizeof(argv)/sizeof(argv[0]), quick);
	if (!argc)
		guiprogexterns.Printf("Too many args\n");
	else
		CompileParams(&guiprogfuncs, NULL, argc, argv);

	if (logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
}

static void GUI_DoDecompile(void *buf, size_t size)
{
	const char *c = ReadProgsCopyright((char*)buf, size);
	if (!c || !*c)
		c = "COPYRIGHT OWNER NOT KNOWN";
	GUIprintf("");
	GUIprintf("The copyright message from this progs is\n%s\nPlease respect the wishes and legal rights of the person who created this.\n", c);
	DecompileProgsDat(progssrcname, buf, size);
}

//---------------------------------------------------------------------------
//project handling

static void UpdateFileList(void)
{
	sourcefiles.clear();
	AddSourceFile(nullptr, progssrcname);

	size_t size;
	char *buffer = static_cast<char*>(QCC_ReadFile(progssrcname, nullptr, nullptr, &size));
	if (buffer)
	{
		pr_file_p = QCC_COM_Parse(buffer);
		if (*qcc_token != '#')
		{
			pr_file_p = QCC_COM_Parse(pr_file_p);   //we dont care about the produced progs.dat
			while(pr_file_p)
			{
				if (*qcc_token == '#')  //panic if there's preprocessor in there.
					break;

				AddSourceFile(progssrcname, qcc_token);
				pr_file_p = QCC_COM_Parse(pr_file_p);
			}
		}
		free(buffer);
	}

	//handle any #includes in there
	RunCompiler(parameters, true);
}

static void QCC_EnumerateFilesResult(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize)
{
	auto buffer = new char[plainsize];
	if (QC_decode(nullptr, compsize, plainsize, method, compdata, buffer))
		QCC_AddVFile(name, buffer, plainsize);

	delete [] buffer;
}

static void SetMainSrcFile(const char *src)
{
	//if its a path, chdir to it instead
	const char *sl = strrchr(src, '/');
	const char *bsl = strrchr(src, '\\');
	if (bsl > sl)
		sl = bsl;
	if (sl)
	{
		sl++;
		std::string dir(src, sl-src);
		_chdir(dir.c_str());
		src = sl;
	}

	QC_strlcpy(progssrcname, src, sizeof(progssrcname));

	docs.clear();
	QCC_CloseAllVFiles();

	GUI_SetDefaultOpts();
	GUI_ParseCommandLine(cmdlineargs, true);
	GUI_RevealOptions();

	//if the project is a .dat or .zip then decompile it now (so we can access the 'source')
	char *ext = strrchr(progssrcname, '.');
	if (ext && (!QC_strcasecmp(ext, ".dat") || !QC_strcasecmp(ext, ".pak") || !QC_strcasecmp(ext, ".zip") || !QC_strcasecmp(ext, ".pk3")))
	{
		FILE *f = fopen(progssrcname, "rb");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			size_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			auto buf = new char[size];
			if (size == fread(buf, 1, size, f))
			{
				if (!QC_EnumerateFilesFromBlob(buf, size, QCC_EnumerateFilesResult) && !QC_strcasecmp(ext, ".dat"))
					GUI_DoDecompile(buf, size);	//its a .dat and contains no .src files
				else if (!QCC_FindVFile("progs.src"))
				{
					vfile_t *v = QCC_FindVFile("progs.dat");
					if (v)
						GUI_DoDecompile(v->file, v->size);
				}
			}
			delete [] buf;
			fclose(f);
		}
		strcpy(progssrcname, "progs.src");

		for (int i = 0; compiler_flag[i].abbrev; i++)
		{
			if (!strcmp("embedsrc", compiler_flag[i].abbrev))
			{
				compiler_flag[i].flags |= FLAG_SETINGUI;
				break;
			}
		}
	}

	UpdateFileList();
	EditFile(progssrcname, -1, false);
}

static void OpenProjectDialog(HWND hwnd)
{
	char filename[MAX_PATH] = "";
	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "QuakeC projects (*.src, *.dat, *.pk3, *.zip)\0*.src;*.dat;*.pk3;*.zip\0All files\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn))
		SetMainSrcFile(filename);
}

//returns the file+line of a "foo.qc:123: error: ..." log line.
static bool ParseLogLocation(const std::string &s, std::string &file, int &line)
{
	size_t start = (s.size() > 2 && s[1] == ':') ? 2 : 0;	//skip drive letters
	size_t c1 = s.find(':', start);
	if (c1 == std::string::npos || c1+1 >= s.size() || !isdigit((unsigned char)s[c1+1]))
		return false;
	file = s.substr(0, c1);
	line = atoi(s.c_str()+c1+1);
	return true;
}

//---------------------------------------------------------------------------
//ui

static void DrawFileTree(const std::string &parent, int depth = 0)
{
	if (depth > 32)
		return;	//#include loops
	for (size_t i = 0; i < sourcefiles.size(); i++)
	{
		const sourcefile_s &s = sourcefiles[i];
		if (s.parent != parent)
			continue;

		bool haschildren = false;
		for (auto &c : sourcefiles)
			if (c.parent == s.name && c.name != s.name)
			{
				haschildren = true;
				break;
			}

		ImGui::PushID((int)i);
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (!haschildren)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		bool open = ImGui::TreeNodeEx(s.name.c_str(), flags);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			EditFile(s.name.c_str(), -1, false);
		if (haschildren && open)
		{
			DrawFileTree(s.name, depth+1);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

static void DrawUI(HWND hwnd)
{
	ImGuiIO &io = ImGui::GetIO();

	if (ImGui::IsKeyChordPressed(ImGuiKey_F7))
		want_compile = true;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open project..."))
				OpenProjectDialog(hwnd);
			if (ImGui::MenuItem("Save all", "Ctrl+Shift+S"))
				SaveAllDocuments();
			ImGui::Separator();
			if (ImGui::MenuItem("Exit"))
				want_quit = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Build"))
		{
			if (ImGui::MenuItem("Compile", "F7"))
				want_compile = true;
			ImGui::EndMenu();
		}
		ImGui::TextDisabled("  %s", progssrcname);
		ImGui::EndMainMenuBar();
	}
	if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S))
		SaveAllDocuments();

	//default layout: files on the left, output at the bottom, editor in the middle.
	ImGuiID dockspace = ImGui::GetID("MainDock");
	if (!ImGui::DockBuilderGetNode(dockspace))
	{
		ImGuiViewport *vp = ImGui::GetMainViewport();
		ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace, vp->WorkSize);
		ImGuiID centre = dockspace;
		ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.2f, nullptr, &centre);
		ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.3f, nullptr, &centre);
		ImGui::DockBuilderDockWindow("Files", left);
		ImGui::DockBuilderDockWindow("Output", bottom);
		ImGui::DockBuilderDockWindow("Editor", centre);
		ImGui::DockBuilderFinish(dockspace);
	}
	ImGui::DockSpaceOverViewport(dockspace);

	if (ImGui::Begin("Files"))
		DrawFileTree("");
	ImGui::End();

	if (ImGui::Begin("Editor"))
	{
		int closeidx = -1;
		if (ImGui::BeginTabBar("docs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll))
		{
			for (size_t i = 0; i < docs.size(); i++)
			{
				document_s &d = docs[i];
				bool open = true;
				ImGuiTabItemFlags flags = 0;
				if (d.dirty)
					flags |= ImGuiTabItemFlags_UnsavedDocument;
				if (docs_select == (int)i)
					flags |= ImGuiTabItemFlags_SetSelected;
				if (ImGui::BeginTabItem(d.name.c_str(), &open, flags))
				{
					if (ImGui::InputTextMultiline("##text", &d.text, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_AllowTabInput))
						d.dirty = true;
					if (ImGui::IsItemFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S))
						SaveDocument(d);
					ImGui::EndTabItem();
				}
				if (!open)
					closeidx = (int)i;	//fixme: ask before discarding changes
			}
			ImGui::EndTabBar();
		}
		docs_select = -1;
		if (closeidx >= 0)
			docs.erase(docs.begin()+closeidx);
	}
	ImGui::End();

	if (ImGui::Begin("Output"))
	{
		if (ImGui::SmallButton("Clear"))
			loglines.clear();
		ImGui::SameLine();
		ImGui::TextDisabled("double-click an error to open its file");
		ImGui::Separator();
		if (ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
		{
			ImGuiListClipper clipper;
			clipper.Begin((int)loglines.size());
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					bool selected = false;
					ImGui::PushID(i);
					ImGui::PushStyleColor(ImGuiCol_Text, loglines[i].colour);
					if (ImGui::Selectable(loglines[i].text.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						std::string file;
						int line;
						if (ParseLogLocation(loglines[i].text, file, line))
							EditFile(file.c_str(), line, true);
					}
					ImGui::PopStyleColor();
					ImGui::PopID();
				}
			}
			if (log_scrolltobottom)
				ImGui::SetScrollHereY(1.0f);
			log_scrolltobottom = false;
		}
		ImGui::EndChild();
	}
	ImGui::End();

	(void)io;
}

//---------------------------------------------------------------------------
//win32 + opengl plumbing

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HDC g_hdc;
static HGLRC g_hrc;
static int g_width = 1280, g_height = 800;

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
		{
			g_width = LOWORD(lparam);
			g_height = HIWORD(lparam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wparam & 0xfff0) == SC_KEYMENU)	//no alt menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static bool CreateGLContext(HWND hwnd)
{
	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	g_hdc = GetDC(hwnd);
	int pf = ChoosePixelFormat(g_hdc, &pfd);
	if (!pf || !SetPixelFormat(g_hdc, pf, &pfd))
		return false;
	g_hrc = wglCreateContext(g_hdc);
	if (!g_hrc || !wglMakeCurrent(g_hdc, g_hrc))
		return false;

	//vsync, otherwise we redraw as fast as possible and eat a whole core.
	typedef BOOL (WINAPI *wglSwapIntervalEXT_t)(int interval);
	auto wglSwapIntervalEXT = (wglSwapIntervalEXT_t)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(1);
	return true;
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmdline, int)
{
	//handle initial commandline args
	cmdlineargs = cmdline;
	GUI_SetDefaultOpts();
	if (GUI_ParseCommandLine(cmdlineargs, false) == 1)
	{	//compile-only
		RunCompiler(cmdlineargs, false);
		return EXIT_SUCCESS;
	}

	//start up the gui parts
	ImGui_ImplWin32_EnableDpiAwareness();
	float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

	WNDCLASSEXW wc = {sizeof(wc), CS_OWNDC, WndProc, 0, 0, hinst, nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"fteqccgui-imgui", nullptr};
	RegisterClassExW(&wc);
	HWND hwnd = CreateWindowW(wc.lpszClassName, L"FTEQCC GUI (ImGui edition)", WS_OVERLAPPEDWINDOW,
		100, 100, (int)(g_width * scale), (int)(g_height * scale), nullptr, nullptr, hinst, nullptr);

	if (!CreateGLContext(hwnd))
	{
		MessageBoxW(hwnd, L"Couldn't create an OpenGL context.", L"fteqccgui", MB_ICONERROR);
		DestroyWindow(hwnd);
		UnregisterClassW(wc.lpszClassName, hinst);
		return 1;
	}
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = nullptr;	//always start with the default layout

	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::StyleColorsDark();
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.ScaleAllSizes(scale);
	style.FontScaleDpi = scale;

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplOpenGL2_Init();

	GUIprintf("Welcome to FTEQCC!\n(ImGui edition)\n");

	//and now set up our project...
	if (*progssrcname)
		SetMainSrcFile(progssrcname);
	else
		SetMainSrcFile("progs.src");

	while (!want_quit)
	{
		//an editor doesn't need to redraw constantly, sleep until there's input (or ~60fps for animations).
		MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				want_quit = true;
		}
		if (want_quit)
			break;
		if (IsIconic(hwnd))
		{
			Sleep(10);
			continue;
		}

		//compiling outside of the frame, so the log/docs aren't touched while they're being drawn.
		if (want_compile)
		{
			want_compile = false;
			RunCompiler(parameters, false);
		}

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawUI(hwnd);

		ImGui::Render();
		glViewport(0, 0, g_width, g_height);
		glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		SwapBuffers(g_hdc);
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(g_hrc);
	ReleaseDC(hwnd, g_hdc);
	DestroyWindow(hwnd);
	UnregisterClassW(wc.lpszClassName, hinst);
	return 0;
}
