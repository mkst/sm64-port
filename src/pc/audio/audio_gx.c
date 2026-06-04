#ifdef TARGET_GX

#include <malloc.h>
#include <string.h>

#include <asndlib.h>
#include <ogc/cache.h>
#include <ogc/irq.h>

#include "macros.h"
#include "audio_api.h"

#ifdef VERSION_EU
#define SAMPLES_HIGH 656
#else
#define SAMPLES_HIGH 544
#endif

#define BUFFER_COUNT 6
#define BUFFER_SIZE (SAMPLES_HIGH * 2 * 2 * sizeof(int16_t))
#define BYTES_PER_FRAME (2 * sizeof(int16_t))

enum AudioBufferState {
    BUFFER_FREE,
    BUFFER_FILLING,
    BUFFER_QUEUED,
    BUFFER_ASND,
};

static void voice_callback(int32_t voice);

static int16_t *audio_buffer[BUFFER_COUNT];
static size_t audio_buffer_len[BUFFER_COUNT];
static int audio_buffer_frames[BUFFER_COUNT];
static enum AudioBufferState audio_buffer_state[BUFFER_COUNT];
static int queued_buffers[BUFFER_COUNT];
static uint8_t queue_read;
static uint8_t queue_write;
static uint8_t queue_count;
static bool voice_started;
static bool feed_in_progress;
static int buffered_frames;

static int pop_queued_buffer(void)
{
    int index = -1;
    u32 level = IRQ_Disable();

    if (queue_count > 0) {
        index = queued_buffers[queue_read];
        queue_read = (queue_read + 1) % BUFFER_COUNT;
        queue_count--;
        audio_buffer_state[index] = BUFFER_ASND;
    }

    IRQ_Restore(level);
    return index;
}

static void push_queued_buffer_front(int index)
{
    u32 level = IRQ_Disable();

    queue_read = (queue_read + BUFFER_COUNT - 1) % BUFFER_COUNT;
    queued_buffers[queue_read] = index;
    queue_count++;
    audio_buffer_state[index] = BUFFER_QUEUED;

    IRQ_Restore(level);
}

static bool begin_feed(void)
{
    bool can_feed = false;
    u32 level = IRQ_Disable();

    if (!feed_in_progress) {
        feed_in_progress = true;
        can_feed = true;
    }

    IRQ_Restore(level);
    return can_feed;
}

static void end_feed(void)
{
    u32 level = IRQ_Disable();
    feed_in_progress = false;
    IRQ_Restore(level);
}

static void release_finished_buffers(void)
{
    u32 level = IRQ_Disable();

    if (voice_started && ASND_StatusVoice(0) == SND_UNUSED) {
        voice_started = false;
    }

    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (audio_buffer_state[i] != BUFFER_ASND) {
            continue;
        }

        if (!voice_started || ASND_TestPointer(0, audio_buffer[i]) == 0) {
            audio_buffer_state[i] = BUFFER_FREE;
            buffered_frames -= audio_buffer_frames[i];
            audio_buffer_len[i] = 0;
            audio_buffer_frames[i] = 0;
        }
    }

    if (buffered_frames < 0) {
        buffered_frames = 0;
    }

    IRQ_Restore(level);
}

static void feed_queued_buffers(int32_t voice)
{
    if (!begin_feed()) {
        return;
    }

    if (!voice_started) {
        int index = pop_queued_buffer();
        if (index >= 0) {
            s32 result = ASND_SetVoice(voice, VOICE_STEREO_16BIT, 32000, 0, audio_buffer[index],
                                      audio_buffer_len[index], MAX_VOLUME, MAX_VOLUME, voice_callback);
            if (result == SND_OK) {
                voice_started = true;
            } else {
                push_queued_buffer_front(index);
            }
        }
    }

    while (voice_started && ASND_TestVoiceBufferReady(voice) == 1) {
        int index = pop_queued_buffer();
        if (index < 0) {
            break;
        }

        s32 result = ASND_AddVoice(voice, audio_buffer[index], audio_buffer_len[index]);
        if (result == SND_OK) {
            continue;
        }

        push_queued_buffer_front(index);
        if (result == SND_INVALID) {
            voice_started = false;
        }
        break;
    }

    end_feed();
}

static void voice_callback(int32_t voice)
{
    feed_queued_buffers(voice);
}

static bool audio_gx_init(void)
{
    queue_read = 0;
    queue_write = 0;
    queue_count = 0;
    voice_started = false;
    feed_in_progress = false;
    buffered_frames = 0;

    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        audio_buffer[i] = (int16_t *) memalign(32, BUFFER_SIZE);
        if (audio_buffer[i] == NULL) {
            return false;
        }
        memset(audio_buffer[i], 0, BUFFER_SIZE);
        DCFlushRange(audio_buffer[i], BUFFER_SIZE);
        audio_buffer_len[i] = 0;
        audio_buffer_frames[i] = 0;
        audio_buffer_state[i] = BUFFER_FREE;
    }

    ASND_Init();
    ASND_Pause(0);

    return true;
}

static int audio_gx_buffered(void)
{
    int result;

    release_finished_buffers();

    u32 level = IRQ_Disable();
    result = buffered_frames;
    IRQ_Restore(level);

    return result;
}

static int audio_gx_get_desired_buffered(void)
{
    return 1100;
}

static void audio_gx_play(const uint8_t *buf, size_t len)
{
    if (len > BUFFER_SIZE)
        return;

    release_finished_buffers();

    int index = -1;
    u32 level = IRQ_Disable();

    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (audio_buffer_state[i] == BUFFER_FREE) {
            index = i;
            audio_buffer_state[i] = BUFFER_FILLING;
            break;
        }
    }

    IRQ_Restore(level);

    if (index < 0) {
        return;
    }

    memcpy(audio_buffer[index], buf, len);
    DCFlushRange(audio_buffer[index], len);

    level = IRQ_Disable();
    queued_buffers[queue_write] = index;
    queue_write = (queue_write + 1) % BUFFER_COUNT;
    queue_count++;
    audio_buffer_len[index] = len;
    audio_buffer_frames[index] = len / BYTES_PER_FRAME;
    buffered_frames += audio_buffer_frames[index];
    audio_buffer_state[index] = BUFFER_QUEUED;
    IRQ_Restore(level);

    feed_queued_buffers(0);
}

struct AudioAPI audio_gx =
{
    audio_gx_init,
    audio_gx_buffered,
    audio_gx_get_desired_buffered,
    audio_gx_play
};

#endif
