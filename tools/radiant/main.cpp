//
//	radiant - map editor. SDL2 + OpenGL3 window, the editor frame itself lives in wndMain.
//

#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

#include "config.h"
#include "ui/wndMain.h"

radiantdefs_t radiantdef;

static void ShowError(const char *what)
{
	char msg[512];
	std::snprintf(msg, sizeof(msg), "%s: %s", what, SDL_GetError());
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "radiant", msg, nullptr);
}

void InitSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		ShowError("SDL_Init");
		exit (1);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

void ShutdownSDL(SDL_Window *w, SDL_GLContext gl)
{
    ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(w);
	SDL_Quit();
}

int main(int, char **)
{
    InitSDL();
	SDL_Window *window = SDL_CreateWindow(radiantdef.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, radiantdef.width, radiantdef.height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window)
	{
		ShowError("SDL_CreateWindow");
		SDL_Quit();
		return 1;
	}
	SDL_GLContext gl = SDL_GL_CreateContext(window);
	if (!gl)
	{
		ShowError("SDL_GL_CreateContext");
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	SDL_GL_MakeCurrent(window, gl);
	SDL_GL_SetSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "radiant.ini";
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	WndMain_Create(window);

	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				running = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				running = false;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		WndMain_Draw();

		ImGui::Render();
		int w, h;
		SDL_GL_GetDrawableSize(window, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
	WndMain_Destroy();
    ShutdownSDL(window, gl);
	return 0;
}
