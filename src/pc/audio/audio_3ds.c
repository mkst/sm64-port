#ifdef TARGET_N3DS

// Must be on top to ensure that 3DS types do not redefine other types.
// Includes 3ds.h and 3ds_types.h.
#include "audio_3ds_threading.h"

#include <stdio.h>
#include <string.h>
#include "macros.h"
#include "audio_3ds.h"
#include "src/audio/external.h"

#define PLAYBACK_RATE 32000

// We synthesize 2 * SAMPLES_HIGH or LOW each frame
#ifdef VERSION_EU
#define SAMPLES_HIGH 656 // ROUND_UP_16(32000/50) + 16
#define SAMPLES_LOW 640
#define SAMPLES_DESIRED 1320
#else
#define SAMPLES_HIGH 544 // ROUND_UP_16(32000/60)
#define SAMPLES_LOW 528
#define SAMPLES_DESIRED 1100
#endif

#define N3DS_DSP_DMA_BUFFER_COUNT 4
#define N3DS_DSP_DMA_BUFFER_SIZE 4096 * 4
#define N3DS_DSP_N_CHANNELS 2

// Definitions taken from libctru comments
union NdspMix {
    float raw[12];
    struct {
        struct {
            float volume_left;
            float volume_right;
            float volume_back_left;
            float volume_back_right;
        } main;

        struct {
            float volume_left;
            float volume_right;
            float volume_back_left;
            float volume_back_right;
        } aux_0;

        struct {
            float volume_left;
            float volume_right;
            float volume_back_left;
            float volume_back_right;
        } aux_1;
    } mix;
};

// Instructions for Thread5
bool s_thread5_wait_for_audio_to_finish = true;
bool s_thread5_does_audio = false;
enum N3dsCpu s_audio_cpu = OLD_CORE_0;

volatile __3ds_s32 s_audio_frames_to_tick = 0;
volatile __3ds_s32 s_audio_frames_to_process = 0;
volatile bool s_audio_thread_processing = false;

// Synchronization variables
static volatile bool running = true;

// Statically allocate to improve performance
static s16 audio_buffer [2 * SAMPLES_HIGH * N3DS_DSP_N_CHANNELS];

// Used in synthesis.c to avoid intermediate copies when possible
size_t samples_to_copy;
int16_t* copy_buf;
int16_t* direct_buf;

static bool is_new_n3ds()
{
    bool is_new_n3ds = false;
    return R_SUCCEEDED(APT_CheckNew3DS(&is_new_n3ds)) ? is_new_n3ds : false;
}

extern void create_next_audio_buffer(s16 *samples, u32 num_samples);

static int sNextBuffer;
static volatile ndspWaveBuf sDspBuffers[N3DS_DSP_DMA_BUFFER_COUNT];
static void* sDspVAddrs[N3DS_DSP_DMA_BUFFER_COUNT];

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
    return SAMPLES_DESIRED;
}

// Returns true if the next buffer is FREE or DONE. Available in audio_3ds.h.
bool audio_3ds_next_buffer_is_ready()
{
    const u8 status = sDspBuffers[sNextBuffer].status;
    return status == NDSP_WBUF_FREE || status == NDSP_WBUF_DONE;
}

