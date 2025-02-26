#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2
#define BUFFER_SIZE (SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8)

void writeWavHeader(HANDLE file, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    short blockAlign = channels * bitsPerSample / 8;
    DWORD written;

    WriteFile(file, "RIFF", 4, &written, NULL);
    int chunkSize = 36 + dataSize;
    WriteFile(file, &chunkSize, 4, &written, NULL);
    WriteFile(file, "WAVE", 4, &written, NULL);
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
    WriteFile(file, "data", 4, &written, NULL);
    WriteFile(file, &dataSize, 4, &written, NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <output_file_path>\n", argv[0]);
        return 1;
    }

    HWAVEIN hWaveIn;
    WAVEFORMATEX waveFormat;
    WAVEHDR waveHeader;
    HANDLE file;
    char buffer[BUFFER_SIZE];
    DWORD written;

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.nBlockAlign = CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.cbSize = 0;

    if (waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        printf("Error opening audio input device.\n");
        return 1;
    }

    waveHeader.lpData = buffer;
    waveHeader.dwBufferLength = BUFFER_SIZE;
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    if (waveInPrepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error preparing audio header.\n");
        return 1;
    }

    if (waveInAddBuffer(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error adding audio buffer.\n");
        return 1;
    }

    if (waveInStart(hWaveIn) != MMSYSERR_NOERROR) {
        printf("Error starting audio recording.\n");
        return 1;
    }

    printf("Recording... Press Enter to stop.\n");
    getchar();

    if (waveInStop(hWaveIn) != MMSYSERR_NOERROR) {
        printf("Error stopping audio recording.\n");
        return 1;
    }

    if (waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        printf("Error unpreparing audio header.\n");
        return 1;
    }

    waveInClose(hWaveIn);

    file = CreateFile(argv[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        printf("Error opening output file.\n");
        return 1;
    }

    writeWavHeader(file, SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, BUFFER_SIZE);
    WriteFile(file, buffer, BUFFER_SIZE, &written, NULL);
    CloseHandle(file);

    printf("Recording saved to %s\n", argv[1]);

    return 0;
}