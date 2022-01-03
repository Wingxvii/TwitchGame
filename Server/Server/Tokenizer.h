#pragma once
#include <string>
#include <vector>

class Tokenizer
{
public:
	static std::vector<std::string> tokenize(char token, std::string text)
	{
		std::vector<std::string> returnText;
		int lastLoc = 0;

		for (unsigned int i = 0; i < text.size(); i++)
		{
			if (text[i] == token)
			{
				returnText.push_back(text.substr(lastLoc, i - lastLoc));
				lastLoc = i + 1;
			}
		}
		returnText.push_back(text.substr(lastLoc, text.size()));

		return returnText;
	}

};