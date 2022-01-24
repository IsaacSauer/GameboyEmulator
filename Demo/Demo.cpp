#include <iostream>

#include <mutex>

#include "../GameboyEmulator/Emulator.h"
#include "nfd\include\FileDialog.h"
#include "../GameboyEmulator/GameBoy.h"

#include "SDL.h"
#include "SDL_image.h"

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl.h"

#include <Measure.h>
#include <fstream>
#include <string>

/*
	This is just a proof of concept, a quick example as to how you'd use the library
*/

#define INSTANCECOUNT 1

SDL_Window* gWind{};
SDL_Renderer* gRendr{};
SDL_Texture* gTextures[INSTANCECOUNT];

std::vector<uint16_t> gFrameBuffer{};
int gSpeedModifiers{ 1 };
Measure gMeasure{ "timing" };
bool gStarted = false;
std::vector<uint64_t> gMsPerFrame{};
std::string gCurrentFile{};

void save_texture(SDL_Renderer* ren, SDL_Texture* tex, const char* filename)
{
	//Source: https://stackoverflow.com/a/48176678
	SDL_Texture* ren_tex;
	SDL_Surface* surf;
	int st;
	int w;
	int h;
	int format;
	void* pixels;

	pixels = NULL;
	surf = NULL;
	ren_tex = NULL;
	format = SDL_PIXELFORMAT_RGBA32;

	/* Get information about texture we want to save */
	st = SDL_QueryTexture(tex, NULL, NULL, &w, &h);
	if (st != 0) {
		SDL_Log("Failed querying texture: %s\n", SDL_GetError());
		goto cleanup;
	}

	ren_tex = SDL_CreateTexture(ren, format, SDL_TEXTUREACCESS_TARGET, w, h);
	if (!ren_tex) {
		SDL_Log("Failed creating render texture: %s\n", SDL_GetError());
		goto cleanup;
	}

	/*
	 * Initialize our canvas, then copy texture to a target whose pixel data we
	 * can access
	 */
	st = SDL_SetRenderTarget(ren, ren_tex);
	if (st != 0) {
		SDL_Log("Failed setting render target: %s\n", SDL_GetError());
		goto cleanup;
	}

	SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(ren);

	st = SDL_RenderCopy(ren, tex, NULL, NULL);
	if (st != 0) {
		SDL_Log("Failed copying texture data: %s\n", SDL_GetError());
		goto cleanup;
	}

	/* Create buffer to hold texture data and load it */
	pixels = malloc(w * h * SDL_BYTESPERPIXEL(format));
	if (!pixels) {
		SDL_Log("Failed allocating memory\n");
		goto cleanup;
	}

	st = SDL_RenderReadPixels(ren, NULL, format, pixels, w * SDL_BYTESPERPIXEL(format));
	if (st != 0) {
		SDL_Log("Failed reading pixel data: %s\n", SDL_GetError());
		goto cleanup;
	}

	/* Copy pixel data over to surface */
	surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, SDL_BITSPERPIXEL(format), w * SDL_BYTESPERPIXEL(format), format);
	if (!surf) {
		SDL_Log("Failed creating new surface: %s\n", SDL_GetError());
		goto cleanup;
	}

	/* Save result to an image */
	st = SDL_SaveBMP(surf, filename);
	if (st != 0) {
		SDL_Log("Failed saving image: %s\n", SDL_GetError());
		goto cleanup;
	}

	SDL_Log("Saved texture as BMP to \"%s\"\n", filename);

cleanup:
	SDL_FreeSurface(surf);
	free(pixels);
	SDL_DestroyTexture(ren_tex);
}

int GetOpenGLDriverIndex()
{
	auto openglIndex = -1;
	const auto driverCount = SDL_GetNumRenderDrivers();
	for (auto i = 0; i < driverCount; i++)
	{
		SDL_RendererInfo info;
		if (!SDL_GetRenderDriverInfo(i, &info))
			if (!strcmp(info.name, "opengl"))
				openglIndex = i;
	}
	return openglIndex;
}

