#pragma once
#include <string>

//"png,jpg;pdf"
bool OpenFileDialog(std::string& out, const std::string& filter = "");

//"png,jpg;pdf"
bool SaveFileDialog(std::string& out, const std::string& filter = "");

bool HasEnding(const std::string& fullString, const std::string& ending);