// configfile.c - handles loading and saving the configuration options
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#ifdef TARGET_GX
#include <fat.h>
#ifdef __wii__
#include <wiiuse/wpad.h>
#endif
#include <ogc/pad.h>
#endif

#include "configfile.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

enum ConfigOptionType {
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_UINT,
    CONFIG_TYPE_FLOAT,
};

struct ConfigOption {
    const char *name;
    enum ConfigOptionType type;
    union {
        bool *boolValue;
        unsigned int *uintValue;
        float *floatValue;
    };
};

/*
 *Config options and default values
 */
bool configFullscreen            = false;
bool config60Fps                 = true; // 60fps interpolation mode
bool config240p                  = false; // Output a true 240p signal instead of 480i/480p
bool configAntialias             = false; // (NOTE: Only used in 240p mode) Selects the antialiased video mode variant
bool configInvertCamera          = false; // Invert camera controls
bool configRumble                = true;  // Shindou controller rumble
#ifndef TARGET_GX
// Keyboard mappings (scancode values)
unsigned int configKeyA          = 0x26;
unsigned int configKeyB          = 0x33;
unsigned int configKeyStart      = 0x39;
unsigned int configKeyR          = 0x36;
unsigned int configKeyZ          = 0x25;
unsigned int configKeyCUp        = 0x148;
unsigned int configKeyCDown      = 0x150;
unsigned int configKeyCLeft      = 0x14B;
unsigned int configKeyCRight     = 0x14D;
unsigned int configKeyStickUp    = 0x11;
unsigned int configKeyStickDown  = 0x1F;
unsigned int configKeyStickLeft  = 0x1E;
unsigned int configKeyStickRight = 0x20;
#else
#ifdef __wii__
unsigned int configKeyA          = WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A;
unsigned int configKeyB          = WPAD_BUTTON_B | WPAD_BUTTON_2 | WPAD_CLASSIC_BUTTON_B;
unsigned int configKeyStart      = WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS;
unsigned int configKeyR          = WPAD_NUNCHUK_BUTTON_C | WPAD_CLASSIC_BUTTON_FULL_R;
unsigned int configKeyZ          = WPAD_BUTTON_1 | WPAD_NUNCHUK_BUTTON_Z | WPAD_CLASSIC_BUTTON_FULL_L;
unsigned int configKeyCUp        = WPAD_BUTTON_UP;    /* cannot use WPAD_CLASSIC_BUTTON_UP as this clashes with WPAD_NUNCHUK_BUTTON_Z */
unsigned int configKeyCDown      = WPAD_BUTTON_DOWN;
unsigned int configKeyCLeft      = WPAD_BUTTON_LEFT;  /* cannot use WPAD_CLASSIC_BUTTON_LEFT as this clashes with WPAD_NUNCHUK_BUTTON_C */
unsigned int configKeyCRight     = WPAD_BUTTON_RIGHT;
unsigned int configKeyStickUp    = 0;
unsigned int configKeyStickDown  = 0;
unsigned int configKeyStickLeft  = 0;
unsigned int configKeyStickRight = 0;
#else
unsigned int configKeyA          = PAD_BUTTON_A;
unsigned int configKeyB          = PAD_BUTTON_B;
unsigned int configKeyStart      = PAD_BUTTON_START;
unsigned int configKeyL          = PAD_TRIGGER_L;
unsigned int configKeyR          = PAD_TRIGGER_R;
unsigned int configKeyZ          = PAD_TRIGGER_Z;
unsigned int configKeyCUp        = PAD_BUTTON_UP;
unsigned int configKeyCDown      = PAD_BUTTON_DOWN;
unsigned int configKeyCLeft      = PAD_BUTTON_LEFT;
unsigned int configKeyCRight     = PAD_BUTTON_RIGHT;
unsigned int configKeyStickUp    = 0;
unsigned int configKeyStickDown  = 0;
unsigned int configKeyStickLeft  = 0;
unsigned int configKeyStickRight = 0;
#endif
#endif

static const struct ConfigOption options[] = {
    {.name = "fullscreen",     .type = CONFIG_TYPE_BOOL, .boolValue = &configFullscreen},
    {.name = "60fps",          .type = CONFIG_TYPE_BOOL, .boolValue = &config60Fps},
    {.name = "240p",           .type = CONFIG_TYPE_BOOL, .boolValue = &config240p},
    {.name = "antialias",      .type = CONFIG_TYPE_BOOL, .boolValue = &configAntialias},
    {.name = "invert_camera",  .type = CONFIG_TYPE_BOOL, .boolValue = &configInvertCamera},
    {.name = "rumble",         .type = CONFIG_TYPE_BOOL, .boolValue = &configRumble},
    {.name = "key_a",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyA},
    {.name = "key_b",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyB},
    {.name = "key_start",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStart},
#ifdef __gamecube__
    {.name = "key_l",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyL},
#endif
    {.name = "key_z",          .type = CONFIG_TYPE_UINT, .uintValue = &configKeyZ},
    {.name = "key_cup",        .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCUp},
    {.name = "key_cdown",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCDown},
    {.name = "key_cleft",      .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCLeft},
    {.name = "key_cright",     .type = CONFIG_TYPE_UINT, .uintValue = &configKeyCRight},
#ifndef TARGET_GX
    {.name = "key_stickup",    .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickUp},
    {.name = "key_stickdown",  .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickDown},
    {.name = "key_stickleft",  .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickLeft},
    {.name = "key_stickright", .type = CONFIG_TYPE_UINT, .uintValue = &configKeyStickRight},
#endif
};

