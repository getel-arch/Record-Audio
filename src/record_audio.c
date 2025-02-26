#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Link with winmm.lib for multimedia functions
#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2

// Function to write the WAV file header
void writeWavHeader(HANDLE file, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    short blockAlign = channels * bitsPerSample / 8;
    DWORD written;

    // Write RIFF header
    WriteFile(file, "RIFF", 4, &written, NULL);
    int chunkSize = 36 + dataSize;
    WriteFile(file, &chunkSize, 4, &written, NULL);
    WriteFile(file, "WAVE", 4, &written, NULL);

    // Write fmt subchunk
    WriteFile(file, "fmt ", 4, &written, NULL);
    int subChunk1Size = 16;
    WriteFile(file, &subChunk1Size, 4, &written, NULL);
    short audioFormat = 1;
    WriteFile(file, &audioFormat, 2, &written, NULL);
    WriteFile(file, &channels, 2, &written, NULL);
    WriteFile(file, &sampleRate, 4, &written, NULL);
    WriteFile(file, &byteRate, 4, &written, NULL);
    WriteFile(file, &blockAlign, 2, &written, NULL);
    WriteFile(file, &bitsPerSample, 2, &written, NULL);

    // Write data subchunk
    WriteFile(file, "data", 4, &written, NULL);
    WriteFile(file, &dataSize, 4, &written, NULL);
}

// Function to clean up resources
void cleanup(HWAVEIN hWaveIn, WAVEHDR *waveHeader, HANDLE file) {
    if (hWaveIn) {
        waveInUnprepareHeader(hWaveIn, waveHeader, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
    }
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }
}

int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc != 3) {
        printf("Usage: %s <output_file_path> <duration_seconds>\n", argv[0]);
        return 1;
    }

    char outputFilePath[MAX_PATH];
    strncpy(outputFilePath, argv[1], MAX_PATH - 1);
    outputFilePath[MAX_PATH - 1] = '\0';

    // Check if the file extension is .wav
    const char *extension = strrchr(outputFilePath, '.');
    if (!extension || strcmp(extension, ".wav") != 0) {
        strncat(outputFilePath, ".wav", MAX_PATH - strlen(outputFilePath) - 1);
    }

    // Parse the duration argument
    int duration = atoi(argv[2]);
    if (duration <= 0) {
        printf("Invalid duration specified.\n");
        return 1;
    }

    // Calculate buffer size for the recording
    int bufferSize = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8 * duration;
    char *buffer = (char *)malloc(bufferSize);
    if (!buffer) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    HWAVEIN hWaveIn = NULL;
    WAVEFORMATEX waveFormat;
    WAVEHDR waveHeader;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD written;

    // Set up the wave format structure
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.nBlockAlign = CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.cbSize = 0;

    // Open the audio input device
    if (waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        printf("Error opening audio input device.\n");
        free(buffer);
        return 1;
    }

    // Prepare the wave header
    waveHeader.lpData = buffer;
    waveHeader.dwBufferLength = bufferSize;
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    if (waveInPrepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error preparing audio header.\n");
        cleanup(hWaveIn, &waveHeader, file);
        free(buffer);
        return 1;
    }

    // Add the buffer to the audio input device
    if (waveInAddBuffer(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error adding audio buffer.\n");
        cleanup(hWaveIn, &waveHeader, file);
        free(buffer);
        return 1;
    }

    // Start recording
    if (waveInStart(hWaveIn) != MMSYSERR_NOERROR) {
        printf("Error starting audio recording.\n");
        cleanup(hWaveIn, &waveHeader, file);
        free(buffer);
        return 1;
    }

    printf("Recording for %d seconds... Press Enter to stop early.\n", duration);
    Sleep(duration * 1000);

    // Stop recording
    if (waveInStop(hWaveIn) != MMSYSERR_NOERROR) {
        printf("Error stopping audio recording.\n");
        cleanup(hWaveIn, &waveHeader, file);
        free(buffer);
        return 1;
    }

    // Unprepare the wave header
    if (waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error unpreparing audio header.\n");
        cleanup(hWaveIn, &waveHeader, file);
        free(buffer);
        return 1;
    }

    // Close the audio input device
    waveInClose(hWaveIn);

    // Create the output file
    file = CreateFile(outputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        printf("Error opening output file.\n");
        free(buffer);
        return 1;
    }

    // Write the WAV file header and data
    writeWavHeader(file, SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, bufferSize);
    WriteFile(file, buffer, bufferSize, &written, NULL);
    CloseHandle(file);

    // Free the buffer
    free(buffer);

    printf("Recording saved to %s\n", outputFilePath);

    return 0;
}