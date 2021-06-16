/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>

int Translator_DetectLanguage(char* inputString, char* detectedLanguage, size_t detectedLanguageLen);
int Translator_Translate(char* inputString, char* FromLang, char* outputString, size_t outputStringLen, char *ToLang);
