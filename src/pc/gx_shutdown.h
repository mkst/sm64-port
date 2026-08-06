#ifndef GX_SHUTDOWN_H
#define GX_SHUTDOWN_H

#ifdef TARGET_GX

// Install the power/reset button handlers
void gx_shutdown_init(void (*save_config)(void));

// Ask to leave the game at the next safe point
void gx_shutdown_request_exit(void);

// See if the console is supposed to start shutting down this frame
void gx_shutdown_poll(void);

#endif

#endif
