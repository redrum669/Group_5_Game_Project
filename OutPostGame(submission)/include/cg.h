#ifndef CUTSCENE_CONTROL_H
#define CUTSCENE_CONTROL_H

#include <Arduino.h>

#define CG_STAGE_COUNT   5
#define CG_TEXT_DELAY    80
#define CG_FRAME_SKIP    2

struct CutsceneData {
  int currentStage;
  uint32_t lastUpdateTime;
  bool drawRequiredFlag;
  bool finishedFlag;
  int textDisplayProgress;
};

extern struct CutsceneData cutscene;

void initCutscene(void);
bool runCutsceneUpdate(void);
void renderFullCutscene(void);
bool checkRedrawRequired(void);
void renderCutsceneProgress(void);

#endif