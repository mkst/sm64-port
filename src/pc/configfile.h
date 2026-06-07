#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#define CONFIG_FILE "sm64config.txt"

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
