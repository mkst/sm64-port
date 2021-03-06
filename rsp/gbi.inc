// GBI: Graphics Binary Interface

G_FOG                  equ 0x00010000
G_LIGHTING             equ 0x00020000
G_TEXTURE_GEN          equ 0x00040000
G_TEXTURE_GEN_LINEAR   equ 0x00080000

G_FOG_H                equ (G_FOG/0x10000)
G_LIGHTING_H           equ (G_LIGHTING/0x10000)
G_TEXTURE_GEN_H        equ (G_TEXTURE_GEN/0x10000)
G_TEXTURE_GEN_LINEAR_H equ(G_TEXTURE_GEN_LINEAR/0x10000)


G_CULL_BACK  equ 0x00002000
G_CULL_FRONT equ 0x00001000

G_ZBUFFER        equ 0x00000001
G_TEXTURE_ENABLE equ 0x00000002
G_SHADE          equ 0x00000004
