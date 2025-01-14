// ---------------------------------------------------------------------
// RSDK Project: Sonic 2 Mania
// Object Description: ManiaModeMenu Object
// Object Author: AChickMcNuggie
// ---------------------------------------------------------------------

#include "ManiaModeMenu.hpp"
#include "MainMenu.hpp"
#include "SaveMenu.hpp"
#include "MenuSetup.hpp"
#include "ExtrasMenu.hpp"
#include "UILoadingIcon.hpp"
#include "UIWidgets.hpp"
#include "OptionsMenu.hpp"
#include "TimeAttackMenu.hpp"
#include "Helpers/LogHelpers.hpp"
#include "Helpers/MenuParam.hpp"
#include "Helpers/DialogRunner.hpp"
#include "Helpers/Options.hpp"
#include "Helpers/ReplayDB.hpp"
#include "Helpers/TimeAttackData.hpp"
#include "Global/Localization.hpp"
#include "Global/SaveGame.hpp"
#include "Global/Music.hpp"
#include "Common/BGSwitch.hpp"

using namespace RSDK;

namespace GameLogic
{
RSDK_REGISTER_OBJECT(ManiaModeMenu);

void ManiaModeMenu::Update() {}

void ManiaModeMenu::LateUpdate() {}

void ManiaModeMenu::StaticUpdate() {}

void ManiaModeMenu::Draw() {}

void ManiaModeMenu::Create(void *data) {}

void ManiaModeMenu::StageLoad() {}

void ManiaModeMenu::Initialize()
{
    LogHelpers::Print("ManiaModeMenu::Initialize()");

    MainMenu::Initialize();
    SaveMenu::Initialize();
    TimeAttackMenu::Initialize();
    OptionsMenu::Initialize();
    ExtrasMenu::Initialize();

    ManiaModeMenu::HandleUnlocks();
    ManiaModeMenu::SetupActions();
}

bool32 ManiaModeMenu::InitAPI()
{
    if (!MenuSetup::sVars->initializedAPI)
        MenuSetup::sVars->fxFade->timer = 512;

    int32 authStatus = API::Auth::GetUserAuthStatus();
    if (!authStatus) {
        API::Auth::TryAuth();
    }
    else if (authStatus != STATUS_CONTINUE) {
        int32 storageStatus = APITable->GetStorageStatus();
        if (!storageStatus) {
            API::Storage::TryInitStorage();
        }
        else if (storageStatus != STATUS_CONTINUE) {
            int32 saveStatus = API::Storage::GetSaveStatus();

            if (!API::Storage::GetNoSave() && (authStatus != STATUS_OK || storageStatus != STATUS_OK)) {
                if (saveStatus != STATUS_CONTINUE) {
                    if (saveStatus != STATUS_FORBIDDEN) {
                        DialogRunner::PromptSavePreference(storageStatus);
                    }
                    else {
                        Stage::SetScene("Presentation", "Title Screen");
                        Stage::LoadScene();
                    }
                }

                return false;
            }

            if (!MenuSetup::sVars->initializedSaves) {
                UILoadingIcon::StartWait();
                Options::LoadOptionsBin();
                SaveGame::LoadFile(&SaveGame::SaveLoadedCB);
                ReplayDB::LoadDB(&ReplayDB::LoadCallback);

                MenuSetup::sVars->initializedSaves = true;
            }

            if (MenuSetup::sVars->initializedAPI)
                return true;

            if (globals->optionsLoaded == STATUS_OK && globals->saveLoaded == STATUS_OK && globals->replayTableLoaded == STATUS_OK
                && globals->taTableLoaded == STATUS_OK) {

                if (!API::Storage::GetNoSave() && DialogRunner::NotifyAutosave())
                    return false;

                UILoadingIcon::FinishWait();
                if (DialogRunner::CheckUnreadNotifs())
                    return false;

                MenuSetup::sVars->initializedAPI = true;
                return true;
            }

            if (API::Storage::GetNoSave()) {
                UILoadingIcon::FinishWait();
                return true;
            }
            else {
                if (globals->optionsLoaded == STATUS_ERROR || globals->saveLoaded == STATUS_ERROR || globals->replayTableLoaded == STATUS_ERROR
                    || globals->taTableLoaded == STATUS_ERROR) {
                    int32 status = API::Storage::GetSaveStatus();

                    if (status != STATUS_CONTINUE) {
                        if (status == STATUS_FORBIDDEN) {
                            Stage::SetScene("Presentation", "Title Screen");
                            Stage::LoadScene();
                        }
                        else {
                            DialogRunner::PromptSavePreference(STATUS_CORRUPT);
                        }
                    }
                }
            }
        }
    }

    return false;
}

void ManiaModeMenu::InitLocalization(bool32 success)
{
    if (success) {
        Localization::sVars->loaded = false;

        Localization::LoadStrings();
        UIWidgets::ApplyLanguage();
    }
}

int32 ManiaModeMenu::GetActiveMenu()
{
    UIControl *control = UIControl::GetUIControl();

    if (control == MainMenu::sVars->menuControl || control == ExtrasMenu::sVars->extrasControl) {
        return MenuSetup::Main;
    }

    if (control == TimeAttackMenu::sVars->timeAttackControl || control == TimeAttackMenu::sVars->taDetailsControl) {
        return MenuSetup::TimeAttackMain;
    }

    if (control == TimeAttackMenu::sVars->taZoneSelControl || control == TimeAttackMenu::sVars->replaysControl) {
        return MenuSetup::TimeAttackElse;
    }

    if (control == ManiaModeMenu::sVars->saveSelectMenu || control == ManiaModeMenu::sVars->noSaveMenu
        || control == ManiaModeMenu::sVars->secretsMenu) {
        return MenuSetup::SaveSelect;
    }

    if (control == OptionsMenu::sVars->optionsControl || control == OptionsMenu::sVars->dataOptionsControl) {
        return MenuSetup::OptionsMain;
    }

    if (control == OptionsMenu::sVars->videoControl_Windows || control == OptionsMenu::sVars->soundControl
        || control == OptionsMenu::sVars->controlsControl_Windows || control == OptionsMenu::sVars->controlsControl_KB
        || control == OptionsMenu::sVars->controlsControl_PS4 || control == OptionsMenu::sVars->controlsControl_XB1
        || control == OptionsMenu::sVars->controlsControl_NX || control == OptionsMenu::sVars->controlsControl_NXGrip
        || control == OptionsMenu::sVars->controlsControl_NXJoycon || control == OptionsMenu::sVars->controlsControl_NXPro) {
        return MenuSetup::OptionsElse;
    }

    return MenuSetup::Main;
}

void ManiaModeMenu::ChangeMenuTrack()
{
    int32 trackID = 0;

    switch (ManiaModeMenu::GetActiveMenu()) {
        default:
        case MenuSetup::Main: trackID = 0; break;
        case MenuSetup::TimeAttackMain:;
        case MenuSetup::TimeAttackElse: trackID = 2; break;
        case MenuSetup::SaveSelect: trackID = 1; break;
        case MenuSetup::OptionsMain:;
        case MenuSetup::OptionsElse: trackID = 0; break;
    }

    if (!Music::IsPlaying())
        Music::PlayTrack(trackID);
    else if (Music::sVars->activeTrack != trackID)
        Music::PlayOnFade(trackID, 0.12f);
}

void ManiaModeMenu::ChangeMenuBG()
{   
    switch (ManiaModeMenu::GetActiveMenu()) {
        default:
        case MenuSetup::Main:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            break;
        case MenuSetup::TimeAttackMain:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            break;
        case MenuSetup::TimeAttackElse:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = 2;
            break;
        case MenuSetup::SaveSelect:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = 2;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            break;
        case MenuSetup::OptionsMain:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = 2;
            break;
        case MenuSetup::OptionsElse:
            RSDKTable->GetTileLayer(1)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(2)->drawGroup[BGSwitch::sVars->screenID] = 0;
            RSDKTable->GetTileLayer(3)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(4)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(5)->drawGroup[BGSwitch::sVars->screenID] = 1;
            RSDKTable->GetTileLayer(6)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            RSDKTable->GetTileLayer(7)->drawGroup[BGSwitch::sVars->screenID] = DRAWGROUP_COUNT;
            break;
    }
}

void ManiaModeMenu::StartReturnToTitle()
{
    UIControl *control = UIControl::GetUIControl();
    if (control)
        control->state.Set(nullptr);

    Music::FadeOut(0.05f);
    MenuSetup::StartTransition(ManiaModeMenu::ReturnToTitle, 32);
}

void ManiaModeMenu::ReturnToTitle()
{
    TimeAttackData::Clear();

    Stage::SetScene("Presentation", "Title Screen");
    Stage::LoadScene();
}

void ManiaModeMenu::State_HandleTransition()
{
    MenuSetup *menuSetup = (MenuSetup *)this;
    menuSetup->fadeTimer = CLAMP(menuSetup->timer << ((menuSetup->fadeShift & 0xFF) - 1), 0, 0x200);
}

void ManiaModeMenu::HandleUnlocks()
{
    MainMenu::HandleUnlocks();
    SaveMenu::HandleUnlocks();
    TimeAttackMenu::HandleUnlocks();
    OptionsMenu::HandleUnlocks();
    ExtrasMenu::HandleUnlocks();
}

void ManiaModeMenu::SetupActions()
{
    MainMenu::SetupActions();
    SaveMenu::SetupActions();
    TimeAttackMenu::SetupActions();
    OptionsMenu::SetupActions();
    ExtrasMenu::SetupActions();
}

void ManiaModeMenu::HandleMenuReturn()
{
    MenuParam *param = MenuParam::GetMenuParam();

    char buffer[0x100];
    memset(buffer, 0, 0x100);
    for (auto control : GameObject::GetEntities<UIControl>(FOR_ALL_ENTITIES)) {
        if (strcmp(param->menuTag, "") != 0) {
            control->tag.CStr(buffer);

            if (strcmp((const char *)buffer, param->menuTag) != 0) {
                control->SetInactiveMenu(control);
            }
            else {
                control->storedButtonID  = param->menuSelection;
                control->hasStoredButton = true;
                UIControl::SetActiveMenu(control);
                control->buttonID = param->menuSelection;
            }
        }
    }

    SaveMenu::HandleMenuReturn(0);
    TimeAttackMenu::HandleMenuReturn();
    OptionsMenu::HandleMenuReturn();

    int32 zoneID = 0, actID = 0, characterID = 0;
    bool32 inTimeAttack = param->inTimeAttack;
    if (inTimeAttack) {
        characterID = param->characterID;
        zoneID      = param->zoneID;
        actID       = param->actID;
    }

    TimeAttackData::Clear();

    if (inTimeAttack) {
        param->characterID = characterID;
        param->zoneID      = zoneID;
        param->actID       = actID;
    }
}

void ManiaModeMenu::MovePromptCB()
{
    auto *control = (UIControl *)this;
    control->ProcessButtonInput();
    for (auto prompt : GameObject::GetEntities<UIButtonPrompt>(FOR_ACTIVE_ENTITIES)) {
        if (prompt->parent == this)
            prompt->position.y = prompt->startPos.y - control->startPos.y + control->position.y
                                 + TO_FIXED(control->promptOffset); // idk why the offset, maybe ask skye to move it
    }
}

#if RETRO_INCLUDE_EDITOR
void ManiaModeMenu::EditorDraw() {}

void ManiaModeMenu::EditorLoad() {}
#endif

void ManiaModeMenu::Serialize() {}
} // namespace GameLogic
