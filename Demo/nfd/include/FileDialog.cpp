//Interface by Isaac Sauer
#include "FileDialog.h"
#include "nfd.h"

bool OpenFileDialog(std::string& out)
{
	nfdchar_t* outPath = nullptr;
	const nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &outPath);

	if (result == NFD_OKAY)
	{
		out = outPath;

		puts("Success!");
		puts(outPath);
		free(outPath);
	}
	else if (result == NFD_CANCEL)
	{
		puts("User pressed cancel.");
	}
	else
	{
		printf("Error: %s\n", NFD_GetError());
	}

	return result == NFD_OKAY;
}

bool SaveFileDialog(std::string& out)
{
	nfdchar_t* outPath = nullptr;
	const nfdresult_t result = NFD_SaveDialog(nullptr, nullptr, &outPath);

	if (result == NFD_OKAY)
	{
		out = outPath;

		puts("Success!");
		puts(outPath);
		free(outPath);
	}
	else if (result == NFD_CANCEL)
	{
		puts("User pressed cancel.");
	}
	else
	{
		printf("Error: %s\n", NFD_GetError());
	}

	return result == NFD_OKAY;
}