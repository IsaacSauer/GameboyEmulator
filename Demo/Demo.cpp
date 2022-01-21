#include <iostream>

#include <mutex>

#include "../GameboyEmulator/Emulator.h"
#include "nfd\include\FileDialog.h"
#include "../GameboyEmulator/Tile.h"
#include "../GameboyEmulator/GameBoy.h"

#include "SDL.h"
#include "SDL_image.h"

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl.h"

/*
	This is just a proof of concept, a quick example as to how you'd use the library
*/

#define INSTANCECOUNT 1

SDL_Window* wind{};
SDL_Renderer* rendr{};
SDL_Texture* textures[INSTANCECOUNT];
//gbee::Emulator* emu{ };

std::vector<uint16_t> frameBuffer{};

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

void VBlankCallback(const FrameBuffer& buffer)
{
	if (rendr)
	{
		//MAKE BUFFER
		frameBuffer = buffer.GetBuffer();
	}
}

void Update(const gbee::Emulator& emu)
{
	const float fps{ 59.73f };

	while (SDLEventPump(emu))
	{
		SDL_RenderClear(rendr);
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(wind);
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		//DRAWING
		if (true)
			SDL_UpdateTexture(textures[0], nullptr, static_cast<void*>(frameBuffer.data()), 160 * sizeof(uint16_t));
		else
		{
			uint16_t* pixelBuffer = new uint16_t[160 * 140]{};

			//uint16_t pixelBuffer[160 * 144]{};
			CraftPixelBuffer(static_cast<uint8_t>(0), pixelBuffer, emu);
			SDL_UpdateTexture(textures[0], nullptr, static_cast<void*>(pixelBuffer), 160 * sizeof(uint16_t));

			delete[] pixelBuffer;
		}

		SDL_SetRenderDrawColor(rendr, 114, 144, 154, 255);

		{
			SDL_Rect texture_rect;
			texture_rect.x = 0;  //the x coordinate
			texture_rect.y = 0; // the y coordinate
			texture_rect.w = 800; //the width of the texture
			texture_rect.h = 720; //the height of the texture
			SDL_RenderCopy(rendr, textures[0], NULL, &texture_rect);
		}


		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		SDL_RenderPresent(rendr);
	}
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

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
			emum.Start();

			std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
			wind = SDL_CreateWindow(base_filename.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, SDL_WINDOW_RESIZABLE); // Original = 800 x 720
			rendr = SDL_CreateRenderer(wind, GetOpenGLDriverIndex(), SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

			SDL_RenderClear(rendr);
			SDL_SetRenderDrawColor(rendr, 114, 144, 154, 255);
			SDL_RenderPresent(rendr);

			for (int i{ 0 }; i < INSTANCECOUNT; ++i)
			{
				textures[i] = SDL_CreateTexture(rendr, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 160, 144);

				SDL_SetRenderTarget(rendr, textures[i]);
				SDL_SetRenderDrawColor(rendr, 255, 0, 255, 255);
				SDL_RenderClear(rendr);
				SDL_SetRenderTarget(rendr, nullptr);
			}

			ImGui::CreateContext();
			ImGui_ImplSDL2_InitForOpenGL(wind, SDL_GL_GetCurrentContext());
			ImGui_ImplOpenGL2_Init();

			//ImGui::Initialize(rendr, 800, 800);

			Update(emum);

			emum.Stop();
			//ImGui::Deinitialize();

			SDL_DestroyRenderer(rendr);
			SDL_DestroyWindow(wind);
			for (int i{ 0 }; i < 1; ++i)
			{
				SDL_DestroyTexture(textures[i]);
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