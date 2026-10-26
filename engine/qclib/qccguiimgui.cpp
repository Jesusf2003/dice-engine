/*Copyright Spoike, license is GPLv2+ */
//Dear ImGui frontend for fteqcc (SDL2 + OpenGL3, so it runs on windows, linux and mac), a replacement for the Qt one in qccguiqt.cpp.
//Shares qccguistuff.c (options, config, GoToDefinition, Grep) with the other frontends.

/*Todo (in no particular order):
	debugger (qccguiqt.cpp talks to the engine through a QProcess)
	jump to the line on EditFile/clicking an error
	syntax highlighting, autocomplete (GenAutoCompleteList)
	options dialog
	decompiler output saving
*/

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

static std::string cmdlineargs;
static SDL_Window *g_window;

static progfuncs_t guiprogfuncs;
static progexterns_t guiprogexterns;

#undef Sys_Error

//---------------------------------------------------------------------------
//gui state
static bool want_compile;
static bool want_quit;
static std::string want_open;	//project to switch to, outside of the frame
static bool openproject_popup;

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
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, text, g_window);
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
		chdir(dir.c_str());
		src = sl;
	}

	QC_strlcpy(progssrcname, src, sizeof(progssrcname));

	docs.clear();
	QCC_CloseAllVFiles();

	GUI_SetDefaultOpts();
	GUI_ParseCommandLine(cmdlineargs.c_str(), true);
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

static void OpenProjectDialog(void)
{
#ifdef _WIN32
	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version);
	char filename[MAX_PATH] = "";
	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = SDL_GetWindowWMInfo(g_window, &wm) ? wm.info.win.window : nullptr;
	ofn.lpstrFilter = "QuakeC projects (*.src, *.dat, *.pk3, *.zip)\0*.src;*.dat;*.pk3;*.zip\0All files\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn))
		want_open = filename;
#else
	//SDL2 has no native file dialog, ask for the path instead.
	openproject_popup = true;
#endif
}

static void DrawOpenProjectPopup(void)
{
	static std::string path;
	if (openproject_popup)
	{
		ImGui::OpenPopup("Open project");
		openproject_popup = false;
	}
	if (ImGui::BeginPopupModal("Open project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Path to a .src, .dat, .pk3 or .zip:");
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 30);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		bool accept = ImGui::InputText("##path", &path, ImGuiInputTextFlags_EnterReturnsTrue);
		accept |= ImGui::Button("Open");
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		if (accept && !path.empty())
		{
			want_open = path;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
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

static void DrawUI(void)
{
	ImGuiIO &io = ImGui::GetIO();

	if (ImGui::IsKeyChordPressed(ImGuiKey_F7))
		want_compile = true;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open project..."))
				OpenProjectDialog();
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
	DrawOpenProjectPopup();

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
//sdl2 + opengl plumbing

const char *glsl_version;

void SDL_CInit() {

    //start up the gui parts
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "fteqccgui", SDL_GetError(), nullptr);
		exit (1);
	}

#ifdef __APPLE__
	glsl_version = "#version 150";	//mac only does 3.2+ as a forward-compatible core profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (i > 1)
			cmdlineargs += ' ';
		if (strpbrk(argv[i], " \t"))
			cmdlineargs += std::string("\"") + argv[i] + "\"";
		else
			cmdlineargs += argv[i];
	}
	GUI_SetDefaultOpts();
	if (GUI_ParseCommandLine(cmdlineargs.c_str(), false) == 1)
	{	//compile-only
		RunCompiler(cmdlineargs.c_str(), false);
		return EXIT_SUCCESS;
	}

	SDL_CInit();

	//macs scale through SDL_WINDOW_ALLOW_HIGHDPI instead.
	float scale = 1.0f;
#ifndef __APPLE__
	float dpi;
	if (SDL_GetDisplayDPI(0, nullptr, &dpi, nullptr) == 0 && dpi > 96.0f)
		scale = dpi / 96.0f;
#endif

	g_window = SDL_CreateWindow("FTEQCC GUI (ImGui edition)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(int)(1280 * scale), (int)(800 * scale), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GLContext glcontext = g_window ? SDL_GL_CreateContext(g_window) : nullptr;
	if (!glcontext)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "fteqccgui", "Couldn't create an OpenGL 3 context.", g_window);
		if (g_window)
			SDL_DestroyWindow(g_window);
		SDL_Quit();
		return 1;
	}
	SDL_GL_MakeCurrent(g_window, glcontext);
	SDL_GL_SetSwapInterval(1);	//vsync, otherwise we redraw as fast as possible and eat a whole core.

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

	ImGui_ImplSDL2_InitForOpenGL(g_window, glcontext);
	ImGui_ImplOpenGL3_Init(glsl_version);

	GUIprintf("Welcome to FTEQCC!\n(ImGui edition)\n");

	//and now set up our project...
	if (*progssrcname)
		SetMainSrcFile(progssrcname);
	else
		SetMainSrcFile("progs.src");

	while (!want_quit)
	{
		//an editor doesn't need to redraw constantly, sleep until there's input (or ~60fps for animations).
		SDL_Event ev;
		if (SDL_WaitEventTimeout(&ev, 16))
		{
			do
			{
				ImGui_ImplSDL2_ProcessEvent(&ev);
				if (ev.type == SDL_QUIT)
					want_quit = true;
				else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE && ev.window.windowID == SDL_GetWindowID(g_window))
					want_quit = true;
			} while (SDL_PollEvent(&ev));
		}
		if (want_quit)
			break;
		if (SDL_GetWindowFlags(g_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		//compiling/switching projects outside of the frame, so the log/docs aren't touched while they're being drawn.
		if (!want_open.empty())
		{
			std::string src = std::move(want_open);
			want_open.clear();
			SetMainSrcFile(src.c_str());
		}
		if (want_compile)
		{
			want_compile = false;
			RunCompiler(parameters, false);
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		DrawUI();

		ImGui::Render();
		int width, height;
		SDL_GL_GetDrawableSize(g_window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(g_window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
