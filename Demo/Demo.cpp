#include <iostream>
#include "imgui/SDL2-2.0.10/include/SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_sdl.h"
#include "imgui/imgui_impl_sdl.h"
#include "../GameboyEmulator/EmulatorClean.h"
#include "nfd\include\FileDialog.h"
#include "../GameboyEmulator/Tile.h"
#include <mutex>

/*
	This is just a proof of concept, a quick example as to how you'd use the library
*/

#define INSTANCECOUNT 1

SDL_Window* wind{};
SDL_Renderer* rendr{};
SDL_Texture* textures[INSTANCECOUNT];
gbee::Emulator* emu{ };

std::vector<uint16_t> frameBuffer{};

void SetKeyState(const SDL_Event& event)
{
	//Just ini 0
	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) return;

	gbee::Key key;
	switch (event.key.keysym.sym)
	{
	case SDLK_a:
		key = gbee::aButton;
		break;
	case SDLK_d:
		key = gbee::bButton;
		break;
	case SDLK_RETURN:
		key = gbee::start;
		break;
	case SDLK_SPACE:
		key = gbee::select;
		break;
	case SDLK_RIGHT:
		key = gbee::right;
		break;
	case SDLK_LEFT:
		key = gbee::left;
		break;
	case SDLK_UP:
		key = gbee::up;
		break;
	case SDLK_DOWN:
		key = gbee::down;
		break;
	default:
		return;
	}
	emu->SetKeyState(key, event.type == SDL_KEYDOWN, 0);
}

bool SDLEventPump()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		SetKeyState(e);
		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT)
			return false;
	}
	return true;
}

void CraftPixelBuffer(const uint8_t instance, uint16_t* buffer)
{
	std::bitset<160 * 144 * 2> bitBuffer{ emu->GetFrameBuffer(instance) };

#pragma loop(hint_parallel(20))
	for (int i{ 0 }; i < 160 * 144; ++i)
	{
		const uint8_t val{ static_cast<uint8_t>(bitBuffer[i * 2] << 1 | static_cast<uint8_t>(bitBuffer[i * 2 + 1])) };
		buffer[i] = 0xFFFF - val * 0x5555;
	}
}

void VBlankCallback(const FrameBuffer& buffer)
{
	if ( rendr)
	{
		//MAKE BUFFER
		frameBuffer = buffer.GetBuffer();
	}
}

void Update()
{
	const float fps{ 59.73f };

	while (SDLEventPump() )
	{
		//DRAWING
		if (true)
			SDL_UpdateTexture(textures[0], nullptr, static_cast<void*>(frameBuffer.data()), 160 * sizeof(uint16_t));
		else
		{
			uint16_t pixelBuffer[160 * 144]{};
			CraftPixelBuffer(static_cast<uint8_t>(0), pixelBuffer);
			SDL_UpdateTexture(textures[0], nullptr, static_cast<void*>(pixelBuffer), 160 * sizeof(uint16_t));
		}

		SDL_RenderClear(rendr);
		SDL_Rect texture_rect;
		texture_rect.x = 0;  //the x coordinate
		texture_rect.y = 0; // the y coordinate
		texture_rect.w = 800; //the width of the texture
		texture_rect.h = 720; //the height of the texture
		SDL_RenderCopy(rendr, textures[0], NULL, &texture_rect);
		SDL_RenderPresent(rendr);
	}

	emu->Stop();
	ImGuiSDL::Deinitialize();

	SDL_DestroyRenderer(rendr);
	SDL_DestroyWindow(wind);
	for (int i{ 0 }; i < 1; ++i)
	{
		SDL_DestroyTexture(textures[i]);
	}

	ImGui::DestroyContext();
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
		gbee::Emulator emum{ path, 1 };
		emu = &emum;
		emum.AssignDrawCallback(VBlankCallback);
		emum.LoadGame(path);
		emum.Start();

		std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
		wind = SDL_CreateWindow(base_filename.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 720, SDL_WINDOW_RESIZABLE);
		rendr = SDL_CreateRenderer(wind, -1, SDL_RENDERER_ACCELERATED);

		for (int i{ 0 }; i < INSTANCECOUNT; ++i)
		{
			textures[i] = SDL_CreateTexture(rendr, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 160, 144);

			SDL_SetRenderTarget(rendr, textures[i]);
			SDL_SetRenderDrawColor(rendr, 255, 0, 255, 255);
			SDL_RenderClear(rendr);
			SDL_SetRenderTarget(rendr, nullptr);
		}

		ImGui::CreateContext();
		ImGuiSDL::Initialize(rendr, 800, 720);

		Update();
	}
	return 0;
}