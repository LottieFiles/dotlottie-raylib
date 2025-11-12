#ifndef __DLRL_H__
#define __DLRL_H__

#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dlrl_Player dlrl_Player;

typedef enum {
  DLRL_FIT_CONTAIN,
  DLRL_FIT_COVER,
  DLRL_FIT_FILL,
  DLRL_FIT_FIT_WIDTH,
  DLRL_FIT_FIT_HEIGHT
} dlrl_Fit;

typedef enum {
  DLRL_MODE_FORWARD,
  DLRL_MODE_REVERSE,
  DLRL_MODE_BOUNCE,
  DLRL_MODE_REVERSE_BOUNCE
} dlrl_Mode;

typedef struct {
  int       width;        
  int       height;       
  float     speed;        
  bool      loop;         
  dlrl_Mode mode;         
  bool      interpolate;  
  dlrl_Fit  fit;          
  Vector2   align;        
  Color     background;   
  
  const char* animation_id;     
  const char* theme_id;         
  const char* state_machine_id; 
  const char* marker;           
} dlrl_Config;

bool          dlrl_Init(void);         
void          dlrl_Shutdown(void);

dlrl_Player*  dlrl_LoadDotLottieFile(const char* path, const dlrl_Config* cfg);
dlrl_Player*  dlrl_LoadLottieJSON(const char* json, size_t len, const dlrl_Config* cfg);
void          dlrl_Unload(dlrl_Player* p);

void          dlrl_Play(dlrl_Player* p);
void          dlrl_Pause(dlrl_Player* p);
void          dlrl_Stop(dlrl_Player* p);
bool          dlrl_IsPlaying(const dlrl_Player* p);

void          dlrl_SetSpeed(dlrl_Player* p, float speed);
void          dlrl_SetLoop(dlrl_Player* p, bool loop);
void          dlrl_SetMode(dlrl_Player* p, dlrl_Mode mode);

void          dlrl_Update(dlrl_Player* p, float dt_seconds); 
void          dlrl_Draw(const dlrl_Player* p, Rectangle dest, float rotation, Color tint);

float         dlrl_Duration(const dlrl_Player* p);     
float         dlrl_CurrentTime(const dlrl_Player* p);  
int           dlrl_TotalFrames(const dlrl_Player* p);
int           dlrl_CurrentFrame(const dlrl_Player* p);
Vector2       dlrl_NaturalSize(const dlrl_Player* p);  
Texture2D     dlrl_GetTexture(const dlrl_Player* p);   

bool          dlrl_SetTheme(dlrl_Player* p, const char* theme_id);
bool          dlrl_SetAnimation(dlrl_Player* p, const char* animation_id);
bool          dlrl_SetMarker(dlrl_Player* p, const char* marker_name); 
int           dlrl_MarkerCount(const dlrl_Player* p);
const char*   dlrl_MarkerName(const dlrl_Player* p, int index);

#endif
