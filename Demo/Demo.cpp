#include <iostream>
#include "imgui/SDL2-2.0.10/include/SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_sdl.h"
#include "imgui/imgui_impl_sdl.h"
#include "../GameboyEmulator/EmulatorClean.h"
#include "nfd\include\FileDialog.h"

/*
	This is just a proof of concept, a quick example as to how you'd use the library
*/

#define INSTANCECOUNT 1

SDL_Window* wind{};
SDL_Renderer* rendr{};
SDL_Texture* textures[INSTANCECOUNT];
gbee::Emulator emu{ "Tetris(JUE)(V1.1)[!].gb", INSTANCECOUNT };
//gbee::Emulator emu{ "PinballFantasies(USA,Europe).gb", INSTANCECOUNT };

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
	emu.SetKeyState(key, event.type == SDL_KEYDOWN, 0);
}

bool SDLEventPump()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		SetKeyState(e);
		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT) return false;
	}
	return true;
}

void CraftPixelBuffer(const uint8_t instance, uint16_t* buffer)
{
	std::bitset<160 * 144 * 2> bitBuffer{ emu.GetFrameBuffer(instance) };

#pragma loop(hint_parallel(20))
	for (int i{ 0 }; i < 160 * 144; ++i)
	{
		const uint8_t val{ static_cast<uint8_t>(bitBuffer[i * 2] << 1 | static_cast<uint8_t>(bitBuffer[i * 2 + 1])) };
		buffer[i] = 0xFFFF - val * 0x5555;
	}
}

void Update()
{
	const float fps{ 59.73f };
	bool autoSpeed[INSTANCECOUNT]{};
	int speedModifiers[INSTANCECOUNT];

	while (SDLEventPump())
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = 1.0f / fps; //Runs a tick behind.. (Due to while loop condition)

		ImGui::NewFrame();

		//ImGui::ShowDemoWindow();
		uint16_t pixelBuffer[160 * 144]{};
		for (int i{ 0 }; i < INSTANCECOUNT; ++i)
		{
			const std::string name{ "Instance " };
			CraftPixelBuffer(static_cast<uint8_t>(i), pixelBuffer);
			SDL_UpdateTexture(textures[i], nullptr, static_cast<void*>(pixelBuffer), 160 * sizeof(uint16_t));
			speedModifiers[i] = emu.GetSpeed(static_cast<uint8_t>(i));

			ImGui::Begin((name + std::to_string(i)).c_str(), nullptr, ImGuiWindowFlags_NoResize);
			ImGui::SetWindowSize({ 300, 230 });
			ImGui::Image(textures[i], { 160, 144 });
			if (ImGui::Button("Toggle Paused"))
				emu.SetPauseState(!emu.GetPauseState(static_cast<uint8_t>(i)), static_cast<uint8_t>(i));

			if (ImGui::Checkbox("AutoSpeed", autoSpeed + i))
				emu.SetAutoSpeed(autoSpeed[i], static_cast<uint8_t>(i));
			ImGui::SameLine();
			ImGui::SliderInt("Speed", &speedModifiers[i], 1, 100);

			ImGui::End();
			emu.SetSpeed(static_cast<uint16_t>(speedModifiers[i]), static_cast<uint8_t>(i));
		}
		//ImGui::ShowMetricsWindow();

		SDL_SetRenderDrawColor(rendr, 114, 144, 154, 255);
		SDL_RenderClear(rendr);

		ImGui::Render();
		ImGuiSDL::Render(ImGui::GetDrawData());

		SDL_RenderPresent(rendr);
	}

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

	emu.TestCPU();

	//Choose rom
	std::string path{};
	if (OpenFileDialog(path))
	{
		emu.LoadGame(path);
		emu.Start();

		wind = SDL_CreateWindow("SDL2 ImGui Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
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
		ImGuiSDL::Initialize(rendr, 800, 600);

		Update();
	}
	return 0;
}