// Copies len_to_copy bytes from src to the audio buffer, then submits len_total to play. 
static void audio_3ds_play_internal(const uint8_t *src, size_t len_total, size_t len_to_copy)
{
    if (len_total > N3DS_DSP_DMA_BUFFER_SIZE)
        return;
    
    // Wait for the next audio buffer to free. This avoids discarding
    // buffers if we outrun the DSP slightly. The DSP should consume
    // buffer at a constant rate, so waiting should be ok. This
    // technically slows down synthesis slightly.
    while (!audio_3ds_next_buffer_is_ready())
        N3DS_AUDIO_SLEEP_FUNC(N3DS_AUDIO_SLEEP_DURATION_NANOS);

    // Copy the data to be played
    s16* dst = (s16*)sDspVAddrs[sNextBuffer];

    if (len_to_copy != 0)
        memcpy(dst, src, len_to_copy);

    // DSP_FlushDataCache is slow if AppCpuLimit is set high for some reason.
    // svcFlushProcessDataCache is much faster and still works perfectly.
    // DSP_FlushDataCache(dst, len_total);
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (Handle) dst, len_total);
    
    // Actually play the data
    sDspBuffers[sNextBuffer].nsamples = len_total / 4;
    sDspBuffers[sNextBuffer].status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(0, (ndspWaveBuf*) &sDspBuffers[sNextBuffer]);

    sNextBuffer = (sNextBuffer + 1) % N3DS_DSP_DMA_BUFFER_COUNT;
}

// Plays len bytes from buf in sNextBuffer, if it is available.
static void audio_3ds_play_ext(const uint8_t *buf, size_t len)
{
    if (audio_3ds_next_buffer_is_ready())
        audio_3ds_play_internal(buf, len, len);
}

inline void audio_3ds_run_one_frame() {

    // If we've buffered less than desired, SAMPLES_HIGH; else, SAMPLES_LOW
    u32 num_audio_samples = audio_3ds_buffered() < audio_3ds_get_desired_buffered() ? SAMPLES_HIGH : SAMPLES_LOW;
    s16* const direct_buf_t = (s16*) sDspVAddrs[sNextBuffer];
    samples_to_copy = 0;
    
    // Update audio state once per Thread5 frame, then let Thread5 continue
    update_game_sound_wrapper_3ds();
    AtomicDecrement(&s_audio_frames_to_tick);

    // Synthesize to our audio buffer
    for (int i = 0; i < 2; i++) {
        copy_buf = audio_buffer + i * (num_audio_samples * N3DS_DSP_N_CHANNELS);
        direct_buf = direct_buf_t + i * (num_audio_samples * N3DS_DSP_N_CHANNELS);

        create_next_audio_buffer(copy_buf, num_audio_samples);
    }

    AtomicDecrement(&s_audio_frames_to_process);

    // Play our audio buffer. If we outrun the DSP, we wait until the DSP is ready.
    audio_3ds_play_internal((u8 *)audio_buffer, N3DS_DSP_N_CHANNELS * num_audio_samples * 4, N3DS_DSP_N_CHANNELS * samples_to_copy * 4);
}

Thread threadId = NULL;

static void audio_3ds_loop(void *arg)
{
    
    while (running)
    {
        s_audio_thread_processing = s_audio_frames_to_process > 0;

        // In an ideal world, we would just continually run audio synthesis.
        // This would give an N64-like behavior, with no audio choppiness on slowdown.
        // However, due to race conditions, that is currently non-viable.
        // If the audio thread's DMA were ripped out, it would likely work.
        if (s_audio_thread_processing)
            audio_3ds_run_one_frame();
        else
            N3DS_AUDIO_SLEEP_FUNC(N3DS_AUDIO_SLEEP_DURATION_NANOS);
    }

    // Set to a negative value to ensure that the game loop does not deadlock.
    s_audio_frames_to_process = -9999;
    s_audio_frames_to_tick = -9999;
    threadId = NULL;
}