void SetKeyState(const SDL_Event& event, const gbee::Emulator& emu)
{
	//Just ini 0
	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) return;

	Key key;
	switch (event.key.keysym.sym)
	{
	case SDLK_a:
		key = aButton;
		break;
	case SDLK_d:
		key = bButton;
		break;
	case SDLK_RETURN:
		key = start;
		break;
	case SDLK_SPACE:
		key = select;
		break;
	case SDLK_RIGHT:
		key = right;
		break;
	case SDLK_LEFT:
		key = left;
		break;
	case SDLK_UP:
		key = up;
		break;
	case SDLK_DOWN:
		key = down;
		break;
	default:
		return;
	}
	emu.SetKeyState(key, event.type == SDL_KEYDOWN, 0);
}

bool SDLEventPump(const gbee::Emulator& emu)
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		SetKeyState(e, emu);
		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT)
			return false;
	}
	return true;
}

void CraftPixelBuffer(const uint8_t instance, uint16_t* buffer, const gbee::Emulator& emu)
{
	std::bitset<160 * 144 * 2> bitBuffer{ emu.GetFrameBuffer(instance) };

#pragma loop(hint_parallel(20))
	for (int i{ 0 }; i < 160 * 144; ++i)
	{
		const uint8_t val{ static_cast<uint8_t>(bitBuffer[i * 2] << 1 | static_cast<uint8_t>(bitBuffer[i * 2 + 1])) };
		buffer[i] = 0xFFFF - val * 0x5555;
	}
}

void VBlankCallback(const std::vector<uint16_t>& buffer)
{
	gMsPerFrame.push_back(gMeasure.Stop());

	if (gRendr)
		gFrameBuffer = buffer;

	gMeasure.Start();
}

