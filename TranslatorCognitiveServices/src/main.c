#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "translator.h"

int main(int argc, char *argv[])
{
	char LangId[40] = { 0 };
	char OutputString[260] = { 0 };
	static char* inputText = "Il pleut. Tu devrais prendre un parapluie.";

	Log_Debug("Application starting.\n");
	Log_Debug("Input text: %s\n", inputText);

	if (Translator_DetectLanguage(inputText, &LangId[0], sizeof(LangId)) == 0)		// if detect worked
	{
		Log_Debug("Detected language: %s\n", LangId);
		if (Translator_Translate(inputText, LangId, OutputString, sizeof(OutputString), "en") == 0)
		{
			Log_Debug("Translated Text: %s\n", OutputString);
		}
	}

	while(true);		// spin
	
	return 0;
}
