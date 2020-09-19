#ifdef TARGET_N3DS

#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "macros.h"
#include "audio_3ds.h"
#include "audio_3ds_threading.h"

#ifdef VERSION_EU
#define SAMPLES_HIGH 656
#define SAMPLES_LOW 640
#else
#define SAMPLES_HIGH 544
#define SAMPLES_LOW 528
#endif

#define N3DS_DSP_DMA_BUFFER_COUNT   4

static bool is_new_n3ds()
{
    bool is_new_n3ds = false;
    return R_SUCCEEDED(APT_CheckNew3DS(&is_new_n3ds)) ? is_new_n3ds : false;
}

extern void create_next_audio_buffer(s16 *samples, u32 num_samples);

static int sNextBuffer;
static ndspWaveBuf sDspBuffers[N3DS_DSP_DMA_BUFFER_COUNT];

static int audio_3ds_buffered(void)
{
    int total = 0;
    for (int i = 0; i < N3DS_DSP_DMA_BUFFER_COUNT; i++)
    {
        if (sDspBuffers[i].status == NDSP_WBUF_QUEUED ||
            sDspBuffers[i].status == NDSP_WBUF_PLAYING)
            total += sDspBuffers[i].nsamples;
    }
    return total;
}

static int audio_3ds_get_desired_buffered(void)
{
    return 1100;
}

static void audio_3ds_play(const uint8_t *buf, size_t len)
{
    if (len > 4096 * 4)
        return;
    if (sDspBuffers[sNextBuffer].status != NDSP_WBUF_FREE &&
        sDspBuffers[sNextBuffer].status != NDSP_WBUF_DONE)
        return;
    sDspBuffers[sNextBuffer].nsamples = len / 4;
    sDspBuffers[sNextBuffer].status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(0, &sDspBuffers[sNextBuffer]);

    s16* dst = (s16*)sDspBuffers[sNextBuffer].data_vaddr;
    memcpy(dst, buf, len);
    DSP_FlushDataCache(dst, len);

    sNextBuffer = (sNextBuffer + 1) % N3DS_DSP_DMA_BUFFER_COUNT;
}

static bool running = true;
LightEvent s_event_audio, s_event_main;

static void audio_3ds_loop()
{
    while (running)
    {
        LightEvent_Wait(&s_event_audio);
        if (!running)
            break;
        u32 num_audio_samples = audio_3ds_buffered() < audio_3ds_get_desired_buffered() ? SAMPLES_HIGH : SAMPLES_LOW;
        s16 audio_buffer[SAMPLES_HIGH * 2 * 2];
        for (int i = 0; i < 2; i++) {
            create_next_audio_buffer(audio_buffer + i * (num_audio_samples * 2), num_audio_samples);
        }
        audio_3ds_play((u8 *)audio_buffer, 2 * num_audio_samples * 4);
        LightEvent_Signal(&s_event_main);
    }
}

Thread threadId;

static bool audio_3ds_init()
{
    ndspInit();

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnWaveBufClear(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 32000);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0;
    mix[1] = 1.0;
    ndspChnSetMix(0, mix);

    u8* bufferData = linearAlloc(4096 * 4 * N3DS_DSP_DMA_BUFFER_COUNT);
    for (int i = 0; i < N3DS_DSP_DMA_BUFFER_COUNT; i++)
    {
        sDspBuffers[i].data_vaddr = &bufferData[i * 4096 * 4];
        sDspBuffers[i].nsamples = 0;
        sDspBuffers[i].status = NDSP_WBUF_FREE;
    }

    sNextBuffer = 0;

    LightEvent_Init(&s_event_audio, RESET_ONESHOT);
    LightEvent_Init(&s_event_main, RESET_ONESHOT);

    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

    int cpu;
    if (is_new_n3ds())
        cpu = 2; // n3ds 3rd core
    else if (R_SUCCEEDED(APT_SetAppCpuTimeLimit(80)))
        cpu = 1; // o3ds 2nd core (system)
    else
        cpu = 0; // better to have choppy sound than no sound?

    threadId = threadCreate(audio_3ds_loop, 0, 128 * 1024, prio - 1, cpu, true);

    if (threadId)
        printf("Created audio thread on core %i\n", cpu);
    else
        printf("Failed to create audio thread\n");

    return threadId != NULL;
}

static void audio_3ds_stop(void)
{
    running = false;
    LightEvent_Signal(&s_event_audio);

    if (threadId)
        threadJoin(threadId, U64_MAX);
    ndspExit();
}

struct AudioAPI audio_3ds =
{
    audio_3ds_init,
    audio_3ds_buffered,
    audio_3ds_get_desired_buffered,
    audio_3ds_play,
    audio_3ds_stop
};

#endif
