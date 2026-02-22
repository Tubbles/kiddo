#include "audio.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100

static Sound sounds[SOUND_COUNT];

static Wave generate_tone(float freq, float duration, float volume)
{
    int sample_count = (int)(SAMPLE_RATE * duration);
    short *data = malloc(sizeof(short) * sample_count);

    for (int i = 0; i < sample_count; i++) {
        float t = (float)i / SAMPLE_RATE;
        float envelope = 1.0f - t / duration;
        float sample = sinf(2.0f * PI * freq * t) * envelope * volume;
        data[i] = (short)(sample * 32767.0f);
    }

    Wave wave = {
        .frameCount = sample_count,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = data,
    };
    return wave;
}

void audio_init(void)
{
    InitAudioDevice();

    Wave button_wave = generate_tone(440.0f, 0.15f, 0.3f);
    sounds[SOUND_BUTTON] = LoadSoundFromWave(button_wave);
    free(button_wave.data);

    Wave collision_wave = generate_tone(220.0f, 0.25f, 0.4f);
    sounds[SOUND_COLLISION] = LoadSoundFromWave(collision_wave);
    free(collision_wave.data);
}

void audio_play(SoundKind kind)
{
    if (kind >= 0 && kind < SOUND_COUNT)
        PlaySound(sounds[kind]);
}

void audio_shutdown(void)
{
    for (int i = 0; i < SOUND_COUNT; i++)
        UnloadSound(sounds[i]);
    CloseAudioDevice();
}
