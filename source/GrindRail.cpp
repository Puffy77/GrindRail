#include "GrindRail.h"


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
    mSnapRadius = 100.0f;
    mJumpDirectionInfluence = 10.0f;
    mMomentumInfluence = 1.0f;
    mJumpAtEdge = 1;

    
    mCurrentSpeed = 0.0f;
    mNearestPos.set(0.0f, 0.0f, 0.0f);
    mSkateBackwards = false;
    mHasSpinned = false;
    mAnimWait = 0;
    mDamageResetDelay = 0;

}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {
    OSReport("Init\n");

    MR::processInitFunction(this, rIter, false);
    MR::onCalcGravity(this);
    MR::connectToSceneMapObjStrongLight(this);
    MR::joinToGroupArray(this, rIter, "RailGroup", 32);

    MR::hideModel(this);

    MR::getJMapInfoArg0NoInit(rIter, &mSnapRadius);
    MR::getJMapInfoArg1NoInit(rIter, &mJumpDirectionInfluence);
    MR::getJMapInfoArg2NoInit(rIter, &mMomentumInfluence);
    MR::getJMapInfoArg3NoInit(rIter, &mJumpAtEdge);
    mJumpDirectionInfluence = mJumpDirectionInfluence / 1000.0f;
    mMomentumInfluence = mMomentumInfluence / 1000.0f;
    

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

    MR::calcNearestRailPosAndDirection(&mNearestPos, &nearestDirection, this, playerPos);

    if (isNerve(&NrvGrindRail::NrvWait::sInstance)){
        mTranslation = mNearestPos;
        mRailRider->moveToNearestPos(mNearestPos);

        delta = mNearestPos - playerPos;
        if (delta.length() < mSnapRadius && !MR::isPlayerJumpRising() && MR::getPlayerLife() > 0 && ((MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) || !MR::isValidSwitchA(this))) {
            OSReport("Snap to rail\n");
            setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);
        }
    }

}

void GrindRail::exeWait(){}

bool GrindRail::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgPlayerSpinAttack(msg) && isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance)) {
        mHasSpinned = true;
        mAnimWait = 49;
        if(mSkateBackwards) {
           mSkateBackwards = false;
        }
        else {
           mSkateBackwards = true;
        }
        return true;
    }
    return false;
}

void GrindRail::exeSnapPlayerToRail() {
    MR::setPlayerPos(mNearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
    mSkateBackwards = false;
    MR::getCurrentRailPointArg0NoInit(this, &mCurrentSpeed);
}


void GrindRail::exePlayerOnRail() {

    bool jumpedFromEdge = false;

    const char *currentBckName = MR::getPlayerCurrentBckName();
    bool isSkateR = strcmp(currentBckName, "SkateR") == 0;
    bool isSkateL = strcmp(currentBckName, "SkateL") == 0;
    bool isSpinBck = strcmp(currentBckName, "SpinGround") == 0;

    if(!MR::isPlayerJumpRising() || jumpedFromEdge) {
        MR::becomePlayerNormalJumpStatus();
        MR::setPlayerStateWait();
    }

    if (MR::isPlayerDamaging() && MR::getPlayerLife() > 0 && !MR::isPlayerParalyzing()) {
        if (mDamageResetDelay <= 0) {
            mDamageResetDelay = 5;
        }
    }
    else {
        mDamageResetDelay = 0;
    }

    if (mDamageResetDelay > 0) {
        mDamageResetDelay--;
        if (mDamageResetDelay == 0) {
            MR::resetPlayerStatus();
            MR::startBckPlayer("SkateR", static_cast< const char* >(nullptr));
            MR::becomeContinuousBckPlayer();
        }
    }

    if (MR::getPlayerLife() <= 0 || MR::isPlayerParalyzing()) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
        mDamageResetDelay = 0;
    }

    if ((!isSpinBck && !isSkateR && !isSkateL) || ((mHasSpinned && mAnimWait <= 0) || MR::isFirstStep(this))) {
        if(mSkateBackwards) {
            MR::startBckPlayer("SkateL", static_cast< const char* >(nullptr));
        }
        else {
            MR::startBckPlayer("SkateR", static_cast< const char* >(nullptr));
        }
        MR::becomeContinuousBckPlayer();
        if (mHasSpinned) {
            mHasSpinned = false;
        }
    }

    OSReport("Current Animation: %s\n", currentBckName);
    mAnimWait--;
    if(mAnimWait < 0) {
        mAnimWait = 0;
    }

    
    
    f32 targetSpeed = 0.0f;
    MR::getCurrentRailPointArg0NoInit(this, &targetSpeed);
    f32 accel = 0.0f;
    MR::getCurrentRailPointArg1NoInit(this, &accel);
    accel /= 1000.0f;

    if (mCurrentSpeed < targetSpeed) {
        mCurrentSpeed += accel;
        if (mCurrentSpeed > targetSpeed) {
            mCurrentSpeed = targetSpeed;
        }
    }
    else if (mCurrentSpeed > targetSpeed) {
        mCurrentSpeed -= accel;
        if (mCurrentSpeed < targetSpeed) {
            mCurrentSpeed = targetSpeed;
        }
    }
    
    
    //OSReport("Rail Speed: %f\n", speed);


    mRailRider->setSpeed(mCurrentSpeed);
    mRailRider->move();
    MR::moveTransToCurrentRailPos(this);
    MR::setPlayerPos(mTranslation);

    TVec3f railDirection;
    railDirection = MR::getRailDirection(this);
    MR::setPlayerFrontVec(railDirection, 1);
    

    if(mRailRider->isReachedGoal() || mRailRider->isReachedEdge()){
        TVec3f endVec = getJumpVec(mCurrentSpeed, mJumpAtEdge);
        MR::forceJumpPlayer(endVec);
        jumpedFromEdge = true;
        if (mJumpAtEdge == 1) {
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
        TVec3f jumpVec = getJumpVec(mCurrentSpeed, 3);
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

    // Left-Right momentum
    TVec3f orthogonalLRVec;
    PSVECCrossProduct(railDirection, jumpVec, orthogonalLRVec);
    orthogonalLRVec.normalize(orthogonalLRVec);
    f32 playerStickX = MR::getPlayerStickX();
    OSReport("Player Stick X: %f\n", playerStickX);
    orthogonalLRVec.scale(playerStickX * mJumpDirectionInfluence);
    

    // Jump momentum
    f32 parallelJumpVec = railDirection.dot(jumpVec);
    TVec3f orthogonalJumpVec = railDirection - (jumpVec * parallelJumpVec);
    orthogonalJumpVec.normalize(orthogonalJumpVec);

    orthogonalJumpVec.scale(currentSpeed * mMomentumInfluence);
    jumpVec.scale(25.0f);


    TVec3f finalVec;
    finalVec = orthogonalJumpVec;

    if(includeJump & 1) {
        finalVec += jumpVec;
    }
    
    if(includeJump & (1 << 1)){
        finalVec += orthogonalLRVec;
    }

    OSReport("Perpendicular Vector: %f, %f, %f\n", orthogonalJumpVec.x, orthogonalJumpVec.y, orthogonalJumpVec.z);
    OSReport("Final Jump Vector: %f, %f, %f\n", finalVec.x, finalVec.y, finalVec.z);
    return finalVec;
}