void Update(gbee::Emulator& emu)
{
	const float fps{ 59.73f };
	bool autoSpeed{ false };
	int speedModifiers{ gSpeedModifiers };

	while (SDLEventPump(emu))
	{
		SDL_RenderClear(gRendr);
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(gWind);
		ImGui::NewFrame();

		//IMGUI
		{
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBackground;

			ImGuiStyle& style = ImGui::GetStyle();
			style.FrameRounding = 4.f;
			style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
			style.WindowBorderSize = 0.f;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos({ 800, 0 });
			ImGui::SetNextWindowSize({ 1280 - 800, 720 });

			ImGui::Begin("Settings", nullptr, flags);
			//ImGui::SetWindowSize({ 300, 230 });

			if (ImGui::Button("Load ROM...", ImVec2{ 100, 20 }))
			{
				std::string path{};

				if (OpenFileDialog(path))
				{
					//emu.Stop();

					//emu.LoadGame(path);
					//emu.Start();
					emu.Stop();
					emu.Join();
					emu.Reset();
					emu.AssignDrawCallback(VBlankCallback);
					emu.LoadGame(path);
					emu.Start();
				}

				gCurrentFile = path.substr(path.find_last_of("/\\") + 1);
			}
			ImGui::SameLine();
			ImGui::TextWrapped(gCurrentFile.c_str());

			if (ImGui::Button("Toggle Paused", ImVec2{ 100, 20 }))
				emu.SetPauseState(!emu.GetPauseState(0), 0);

			if (ImGui::Checkbox("AutoSpeed", &autoSpeed))
				emu.SetAutoSpeed(autoSpeed, 0);
			ImGui::SameLine();
			ImGui::SliderInt("Speed", &speedModifiers, 1, 100);

			ImGui::Spacing();

			//Screenshot
			if (ImGui::Button("Save Screenshot...", ImVec2{ 150, 20 }))
			{
				emu.SetPauseState(true, 0);
				std::string path{};

				if (SaveFileDialog(path))
				{
					save_texture(gRendr, gTextures[0], path.c_str());
				}

				emu.SetPauseState(false, 0);
			}

			//COLOR
			ImGui::Spacing();
			if (ImGui::TreeNode("Colors:"))
			{
				{
					static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
					ImGuiColorEditFlags misc_flags = flags;
					flags |= ImGuiColorEditFlags_NoAlpha;        // This is by default if you call ColorPicker3() instead of ColorPicker4()
					flags |= ImGuiColorEditFlags_AlphaBar;
					flags |= ImGuiColorEditFlags_PickerHueBar;
					flags |= ImGuiColorEditFlags_PickerHueWheel;
					flags |= ImGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays
					flags |= ImGuiColorEditFlags_DisplayRGB;     // Override display mode
					flags |= ImGuiColorEditFlags_DisplayHSV;
					flags |= ImGuiColorEditFlags_DisplayHex;
					if (ImGui::ColorPicker4("Color", (float*)&color, misc_flags, nullptr))
					{
						emu.SetColor0((float*)&color);
					}
					ImGui::TreePop();
				}
			}

			ImGui::End();
			emu.SetSpeed(speedModifiers, 0);
		}

		//DRAWING
		if (true)
			SDL_UpdateTexture(gTextures[0], nullptr, static_cast<void*>(gFrameBuffer.data()), 160 * sizeof(uint16_t));
		else
		{
			uint16_t* pixelBuffer = new uint16_t[160 * 140]{};

			//uint16_t pixelBuffer[160 * 144]{};
			CraftPixelBuffer(static_cast<uint8_t>(0), pixelBuffer, emu);
			SDL_UpdateTexture(gTextures[0], nullptr, static_cast<void*>(pixelBuffer), 160 * sizeof(uint16_t));

			delete[] pixelBuffer;
		}

		SDL_SetRenderDrawColor(gRendr, 114 - 50, 144 - 50, 154 - 50, 255);

		{
			SDL_Rect texture_rect;
			texture_rect.x = 0;  //the x coordinate
			texture_rect.y = 0; // the y coordinate
			texture_rect.w = 800; //the width of the texture
			texture_rect.h = 720; //the height of the texture
			SDL_RenderCopy(gRendr, gTextures[0], NULL, &texture_rect);
		}

		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		SDL_RenderPresent(gRendr);
	}
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	gWind = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE); // Original = 800 x 720
	gRendr = SDL_CreateRenderer(gWind, GetOpenGLDriverIndex(), SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	std::string path{};
	if (false)
	{
		gbee::Emulator testEmu{ "Tetris(JUE)(V1.1)[!].gb", INSTANCECOUNT };
		testEmu.TestCPU();
	}
	//Choose rom
	else if (OpenFileDialog(path))
	{
		try
		{
			gbee::Emulator emum{ path, 1 };

			emum.AssignDrawCallback(VBlankCallback);
			emum.LoadGame(path);

			gMeasure.Start();

			emum.Start();

			gCurrentFile = path.substr(path.find_last_of("/\\") + 1);

			SDL_RenderClear(gRendr);
			SDL_SetRenderDrawColor(gRendr, 114 - 50, 144 - 50, 154 - 50, 255);
			SDL_RenderPresent(gRendr);

			for (int i{ 0 }; i < INSTANCECOUNT; ++i)
			{
				gTextures[i] = SDL_CreateTexture(gRendr, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 160, 144);

				SDL_SetRenderTarget(gRendr, gTextures[i]);
				SDL_SetRenderDrawColor(gRendr, 255, 0, 255, 255);
				SDL_RenderClear(gRendr);
				SDL_SetRenderTarget(gRendr, nullptr);
			}

			ImGui::CreateContext();
			ImGui_ImplSDL2_InitForOpenGL(gWind, SDL_GL_GetCurrentContext());
			ImGui_ImplOpenGL2_Init();

			Update(emum);

			emum.Stop();
			emum.Join();

			//Log Timings
			std::ofstream out;
			std::string file{ "timingsMultiplier" };
			file += std::to_string(gSpeedModifiers);
			file += ".txt";
			out.open(file);
			for (auto time : gMsPerFrame)
			{
				out << time << '\n';
			}
			out.close();

			SDL_DestroyRenderer(gRendr);
			SDL_DestroyWindow(gWind);
			for (int i{ 0 }; i < 1; ++i)
			{
				SDL_DestroyTexture(gTextures[i]);
			}

			ImGui::DestroyContext();
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		catch (const std::string& e)
		{
			std::cout << e << std::endl;
		}
	}

	return 0;
}