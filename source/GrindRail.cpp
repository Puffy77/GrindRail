#include "GrindRail.h"
#include "Game/Player/MarioActor.h"
#include "Game/Player/MarioAccess.h"
#include "Game/Player/MarioState.h"
#include "Game/Player/MarioModule.h"

namespace pt {
    extern void initRailToNearestAndRepositionWithGravity(LiveActor* pActor);
    extern void turnToDirectionUpFront(LiveActor *pActor, TVec3f rUp, TVec3f rFront);
}

namespace NrvGrindRail {
    FULL_NERVE(NrvWait, GrindRail, Wait);
    FULL_NERVE(NrvSnapPlayerToRail, GrindRail, SnapPlayerToRail);
    FULL_NERVE(NrvPlayerOnRail, GrindRail, PlayerOnRail);
    FULL_NERVE(NrvJumpingOff, GrindRail, JumpingOff);
}

GrindRail::GrindRail(const char *pName) : LiveActor(pName) {
    snapRadius = 100.0f;
    nearestPos.set(0.0f, 0.0f, 0.0f);
    MarioActor* player = MarioAccess::getPlayerActor();
}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {
    OSReport("Init\n");

    MR::processInitFunction(this, rIter, false);
    MR::onCalcGravity(this);
    MR::connectToSceneMapObjStrongLight(this);

    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initNerve(&NrvGrindRail::NrvWait::sInstance, 0);
    makeActorAppeared();
    
}


void GrindRail::control() {

    TVec3f nearestDirection;
    TVec3f delta;
    TVec3f playerPos = *MR::getPlayerPos();

    MR::calcNearestRailPosAndDirection(&nearestPos, &nearestDirection, this, playerPos);

    if (isNerve(&NrvGrindRail::NrvWait::sInstance)){
        mTranslation = nearestPos;
        

        delta = nearestPos - playerPos;
        if (delta.length() < snapRadius && !MR::isPlayerJumpRising() ) {
            OSReport("Snap to rail\n");
            setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);
        }
    }

}

void GrindRail::exeWait(){}

void GrindRail::exeSnapPlayerToRail() {
    MR::setPlayerPos(nearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
}

void GrindRail::exePlayerOnRail() {
    if (MR::isFirstStep(this) || MR::isPlayerHipDropFalling() || MR::isPlayerSquat()) {
        OSReport("Player on rail\n");
        MR::startBckPlayerJ("SkateL");
        MR::becomePlayerNormalJumpStatus();
        MR::setPlayerStateWait();
    }

    
    
    s32 railPoint = MR::getCurrentRailPointNo(this);
    //OSReport("Rail Point: %d\n", railPoint);
    f32 speed = 0.0f;
    MR::getCurrentRailPointArg0NoInit(this, &speed);
    //OSReport("Rail Speed: %f\n", speed);


    mRailRider->setSpeed(speed);
    mRailRider->move();
    MR::moveTransToCurrentRailPos(this);
    MR::setPlayerPos(mTranslation);

    if(MR::isPlayerJumpRising()) {
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
    }


}

void GrindRail::exeJumpingOff() {
    if (MR::isGreaterEqualStep(this, 30)) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }
}