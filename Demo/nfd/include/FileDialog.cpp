//Interface by Isaac Sauer
#include "FileDialog.h"
#include "nfd.h"

bool OpenFileDialog(std::string& out, const std::string& filter)
{
	nfdchar_t* outPath = nullptr;
	const nfdresult_t result = NFD_OpenDialog(filter.empty() ? nullptr : filter.c_str(), nullptr, &outPath);

	if (result == NFD_OKAY)
	{
		out = outPath;

		puts("Success!");
		puts(outPath);
		free(outPath);
	}
	else if (result == NFD_CANCEL)
		puts("User pressed cancel.");
	else
		printf("Error: %s\n", NFD_GetError());

	return result == NFD_OKAY;
}

bool SaveFileDialog(std::string& out, const std::string& filter)
{
	nfdchar_t* outPath = nullptr;
	const nfdresult_t result = NFD_SaveDialog(filter.empty() ? nullptr : filter.c_str(), nullptr, &outPath);

	if (result == NFD_OKAY)
	{
		out = outPath;

		puts("Success!");
		puts(outPath);
		free(outPath);
	}
	else if (result == NFD_CANCEL)
		puts("User pressed cancel.");
	else
		printf("Error: %s\n", NFD_GetError());

	return result == NFD_OKAY;
}

bool HasEnding(const std::string& fullString, const std::string& ending)
{
	if (fullString.length() >= ending.length())
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	else
		return false;
}