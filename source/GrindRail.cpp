#include "GrindRail.h"

namespace pt {
    extern void initRailToNearestAndRepositionWithGravity(LiveActor* pActor);
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


void GrindRail::control() {}

void GrindRail::exeWait(){

    TVec3f nearestDirection;
    TVec3f delta;
    TVec3f playerPos = *MR::getPlayerPos();

    MR::calcNearestRailPosAndDirection(&mNearestPos, &nearestDirection, this, playerPos);

    mTranslation = mNearestPos;
    mRailRider->moveToNearestPos(mNearestPos);

    delta = mNearestPos - playerPos;
    if (delta.length() < mSnapRadius && !MR::isPlayerJumpRising() && MR::getPlayerLife() > 0 && ((MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) || !MR::isValidSwitchA(this))) {

        setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);

    }

}

bool GrindRail::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgPlayerSpinAttack(msg) && isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance)) {
        mHasSpinned = true;
        mAnimWait = 34;
        return true;
    }
    return false;
}

void GrindRail::exeSnapPlayerToRail() {
    MR::setPlayerPos(mNearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
    MR::getCurrentRailPointArg0NoInit(this, &mCurrentSpeed);
}


void GrindRail::exePlayerOnRail() {

    bool jumpedFromEdge = false;

    const char *currentBckName = MR::getPlayerCurrentBckName();
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
            MR::startBckPlayer("SlidingRopeWait", static_cast< const char* >(nullptr));
            MR::becomeContinuousBckPlayer();

        }
    }

    if (MR::getPlayerLife() <= 0 || MR::isPlayerParalyzing()) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }

    if (!isSpinBck || ((mHasSpinned && mAnimWait <= 0) || MR::isFirstStep(this))) {
        
        MR::startBckPlayer("SlidingRopeWait", static_cast< const char* >(nullptr));
        
        MR::becomeContinuousBckPlayer();
        if (mHasSpinned) {
            mHasSpinned = false;
        }

    }

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


    mRailRider->setSpeed(mCurrentSpeed);
    mRailRider->move();
    MR::moveTransToCurrentRailPos(this);
    MR::setPlayerPos(mTranslation);

    TVec3f railDir = MR::getRailDirection(this);
    railDir.normalize(railDir);
    MR::setPlayerFrontVec(railDir,1);

    if (MR::isPlayerJumpRising() && !jumpedFromEdge) {
        MR::changePlayerAnimAndStartBvaIfExist("JumpBack");
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        MR::setPlayerJumpVec(getJumpVec(mCurrentSpeed, 3));
        return;
    }

    if(mRailRider->isReachedGoal() || mRailRider->isReachedEdge()){
        TVec3f endVec = getJumpVec(mCurrentSpeed, mJumpAtEdge);
        MR::forceJumpPlayer(endVec);
        jumpedFromEdge = true;
        if (mJumpAtEdge & 1) {
            MR::changePlayerAnimAndStartBvaIfExist("GrowPlantJump");
        }
        else {
            MR::changePlayerAnimAndStartBvaIfExist("Fall");
        }
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
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