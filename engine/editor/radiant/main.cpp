//radiant: map editor entry point.
//SDL2 owns the window and input, Dear ImGui (docking branch) the whole UI, OpenGL3 draws it.
//The panels mirror QuakeEd 3's windows (engine/editor/quaked3) so its code has somewhere to land while it's migrated.

#include <imgui.h>
#include <imgui_internal.h>	//DockBuilder
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>	//renames main to SDL_main, SDL2main provides the WinMain that calls it
#include <SDL_opengl.h>

namespace
{

struct Panels
{
	bool camera = true;
	bool grid = true;
	bool zchecker = true;
	bool console = true;
	bool entity = true;
	bool texture = true;
};

struct App
{
	SDL_Window *window = nullptr;
	SDL_GLContext gl = nullptr;
	Panels panels;
	bool resetLayout = false;
	bool showDemo = false;
	bool quit = false;
};

const char *const kCamera   = "Camera";
const char *const kGrid     = "Top (XY)";
const char *const kZ        = "Z";
const char *const kConsole  = "Console";
const char *const kEntity   = "Entities";
const char *const kTexture  = "Textures";

//Same arrangement as QuakeEd: grid in the middle, Z strip on the left, camera and inspectors on the right, console below.
void BuildDefaultLayout(ImGuiID dockspace)
{
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::DockBuilderRemoveNode(dockspace);
	ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace, vp->WorkSize);

	ImGuiID centre = dockspace;
	ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.22f, nullptr, &centre);
	ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.35f, nullptr, &centre);
	ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.06f, nullptr, &centre);
	ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.5f, nullptr, &right);

	ImGui::DockBuilderDockWindow(kGrid, centre);
	ImGui::DockBuilderDockWindow(kZ, left);
	ImGui::DockBuilderDockWindow(kCamera, right);
	ImGui::DockBuilderDockWindow(kEntity, rightBottom);
	ImGui::DockBuilderDockWindow(kTexture, rightBottom);
	ImGui::DockBuilderDockWindow(kConsole, bottom);
	ImGui::DockBuilderFinish(dockspace);
}

void DrawMenuBar(App &app)
{
	if (!ImGui::BeginMainMenuBar())
		return;
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New", "Ctrl+N", false, false);
		ImGui::MenuItem("Open...", "Ctrl+O", false, false);
		ImGui::MenuItem("Save", "Ctrl+S", false, false);
		ImGui::Separator();
		if (ImGui::MenuItem("Exit"))
			app.quit = true;
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem(kCamera, nullptr, &app.panels.camera);
		ImGui::MenuItem(kGrid, nullptr, &app.panels.grid);
		ImGui::MenuItem(kZ, nullptr, &app.panels.zchecker);
		ImGui::MenuItem(kEntity, nullptr, &app.panels.entity);
		ImGui::MenuItem(kTexture, nullptr, &app.panels.texture);
		ImGui::MenuItem(kConsole, nullptr, &app.panels.console);
		ImGui::Separator();
		if (ImGui::MenuItem("Reset layout"))
			app.resetLayout = true;
		ImGui::MenuItem("ImGui demo", nullptr, &app.showDemo);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

//Placeholder body for a panel that hasn't been migrated yet.
void Panel(const char *name, bool *open, const char *todo)
{
	if (!*open)
		return;
	if (ImGui::Begin(name, open))
		ImGui::TextDisabled("%s", todo);
	ImGui::End();
}

void DrawUI(App &app)
{
	DrawMenuBar(app);

	ImGuiID dockspace = ImGui::GetID("MainDock");
	if (app.resetLayout || !ImGui::DockBuilderGetNode(dockspace))
	{
		BuildDefaultLayout(dockspace);
		app.panels = Panels();
		app.resetLayout = false;
	}
	ImGui::DockSpaceOverViewport(dockspace);

	//The 3D/2D views will render into framebuffer objects and show them with ImGui::Image.
	Panel(kCamera, &app.panels.camera, "Camera view (quaked3: CameraView / CameraRenderer)");
	Panel(kGrid, &app.panels.grid, "Grid view (quaked3: GridView / GridViewRenderer)");
	Panel(kZ, &app.panels.zchecker, "Z checker (quaked3: ZView / ZViewRenderer)");
	Panel(kEntity, &app.panels.entity, "Entity inspector (quaked3: WndEntity / EntityView)");
	Panel(kTexture, &app.panels.texture, "Texture browser (quaked3: WndTexture / TextureView)");
	Panel(kConsole, &app.panels.console, "Console (quaked3: WndConsole / Log)");

	if (app.showDemo)
		ImGui::ShowDemoWindow(&app.showDemo);
}

void HandleShortcuts(App &app)
{
	if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Q))
		app.quit = true;
}

} //namespace

int main(int, char **)
{
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "radiant", SDL_GetError(), nullptr);
		return 1;
	}

#ifdef __APPLE__
	const char *glslVersion = "#version 150";	//mac only does 3.2+ as a forward-compatible core profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	const char *glslVersion = "#version 330";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	//macs scale through SDL_WINDOW_ALLOW_HIGHDPI instead.
	float scale = 1.0f;
#ifndef __APPLE__
	float dpi;
	if (SDL_GetDisplayDPI(0, nullptr, &dpi, nullptr) == 0 && dpi > 96.0f)
		scale = dpi / 96.0f;
#endif

	App app;
	app.window = SDL_CreateWindow("Radiant", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		static_cast<int>(1600 * scale), static_cast<int>(900 * scale),
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
	app.gl = app.window ? SDL_GL_CreateContext(app.window) : nullptr;
	if (!app.gl)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "radiant", "Couldn't create an OpenGL 3.3 context.", app.window);
		if (app.window)
			SDL_DestroyWindow(app.window);
		SDL_Quit();
		return 1;
	}
	SDL_GL_MakeCurrent(app.window, app.gl);
	SDL_GL_SetSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "radiant.ini";	//remembers the dock layout, next to the exe's working dir

	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::StyleColorsDark();
	style.FrameRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.ScaleAllSizes(scale);
	style.FontScaleDpi = scale;

	ImGui_ImplSDL2_InitForOpenGL(app.window, app.gl);
	ImGui_ImplOpenGL3_Init(glslVersion);

	while (!app.quit)
	{
		//vsync alone doesn't throttle every driver, so sleep until there's input (or ~60fps for anything animating).
		SDL_Event ev;
		if (SDL_WaitEventTimeout(&ev, 16))
		{
			do
			{
				ImGui_ImplSDL2_ProcessEvent(&ev);
				if (ev.type == SDL_QUIT)
					app.quit = true;
				else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE && ev.window.windowID == SDL_GetWindowID(app.window))
					app.quit = true;
			} while (SDL_PollEvent(&ev));
		}
		if (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		HandleShortcuts(app);
		DrawUI(app);

		ImGui::Render();
		int width, height;
		SDL_GL_GetDrawableSize(app.window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(app.window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(app.gl);
	SDL_DestroyWindow(app.window);
	SDL_Quit();
	return 0;
}
