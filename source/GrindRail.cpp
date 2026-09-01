#include "GrindRail.h"
#include "Game/Player/MarioAccess.h"


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
    momentumInfluence = 1.0f;
    jumpAtEdge = 1;

    
    currentSpeed = 0.0f;
    nearestPos.set(0.0f, 0.0f, 0.0f);
    skateBackwards = false;
    hasSpinned = false;
    animWait = 0;

}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {
    OSReport("Init\n");

    MR::processInitFunction(this, rIter, false);
    MR::onCalcGravity(this);
    MR::connectToSceneMapObjStrongLight(this);

    MR::hideModel(this);

    MR::getJMapInfoArg0NoInit(rIter, &snapRadius);
    MR::getJMapInfoArg1NoInit(rIter, &momentumInfluence);
    MR::getJMapInfoArg2NoInit(rIter, &jumpAtEdge);
    momentumInfluence = momentumInfluence / 1000.0f;

    MR::useStageSwitchReadA(this, rIter);

    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initHitSensor(1);
    MR::addHitSensorMapObj(this, "SpinDetector", 1, 500.0f, TVec3f(0.0f, 0.0f, 0.0f));

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
        mRailRider->moveToNearestPos(nearestPos);

        delta = nearestPos - playerPos;
        if (delta.length() < snapRadius && !MR::isPlayerJumpRising() && MR::getPlayerLife() > 0 && ((MR::isValidSwitchA(this) && MR::isOnSwitchA(this))) || !MR::isValidSwitchA(this)) {
            OSReport("Snap to rail\n");
            setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);
        }
    }

}

void GrindRail::exeWait(){}

bool GrindRail::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgPlayerSpinAttack(msg) && isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance)) {
        MR::changePlayerAnimAndStartBvaIfExist("IceSkateSpin");
        hasSpinned = true;
        animWait = 49;
        if(skateBackwards) {
           skateBackwards = false;
        }
        else {
           skateBackwards = true;
        }
        return true;
    }
    return false;
}

void GrindRail::exeSnapPlayerToRail() {
    MR::setPlayerPos(nearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
    skateBackwards = false;
    MR::getCurrentRailPointArg0NoInit(this, &currentSpeed);
}


void GrindRail::exePlayerOnRail() {

    bool jumpedFromEdge = false;
   
    if(!MR::isPlayerJumpRising() || jumpedFromEdge) {
        MR::becomePlayerNormalJumpStatus();
        MR::setPlayerStateWait();
    }

    if (MR::isPlayerDamaging() && MR::getPlayerLife() > 0 && !MR::isPlayerParalyzing()){
        MR::resetPlayerStatus();
        MR::startBckPlayer("SkateR", static_cast< const char* >(nullptr));
        MR::becomeContinuousBckPlayer();
    }

    if (MR::getPlayerLife() <= 0 || MR::isPlayerParalyzing()) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }

    if((hasSpinned && animWait <= 0) || MR::isFirstStep(this)) {
        if(skateBackwards) {
            MR::startBckPlayer("SkateL", static_cast< const char* >(nullptr));
            hasSpinned = false;
            
        }
        else {
            MR::startBckPlayer("SkateR", static_cast< const char* >(nullptr));
            hasSpinned = false;
        }
        MR::becomeContinuousBckPlayer();
    }

    if (MR::getPlayerCurrentBckName() != "SkateR" && MR::getPlayerCurrentBckName() != "SkateL" && !MR::getPlayerCurrentBckName() != "IceSkateSpin") {
        if(skateBackwards) {
            MR::startBckPlayer("SkateL", static_cast< const char* >(nullptr));
        }
        else {
            MR::startBckPlayer("SkateR", static_cast< const char* >(nullptr));
        }
        MR::becomeContinuousBckPlayer();
    }

    animWait--;
    if(animWait < 0) {
        animWait = 0;
    }

    
    
    f32 targetSpeed = 0.0f;
    MR::getCurrentRailPointArg0NoInit(this, &targetSpeed);
    f32 accel = 0.0f;
    MR::getCurrentRailPointArg1NoInit(this, &accel);
    accel /= 1000.0f;

    if (currentSpeed < targetSpeed) {
        currentSpeed += accel;
        if (currentSpeed > targetSpeed) {
            currentSpeed = targetSpeed;
        }
    }
    else if (currentSpeed > targetSpeed) {
        currentSpeed -= accel;
        if (currentSpeed < targetSpeed) {
            currentSpeed = targetSpeed;
        }
    }
    
    
    //OSReport("Rail Speed: %f\n", speed);


    mRailRider->setSpeed(currentSpeed);
    mRailRider->move();
    MR::moveTransToCurrentRailPos(this);
    MR::setPlayerPos(mTranslation);

    TVec3f railDirection;
    railDirection = MR::getRailDirection(this);
    MR::setPlayerFrontVec(railDirection, 1);
    

    if(mRailRider->isReachedGoal() || mRailRider->isReachedEdge()){
        TVec3f endVec = getJumpVec(currentSpeed, jumpAtEdge);
        MR::forceJumpPlayer(endVec);
        jumpedFromEdge = true;
        if (jumpAtEdge == 1) {
            MR::changePlayerAnimAndStartBvaIfExist("IceJump");
        }
        else {
            MR::changePlayerAnimAndStartBvaIfExist("Fall");
        }
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
    }

    if(MR::isPlayerJumpRising() && !jumpedFromEdge) {
        MR::changePlayerAnimAndStartBvaIfExist("IceJump");
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        TVec3f jumpVec = getJumpVec(currentSpeed, 1);
        MR::setPlayerJumpVec(jumpVec);
    }


}

void GrindRail::exeJumpingOff() {
    if (MR::isGreaterEqualStep(this, 30)) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }
}

TVec3f GrindRail::getJumpVec(f32 currentSpeed, s32 includeJump){

    TVec3f railDirection;
    railDirection = MR::getRailDirection(this);
    OSReport("Rail Direction: %f, %f, %f\n", railDirection.x, railDirection.y, railDirection.z);

    TVec3f jumpVec = -mGravity;
    jumpVec.normalize(jumpVec);

    f32 parallelVec = railDirection.dot(jumpVec);
    TVec3f orthogonalVec = railDirection - (jumpVec * parallelVec);
    orthogonalVec.normalize(orthogonalVec);

    orthogonalVec.scale(currentSpeed * momentumInfluence);
    jumpVec.scale(25.0f);
    TVec3f finalVec;

    if(includeJump == 1) {
        finalVec = orthogonalVec + jumpVec;
    }
    else {
        finalVec = orthogonalVec;
    }
    

    OSReport("Perpendicular Vector: %f, %f, %f\n", orthogonalVec.x, orthogonalVec.y, orthogonalVec.z);
    OSReport("Final Jump Vector: %f, %f, %f\n", finalVec.x, finalVec.y, finalVec.z);
    return finalVec;
}