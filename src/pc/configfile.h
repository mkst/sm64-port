#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#define CONFIG_FILE "sm64config.txt"

#ifdef __wii__
// Save is always located under /apps/sm64 now on Wii
#define SAVE_FILE "sm64_save_file.bin"

enum StorageDevice {
    STORAGE_DEVICE_SD  = 0,
    STORAGE_DEVICE_USB = 1,
};

extern unsigned int configStorageDevice;

const char *get_storage_path(const char *filename);

void configfile_switch_storage_device(unsigned int newDevice);
#endif

extern bool         configFullscreen;
extern bool         config60Fps;
extern bool         config240p;
extern bool         configAntialias;
extern bool         configInvertCamera;
extern bool         configRumble;
extern bool         configFog;
extern bool         configForceNearest;
extern bool         configPuppycam;
extern unsigned int puppycam_sensitivityX;
extern unsigned int puppycam_sensitivityY;
extern unsigned int puppycam_invertX;
extern unsigned int puppycam_invertY;
extern unsigned int puppycam_degrade;
extern unsigned int puppycam_aggression;
extern unsigned int puppycam_panlevel;
extern unsigned int configKeyA;
extern unsigned int configKeyB;
extern unsigned int configKeyStart;
#ifdef __gamecube__
extern unsigned int configKeyL;
#endif
extern unsigned int configKeyR;
extern unsigned int configKeyZ;
extern unsigned int configKeyCUp;
extern unsigned int configKeyCDown;
extern unsigned int configKeyCLeft;
extern unsigned int configKeyCRight;
extern unsigned int configKeyStickUp;
extern unsigned int configKeyStickDown;
extern unsigned int configKeyStickLeft;
extern unsigned int configKeyStickRight;

void configfile_load(const char *filename);
void configfile_save(const char *filename);

#endif