// Fully initializes the audio thread
static void audio_3ds_initialize_thread()
{
    // Start audio thread in a consistent state
    s_audio_frames_to_tick = s_audio_frames_to_process = 0;
    s_audio_thread_processing = true;

    // Set main thread priority to desired value
    if (R_SUCCEEDED(svcSetThreadPriority(CUR_THREAD_HANDLE, N3DS_DESIRED_PRIORITY_MAIN_THREAD)))
        printf("Set main thread priority to 0x%x.\n", N3DS_DESIRED_PRIORITY_MAIN_THREAD);
    else
        fprintf(stderr, "Couldn't set main thread priority to 0x%x.\n", N3DS_DESIRED_PRIORITY_MAIN_THREAD);

    // Select core to use
    if (is_new_n3ds()) {
        s_audio_cpu = NEW_CORE_2; // n3ds 3rd core
    } else if (R_SUCCEEDED(APT_SetAppCpuTimeLimit(N3DS_AUDIO_CORE_1_LIMIT))) {
        s_audio_cpu = OLD_CORE_1; // o3ds 2nd core (system)
        printf("AppCpuTimeLimit is %d.\nAppCpuIdleLimit is %d.\n", N3DS_AUDIO_CORE_1_LIMIT, N3DS_AUDIO_CORE_1_LIMIT_IDLE);
    } else {
        s_audio_cpu = OLD_CORE_0; // Run in Thread5
        fprintf(stderr, "Failed to set AppCpuTimeLimit to %d.\n", N3DS_AUDIO_CORE_1_LIMIT);
    }

    // Create a thread if applicable
    if (s_audio_cpu != OLD_CORE_0) {

        printf("Audio thread priority: 0x%x\n", N3DS_DESIRED_PRIORITY_AUDIO_THREAD);

        threadId = threadCreate(audio_3ds_loop, NULL, 64 * 1024, N3DS_DESIRED_PRIORITY_AUDIO_THREAD, s_audio_cpu, true);

        if (threadId != NULL) {
            printf("Created audio thread on core %i.\n", s_audio_cpu);

            while (s_audio_thread_processing) {
                printf("Waiting for audio thread to settle...\n");
                N3DS_AUDIO_SLEEP_FUNC(N3DS_AUDIO_MILLIS_TO_NANOS(33));
            }
            printf("Audio thread finished settling.\n");
        } else
            printf("Failed to create audio thread.\n");
    }
    
    // If thread creation failed, or was never attempted...
    if (threadId == NULL) {
        s_thread5_does_audio = true;
        s_audio_thread_processing = false;
        printf("Using Thread5 for audio.\n");
    } else {
        s_thread5_does_audio = false;
    }
}

union NdspMix ndsp_mix;

static void audio_3ds_initialize_dsp()
{
    ndspInit();

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnWaveBufClear(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, PLAYBACK_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    memset(ndsp_mix.raw, 0, sizeof(ndsp_mix));
    ndsp_mix.mix.main.volume_left = 1.0;
    ndsp_mix.mix.main.volume_right = 1.0;
    ndspChnSetMix(0, ndsp_mix.raw);

    u8* bufferData = linearAlloc(N3DS_DSP_DMA_BUFFER_SIZE * N3DS_DSP_DMA_BUFFER_COUNT);
    for (int i = 0; i < N3DS_DSP_DMA_BUFFER_COUNT; i++)
    {
        sDspVAddrs[i] = &bufferData[i * N3DS_DSP_DMA_BUFFER_SIZE];
        sDspBuffers[i].data_vaddr = &bufferData[i * N3DS_DSP_DMA_BUFFER_SIZE];
        sDspBuffers[i].nsamples = 0;
        sDspBuffers[i].status = NDSP_WBUF_FREE;
    }

    sNextBuffer = 0;
}

static bool audio_3ds_init()
{
    audio_3ds_initialize_dsp();
    audio_3ds_initialize_thread();
    return true;
}

// Stops the audio thread and waits for it to exit.
static void audio_3ds_stop(void)
{
    running = false;

    if (threadId)
        threadJoin(threadId, U64_MAX);

    ndspExit();
}

void audio_3ds_set_dsp_volume(float left, float right)
{
    ndsp_mix.mix.main.volume_left = left;
    ndsp_mix.mix.main.volume_right = right;
    ndspChnSetMix(0, ndsp_mix.raw);
}

struct AudioAPI audio_3ds =
{
    audio_3ds_init,
    audio_3ds_buffered,
    audio_3ds_get_desired_buffered,
    audio_3ds_play_ext,
    audio_3ds_stop
};

#endif
