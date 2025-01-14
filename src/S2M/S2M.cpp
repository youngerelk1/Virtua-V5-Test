#include "S2M.hpp"
#include "Helpers/RPCHelpers.hpp"

using namespace RSDK;

#if RETRO_USE_MOD_LOADER
extern "C" {
DLLExport bool32 LinkModLogic(EngineInfo *info, const char *id);
}
#endif

GlobalVariables::Constructor c;
GlobalVariables *globals = nullptr;

void GlobalVariables::Init(void *g)
{
    GlobalVariables *globals = (GlobalVariables *)g;
    memset(globals, 0, sizeof(GlobalVariables));

    // Global Variable Initialization Goes Here!

    globals->playerID       = ID_SONIC | (ID_TAILS << 8);
    globals->saveSlotID     = NO_SAVE_SLOT;
    globals->medalMods      = MEDAL_INSTASHIELD + MEDAL_PEELOUT;
    globals->stock          = (ID_KNUCKLES << 8) | ID_TAILS;
    globals->characterFlags = ID_SONIC | ID_TAILS | ID_KNUCKLES;

    globals->superMusicEnabled = true;
    globals->playerSpriteStyle = GAME_SM;
    globals->gameSpriteStyle   = GAME_SM;
    globals->ostStyle          = GAME_SM;
    globals->starpostStyle     = GAME_SM;

    globals->restart1UP      = 100;
    globals->restartScore1UP = 5000;
    globals->restartLives[0] = 3;
    globals->restartLives[1] = 3;
    globals->restartLives[2] = 3;
    globals->restartLives[3] = 3;

    globals->useManiaBehavior  = true;

    globals->gravityDir        = CMODE_FLOOR;
    globals->tileCollisionMode = TILECOLLISION_DOWN;

    InitDiscord(); // initializes the discord core at startup
}

void InitModAPI(void) {  }

#if RETRO_USE_MOD_LOADER
#define ADD_PUBLIC_FUNC(func) Mod.AddPublicFunction(#func, (void *)(func))

void InitModAPI(void);

bool32 LinkModLogic(EngineInfo *info, const char *id)
{
#if !RETRO_REV01
    LinkGameLogicDLL(info);
#else
    LinkGameLogicDLL(*info);
#endif

    InitModAPI();

    return true;
}
#endif