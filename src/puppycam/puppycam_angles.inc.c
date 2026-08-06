///This is the bit that defines where the angles happen. They're basically environment boxes that dictate camera behaviour.
///Permaswap is a boolean that simply determines wether or not when the camera changes at this point it stays changed. 0 means it resets when you leave, and 1 means it stays changed.
///The camera position fields accept "32767" as an ignore flag.
///The script supports anything that does not take an argument. It's reccomended to keep the scripts in puppycam_scripts.inc.c for the sake of cleanliness.
///If you do not wish to use a script in the angle, then just leave the field as 0.
///All these are sample camera angles that are designed for the base game of SM64.
struct newcam_hardpos newcam_fixedcam[] =
{

///Example camera angle. This points the camera towards the door outside the castle on the bridge.
{
/*Level ID*/  .newcam_hard_levelID   =  16,
/*Area ID*/   .newcam_hard_areaID    =  1,
/*Permaswap*/ .newcam_hard_permaswap =  0,
/*Mode*/      .newcam_hard_modeset   = NC_MODE_FIXED_NOMOVE,
/*Script*/    .newcam_hard_script    =  0, //Standard params.
/*X begin*/   .newcam_hard_X1        = -540,
/*Y begin*/   .newcam_hard_Y1        =  800,
/*Z begin*/   .newcam_hard_Z1        = -3500,  //Where the activation box begins
/*X end*/     .newcam_hard_X2        =  540,
/*Y end*/     .newcam_hard_Y2        =  2000,
/*Z end*/     .newcam_hard_Z2        = -1500,  //Where the activation box ends.
/*Cam X*/     .newcam_hard_camX      =  0,
/*Cam Y*/     .newcam_hard_camY      =  1500,
/*Cam Z*/     .newcam_hard_camZ      = -1000,  //The position the camera gets placed for NC_MODE_FIXED and NC_MODE_FIXED_NOMOVE
/*Look X*/    .newcam_hard_lookX     =  0,
/*Look Y*/    .newcam_hard_lookY     =  800,
/*Look Z*/    .newcam_hard_lookZ     = -2500   //The position the camera looks at for NC_MODE_FIXED_NOMOVE
},

///Another example angle. This activates a script that slowly rotates the camera around the area.
{
/*Level ID*/  .newcam_hard_levelID   =  16,
/*Area ID*/   .newcam_hard_areaID    =  1,
/*Permaswap*/ .newcam_hard_permaswap =  0,
/*Mode*/      .newcam_hard_modeset   = NC_MODE_NOROTATE,
/*Script*/    .newcam_hard_script    = &newcam_angle_rotate, //Standard params.
/*X begin*/   .newcam_hard_X1        =  5716,
/*Y begin*/   .newcam_hard_Y1        =  400,
/*Z begin*/   .newcam_hard_Z1        = -859,    //Where the activation box begins
/*X end*/     .newcam_hard_X2        =  6908,
/*Y end*/     .newcam_hard_Y2        =  1000,
/*Z end*/     .newcam_hard_Z2        =  62,     //Where the activation box ends.
/*Cam X*/     .newcam_hard_camX      =  32767,
/*Cam Y*/     .newcam_hard_camY      =  32767,
/*Cam Z*/     .newcam_hard_camZ      =  32767,  //The position the camera gets placed for NC_MODE_FIXED and NC_MODE_FIXED_NOMOVE
/*Look X*/    .newcam_hard_lookX     =  32767,
/*Look Y*/    .newcam_hard_lookY     =  32767,
/*Look Z*/    .newcam_hard_lookZ     =  32767   //The position the camera looks at for NC_MODE_FIXED_NOMOVE
},
};