// Reads an entire line from a file (excluding the newline character) and returns an allocated string
// Returns NULL if no lines could be read from the file
static char *read_file_line(FILE *file) {
    char *buffer;
    size_t bufferSize = 8;
    size_t offset = 0; // offset in buffer to write

    buffer = malloc(bufferSize);
    while (1) {
        // Read a line from the file
        if (fgets(buffer + offset, bufferSize - offset, file) == NULL) {
            free(buffer);
            return NULL; // Nothing could be read.
        }
        offset = strlen(buffer);
        assert(offset > 0);

        // If a newline was found, remove the trailing newline and exit
        if (buffer[offset - 1] == '\n') {
            buffer[offset - 1] = '\0';
            break;
        }

        if (feof(file)) // EOF was reached
            break;

        // If no newline or EOF was reached, then the whole line wasn't read.
        bufferSize *= 2; // Increase buffer size
        buffer = realloc(buffer, bufferSize);
        assert(buffer != NULL);
    }

    return buffer;
}

// Returns the position of the first non-whitespace character
static char *skip_whitespace(char *str) {
    while (isspace(*str))
        str++;
    return str;
}

// NULL-terminates the current whitespace-delimited word, and returns a pointer to the next word
static char *word_split(char *str) {
    // Precondition: str must not point to whitespace
    assert(!isspace(*str));

    // Find either the next whitespace char or end of string
    while (!isspace(*str) && *str != '\0')
        str++;
    if (*str == '\0') // End of string
        return str;

    // Terminate current word
    *(str++) = '\0';

    // Skip whitespace to next word
    return skip_whitespace(str);
}

// Splits a string into words, and stores the words into the 'tokens' array
// 'maxTokens' is the length of the 'tokens' array
// Returns the number of tokens parsed
static unsigned int tokenize_string(char *str, int maxTokens, char **tokens) {
    int count = 0;

    str = skip_whitespace(str);
    while (str[0] != '\0' && count < maxTokens) {
        tokens[count] = str;
        str = word_split(str);
        count++;
    }
    return count;
}

// Loads the config file specified by 'filename'
void configfile_load(const char *filename) {
#ifdef TARGET_GX
    fatInitDefault();
#endif
    FILE *file;
    char *line;

    printf("Loading configuration from '%s'\n", filename);

    file = fopen(filename, "r");
    if (file == NULL) {
        // Create a new config file and save defaults
        printf("Config file '%s' not found. Creating it.\n", filename);
        configfile_save(filename);
        return;
    }

    // Go through each line in the file
    while ((line = read_file_line(file)) != NULL) {
        char *p = line;
        char *tokens[2];
        int numTokens;

        while (isspace(*p))
            p++;
        numTokens = tokenize_string(p, 2, tokens);
        if (numTokens != 0) {
            if (numTokens == 2) {
                const struct ConfigOption *option = NULL;

                for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
                    if (strcmp(tokens[0], options[i].name) == 0) {
                        option = &options[i];
                        break;
                    }
                }
                if (option == NULL)
                    printf("unknown option '%s'\n", tokens[0]);
                else {
                    switch (option->type) {
                        case CONFIG_TYPE_BOOL:
                            if (strcmp(tokens[1], "true") == 0)
                                *option->boolValue = true;
                            else if (strcmp(tokens[1], "false") == 0)
                                *option->boolValue = false;
                            break;
                        case CONFIG_TYPE_UINT:
                            sscanf(tokens[1], "%u", option->uintValue);
                            break;
                        case CONFIG_TYPE_FLOAT:
                            sscanf(tokens[1], "%f", option->floatValue);
                            break;
                        default:
                            assert(0); // bad type
                    }
                    printf("option: '%s', value: '%s'\n", tokens[0], tokens[1]);
                }
            } else
                puts("error: expected value");
        }
        free(line);
    }

    fclose(file);
}

// Writes the config file to 'filename'
void configfile_save(const char *filename) {
#ifdef TARGET_GX
    fatInitDefault();
#endif
    FILE *file;

    printf("Saving configuration to '%s'\n", filename);

    file = fopen(filename, "w");
    if (file == NULL) {
        // error
        return;
    }

    for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
        const struct ConfigOption *option = &options[i];

        switch (option->type) {
            case CONFIG_TYPE_BOOL:
                fprintf(file, "%s %s\n", option->name, *option->boolValue ? "true" : "false");
                break;
            case CONFIG_TYPE_UINT:
                fprintf(file, "%s %u\n", option->name, *option->uintValue);
                break;
            case CONFIG_TYPE_FLOAT:
                fprintf(file, "%s %f\n", option->name, *option->floatValue);
                break;
            default:
                assert(0); // unknown type
        }
    }

    fclose(file);
}
