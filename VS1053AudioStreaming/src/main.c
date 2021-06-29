/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

#include <applibs/log.h>
#include <applibs/storage.h>

#include "vs1053.h"

// KOUW/NPR Seattle 32kbps audio stream
static char httpRequest[1024];
static uint8_t audioBuffer[4096];

// NPR/KUOW 32kbps stream: https://17853.live.streamtheworld.com/KUOWFM_LOW_MP3.mp3
static const char* streamHost = "17853.live.streamtheworld.com";
static const char* streamPath = "KUOWFM_LOW_MP3.mp3";
static const char *request_template = "GET /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: audio/mpeg\r\n\r\n";
static const char* httpOK = "HTTP/1.0 200 OK";
static const char* httpServiceNotAvailable = "HTTP/1.0 503";

int ReadEmbeddedAudio(char* audioFile, uint8_t **audioData, ssize_t *audioLength)
{
    *audioData = NULL;
    *audioLength = 0;

    int audioFd = Storage_OpenFileInImagePackage(audioFile);
    if (audioFd >= 0)
    {
        off_t length = lseek(audioFd, 0, SEEK_END);	// get the length of the file

        *audioData = (uint8_t*)calloc((size_t)length, 1);	// allocate space for the audio
        if (*audioData == NULL)
        {
            Log_Debug("Failed to allocate memory for Embedded Resource\n");
            close(audioFd);
            return -1;
        }

        lseek(audioFd, 0, SEEK_SET);
        read(audioFd, *audioData, (size_t)length);
        *audioLength = (ssize_t)length;
        close(audioFd);
    }
    else
    {
        Log_Debug("Cannot open %s\n", audioFile);
        return -1;
    }

    return 0;
}

int InitSocket(void)
{
    int _sockFd = -1;
    in_addr_t in_addr;
    struct hostent* hostent;
    struct sockaddr_in sockaddr_in;
    unsigned short server_port = 80;

    _sockFd = socket(AF_INET, SOCK_STREAM, 0);
    hostent = gethostbyname(streamHost);
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);
    if (connect(_sockFd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        Log_Debug("Failed to connect\n");
        return -1;
    }

    return _sockFd;
}

int PlayEmbeddedResource(void)
{
    uint8_t* pAudio = NULL;
    ssize_t audioLength = 0;

    int ret = ReadEmbeddedAudio("speech.mp3", &pAudio, &audioLength);
    if (ret != 0)
    {
        Log_Debug("Failed to get audio data\n");
        return -1;
    }

    if (VS1053_Init() == 0)
    {
        VS1053_SetVolume(20);

        for (ssize_t x = 0; x < audioLength; x++)
        {
            VS1053_PlayByte(pAudio[x]);
        }

        free(pAudio);
        VS1053_SetVolume(0);
        Log_Debug("Cleanup\n");
        VS1053_Cleanup();
    }
    else
    {
        Log_Debug("Failed to initialize VS1053 hardware\n");
        return -1;
    }

    return 0;
}

int PlayInternetRadio(void)
{
    int SockFd = InitSocket();

    if (SockFd == -1)
    {
        Log_Debug("Failed to initialize socket\n");
        return -1;
    }

    if (VS1053_Init() == 0)
    {
        VS1053_SetVolume(20);

        ssize_t length = 0;

        // setup the HTTP request
        snprintf(httpRequest, 1024, request_template, streamPath, streamHost);
        Log_Debug("Request: %s\n", httpRequest);
        // write the HTTP Request.
        write(SockFd, httpRequest, strlen(httpRequest));

        while (true)
        {
            // read audio data
            length = read(SockFd, audioBuffer, 4096);
            if (length <= 0)
            {
                Log_Debug("!read\n");
                break;
            }
            else
            {
                if (strncmp(audioBuffer, httpServiceNotAvailable, strlen(httpServiceNotAvailable)) == 0)
                {
                    Log_Debug("Service not available\n");
                    break;
                }
                if (strncmp(audioBuffer, httpOK, strlen(httpOK)) == 0)
                {
                    Log_Debug("Skip HTTP OK Response\n");
                }
                else
                {
                    for (int x = 0; x < length; x++)
                    {
                        VS1053_PlayByte(audioBuffer[x]);
                    }
                }
            }
        }

        VS1053_SetVolume(0);
        close(SockFd);
    }
    else
    {
        Log_Debug("Error initializing VS1053\n");
        return -1;
    }

    Log_Debug("Cleanup\n");
    VS1053_Cleanup();

    return 0;
}

int main(void)
{
#ifdef ENABLE_RADIO_STREAMING
    return PlayInternetRadio();
#else
    return PlayEmbeddedResource();
#endif

    return 0;
}


