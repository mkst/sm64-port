#ifndef WII_SHUTDOWN_H
#define WII_SHUTDOWN_H

#if defined(TARGET_GX) && defined(__wii__)

// Gracefully shutdown the Wii
void wii_shutdown_init(void (*save_config)(void));

// See if the Wii is supposed to start shutting down this frome
void wii_shutdown_poll(void);

#endif

#endif
