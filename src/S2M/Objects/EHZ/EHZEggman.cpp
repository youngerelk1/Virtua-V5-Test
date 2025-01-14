// ---------------------------------------------------------------------
// RSDK Project: Sonic 2 Mania
// Object Description: EHZEggman Object
// Object Author: AChickMcNuggie
// ---------------------------------------------------------------------

#include "EHZEggman.hpp"
#include "EggDriller.hpp"
#include "Global/Zone.hpp"
#include "Global/Camera.hpp"
#include "Global/Player.hpp"
#include "Global/Explosion.hpp"
#include "Global/Music.hpp"
#include "Helpers/BadnikHelpers.hpp"

using namespace RSDK;

namespace GameLogic
{
RSDK_REGISTER_OBJECT(EHZEggman);

void EHZEggman::Update()
{
    if (this->health != 0) {
        if (this->invincibilityTimer > 0) {
            this->invincibilityTimer--;
            int32 invincible =
                GET_BIT(this->invincibilityTimer, 0); // assuming this stops the palette from flashing again when the invincility timer is still up
            if (invincible) {                         // Palette flashing when hit
                paletteBank[0].SetEntry(189, 0xE0E0E0);
            }
            else {
                paletteBank[0].SetEntry(189, 0x000000);
            }
        }

        for (auto currentPlayer : GameObject::GetEntities<Player>(FOR_ACTIVE_ENTITIES)) {
            if (this->invincibilityTimer == 0) { // Invulnerable?
                if (currentPlayer->animator.animationID == Player::ANI_HURT || currentPlayer->animator.animationID == Player::ANI_DIE
                    || currentPlayer->animator.animationID == Player::ANI_DROWN) {
                    if (this->eggmanAnimator.animationID != Laugh) {
                        this->eggmanAnimator.SetAnimation(sVars->aniFrames, Laugh, false, 0);
                    }

                    if (currentPlayer == 0) {
                        this->didHitPlayer = true;
                    }
                }

                // review
                if (currentPlayer->CheckBadnikTouch(this, &sVars->hitbox)
                    && currentPlayer->CheckBossHit(
                        this)) { // this hitbox only becomes active when the health isnt 0, it gets set to 8 in the startcar state
                    this->health--;
                    if (this->health == 0) {
                        currentPlayer->score += 1000;
                        this->eggmanAnimator.SetAnimation(sVars->aniFrames, Toasted, false, 0);
                        this->exploding = true;
                        this->state.Set(&EHZEggman::State_Explode); // this->state++;

                        // The Car!!!!
                        for (auto driller : GameObject::GetEntities<EggDriller>(
                                 FOR_ALL_ENTITIES)) { // gets all egg driller entities, does stuff with both the car and drill types
                            driller->car->state.Set(&EggDriller::Car_Explode); // car->state++;
                            driller->car->velocity.y = -0x18000;               // sets the y velocity for the car bounce

                            if (driller->drill->state.Matches(&EggDriller::Drill_Attached)) {
                                driller->drill->state.Set(&EggDriller::Drill_Fired);
                                if (this->direction == FLIP_NONE) {
                                    driller->drill->velocity.x = -0x30000;
                                }
                                else {
                                    driller->drill->velocity.x = 0x30000;
                                }
                            }
                        }
                    }
                    else {
                        this->eggmanAnimator.SetAnimation(sVars->aniFrames, Hit, false, 0);
                        this->invincibilityTimer = 32;
                        sVars->bossHitSFX.Play(false, 255);
                    }
                }
            }
        }
    }

    if (this->exploding == true) { // checks at any point if the object is exploding yet lol
        if ((Zone::sVars->timer &= 7) == 0) {
            Vector2 explosionPos;
            explosionPos.x = this->position.x + Math::Rand(TO_FIXED(-48), TO_FIXED(48));
            explosionPos.y = this->position.y + Math::Rand(TO_FIXED(-24), TO_FIXED(24));

            Explosion *explosion = GameObject::Create<Explosion>(INT_TO_VOID(Explosion::Type2), explosionPos.x, explosionPos.y);
            explosion->drawGroup = 4;
            explosion->sVars->sfxExplosion.Play(false, 255);
        }
    }

    // if eggman is on the hit or laugh animation, it checks for when the animator reaches the final frame (via the frame count) and sets it back to
    // idle when it gets there
    if (this->eggmanAnimator.animationID == Hit || this->eggmanAnimator.animationID == Laugh) {
        if (this->eggmanAnimator.frameID == this->eggmanAnimator.frameCount - 1) {
            this->eggmanAnimator.SetAnimation(sVars->aniFrames, Idle, false, 0);
        }
        if (this->health == 0) { // way of fixing an issue of him still laughing and getting stuck on the idle animation if he dies while hes in the
                                 // middle of playing it
            this->eggmanAnimator.SetAnimation(sVars->aniFrames, Toasted, false, 0);
        }
    }

    this->state.Run(this);
    this->helicopterAnimator.Process();
    this->mobileAnimator.Process();
    this->eggmanAnimator.Process();
}

void EHZEggman::LateUpdate() {}
void EHZEggman::StaticUpdate() {}
void EHZEggman::Draw()
{
    this->helicopterAnimator.DrawSprite(nullptr, false);
    this->seatAnimator.DrawSprite(nullptr, false);
    this->eggmanAnimator.DrawSprite(nullptr, false);
    this->mobileAnimator.DrawSprite(nullptr, false);
}

void EHZEggman::Create(void *data)
{
    if (!sceneInfo->inEditor) {
        this->active    = ACTIVE_BOUNDS;
        this->drawFX    = FX_FLIP;
        this->visible   = true;
        this->drawGroup = 2;
        this->seatAnimator.SetAnimation(sVars->aniFrames, 5, true, 0);
        this->mobileAnimator.SetAnimation(sVars->aniFrames, 6, true, 0);
        this->state.Set(&EHZEggman::State_AwaitPlayer);
    }
}

void EHZEggman::StageLoad()
{
    sVars->aniFrames.Load("Eggman/EggMobile.bin", SCOPE_STAGE);
    sVars->helicopterSFX.Get("Stage/Helicopter.wav");
    sVars->bossHitSFX.Get("Stage/BossHit.wav");
    sVars->hitbox.left   = -24;
    sVars->hitbox.top    = -24;
    sVars->hitbox.right  = 24;
    sVars->hitbox.bottom = 24;

    Zone::AddToHyperList(sVars->classID, true, true, true);
}

void EHZEggman::PlayHeliSFX()
{
    if (this->heliSFXTimer == 0) {
        sVars->helicopterSFX.Play(false, 255);
    }
    this->heliSFXTimer++;
    this->heliSFXTimer &= 31;
}

void EHZEggman::State_AwaitPlayer()
{
    Zone::sVars->playerBoundActiveL[0] = true;
    Zone::sVars->playerBoundActiveR[0] = true;
    Zone::sVars->playerBoundActiveB[0] = true;
    Zone::sVars->cameraBoundsL[0]      = FROM_FIXED(this->position.x) - screenInfo->center.x;
    Zone::sVars->cameraBoundsR[0]      = FROM_FIXED(this->position.x) + screenInfo->center.x;
    Zone::sVars->cameraBoundsB[0]      = FROM_FIXED(this->position.y);

    this->eggmanAnimator.SetAnimation(sVars->aniFrames, Idle, false, 0);
    this->helicopterAnimator.SetAnimation(sVars->aniFrames, Active, false, 0);
    Music::ClearMusicStack();
    Music::PlayOnFade(Music::TRACK_EGGMAN2, 0.025f);

    // The Car!!!!
    // object[+1].type = TypeName[Eggman Car]
    EggDriller *car = GameObject::Get<EggDriller>(
        sceneInfo->entitySlot + 1); // gets the current slot of the object, and adds it by 1 as the egg driller car is 1 slot above ehzeggman
    GameObject::Reset(sceneInfo->entitySlot + 1, EggDriller::sVars->classID,
                      INT_TO_VOID(Car)); // so INT_TO_VOID(type) is how you can handle different types at create huh
    car->position.x = this->position.x + 0x1590000;
    car->position.y = this->position.y;
    car->boundsL    = this->position.x - 0x1180000;
    // ixpos
    int32 screenPosition =
        ((this->position.x - 200) + screenInfo->center.x) >> 16; // i assume this is the position the screen stops at so the boss can play
    // camera[0] position in v4, i think this is the equivalent?
    ScreenInfo *screen = &screenInfo[sceneInfo->currentScreenID];
    if (screen->position.x < screenPosition) {
        screenPosition -= screen->position.x;
        car->boundsL -= screenPosition;
    }
    car->boundsR = this->position.x + 0x1500000;
    car->car     = car;
    car->eggman  = this; // assigns eggman pointer for this egg driller entity to this ehzeggman entity

    // Drrrrrill
    // object[+2].type = TypeName[Eggman Drill]
    EggDriller *drill = GameObject::Get<EggDriller>(sceneInfo->entitySlot + 2);
    GameObject::Reset(sceneInfo->entitySlot + 2, EggDriller::sVars->classID, INT_TO_VOID(Drill));
    drill->car   = car; // sets the new drill entity car pointer to the previous entity created
    drill->drill = drill;
    car->drill   = drill; // sets the car entity drill pointer to this drill

    // Back Wheel
    // object[-1].type = TypeName[Eggman Wheel]
    EggDriller *backWheel =
        GameObject::Get<EggDriller>(sceneInfo->entitySlot - 1); // as its one slot below everything else, its drawn behind everything else
    GameObject::Reset(sceneInfo->entitySlot - 1, EggDriller::sVars->classID, INT_TO_VOID(BackWheel));
    backWheel->xOffset = -0x2C0000;
    car->wheel[0]      = backWheel; // car needs its wheel assigned as the car functions use em
    backWheel->car     = car;
    backWheel->drill   = drill;

    // Front Wheel 1
    // object[+3].type = TypeName[Eggman Wheel]
    EggDriller *frontWheel = GameObject::Get<EggDriller>(sceneInfo->entitySlot + 3);
    GameObject::Reset(sceneInfo->entitySlot + 3, EggDriller::sVars->classID, INT_TO_VOID(FrontWheel));
    frontWheel->xOffset = -0xC0000;
    car->wheel[1]       = frontWheel;
    frontWheel->car     = car;
    frontWheel->drill   = drill;

    // Front Wheel 2
    // object[+4].type = TypeName[Eggman Wheel]
    EggDriller *frontWheel2 = GameObject::Get<EggDriller>(sceneInfo->entitySlot + 4);
    GameObject::Reset(sceneInfo->entitySlot + 4, EggDriller::sVars->classID, INT_TO_VOID(FrontWheel));
    frontWheel2->xOffset = 0x1C0000;
    car->wheel[2]        = frontWheel2;
    frontWheel2->car     = car;
    frontWheel2->drill   = drill;

    this->position.x += 0x1580000;
    this->position.y -= 0x13F0000;
    this->active = ACTIVE_NORMAL;
    this->state.Set(&EHZEggman::State_FlyIn);
}

void EHZEggman::State_FlyIn()
{
    EHZEggman::PlayHeliSFX();
    this->position.x -= 0x10000;
    this->position.y += 0x10000;
    this->timer++;
    if (this->timer == 320) {
        this->helicopterAnimator.SetAnimation(sVars->aniFrames, Stop, false, 0);
        this->timer = 0;
        this->state.Set(&EHZEggman::State_EnterCar);
    }
}

void EHZEggman::State_EnterCar() { this->state.Set(&EHZEggman::State_StartCar); }

void EHZEggman::State_StartCar()
{
    this->timer++;
    if (this->timer == 24) {
        this->helicopterAnimator.SetAnimation(sVars->aniFrames, Retracting, false, 0);
        this->timer = 0;

        this->health = 8;

        this->state.Set(&EHZEggman::State_InCar);
        for (auto driller : GameObject::GetEntities<EggDriller>(FOR_ALL_ENTITIES)) { // gets all the egg drillers, but only uses the car type
            if (driller->type == Car) {
                driller->velocity.x = -0x20000;
                driller->state.Set(&EggDriller::Car_Driving);
            }
        }
    }
}

void EHZEggman::State_InCar() {} // idle state for this object basically, gets set to the next state when the health reaches 0 in update

void EHZEggman::State_Explode()
{
    this->timer++;
    if (this->timer == 180) {
        this->timer = 0;
        this->helicopterAnimator.SetAnimation(sVars->aniFrames, Extending, false, 0);
        this->exploding = false;
        this->state.Set(&EHZEggman::State_ExitCar);
        Music::ClearMusicStack();
        Music::PlayOnFade(Music::TRACK_STAGE, 0.025f);
        Zone::sVars->playerBoundActiveR[0] = false;
        Vector2 layerSize;
        Zone::sVars->fgLayer[0].Size(&layerSize,
                                     true);          // i assume this gets the size of the first fg layer (low) and sets the layerSize variable to it
        Zone::sVars->cameraBoundsR[0] = layerSize.x; // sets the right camera bound to the stage size
    }
}

void EHZEggman::State_ExitCar() { this->state.Set(&EHZEggman::State_Flee); }

void EHZEggman::State_Flee()
{
    EHZEggman::PlayHeliSFX();
    this->position.y -= 0x10000;
    if (this->helicopterAnimator.animationID == Extending) {
        if (this->helicopterAnimator.frameID == this->helicopterAnimator.frameCount - 1) {
            this->helicopterAnimator.SetAnimation(sVars->aniFrames, Active, false, 0);
        }
    }

    this->timer++;
    if (this->timer == 96) {
        this->timer     = 0;
        this->direction = FLIP_X; // FACING_LEFT in v4
        this->state.Set(&EHZEggman::State_Escape);
    }
}

void EHZEggman::State_Escape()
{
    if (this->CheckOnScreen(nullptr)) {
        EHZEggman::PlayHeliSFX();
    }
    this->position.x += 0x60000;

    if (!this->CheckOnScreen(nullptr)) {
        this->Destroy();
    }
}

#if RETRO_INCLUDE_EDITOR
void EHZEggman::EditorDraw() {}

void EHZEggman::EditorLoad() {}
#endif

void EHZEggman::Serialize() {}

} // namespace GameLogic