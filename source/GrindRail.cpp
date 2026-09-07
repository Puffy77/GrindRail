#include "GrindRail.h"

// hello chat today we will be porting minecraft into super mario galaxy 3
// Please liek and soupscribe if you want more sick minecraft porting in super mairo galaxee
namespace pt {
    extern void initRailToNearestAndRepositionWithGravity(LiveActor* pActor);
}

namespace NrvGrindRail {
    FULL_NERVE(NrvWait, GrindRail, Wait);
    FULL_NERVE(NrvSnapPlayerToRail, GrindRail, SnapPlayerToRail);
    FULL_NERVE(NrvPlayerOnRail, GrindRail, PlayerOnRail);
    FULL_NERVE(NrvJumpingOff, GrindRail, JumpingOff);
}

/*
Obj_Arg 0: Snapping Radius
Obj_Arg 1: Jump Behavior at Goal

Point_Arg 0: Speed
Point_Arg 1: Acceleration
Point_Arg 2: Jump Momentum Influence
Point_Arg 3: Jump Momentum Type (Orthogonal or Full)
Point_Arg 4: Left-Right Jumping (0: None 1: Set 2: Analog)
Point_Arg 5: Left-Right Jumping Influence
Point_Arg 6: Allow Jumping
*/
GrindRail::GrindRail(const char *pName) : LiveActor(pName) {
    mSnapRadius = 100000.0f;
    mJumpDirectionInfluence = 10.0f;
    mMomentumInfluence = 1.0f;
    mJumpAtEdge = 1;

    
    mCurrentSpeed = 0.0f;
    mNearestPos.set(0.0f, 0.0f, 0.0f);
    mHasSpinned = false;
    mAnimWait = 0;
    mDamageResetDelay = 0;
    mLastUpVec.set(0.0f, 0.0f, 0.0f);
    mLastSideVec.set(0.0f, 0.0f, 0.0f);
}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {
    OSReport("Init\n");

    MR::processInitFunction(this, rIter, false);
    MR::onCalcGravity(this);
    MR::connectToSceneRide(this);

    //MR::hideModel(this);

    MR::getJMapInfoArg0NoInit(rIter, &mSnapRadius);
    MR::getJMapInfoArg1NoInit(rIter, &mJumpDirectionInfluence);
    MR::getJMapInfoArg2NoInit(rIter, &mMomentumInfluence);
    MR::getJMapInfoArg3NoInit(rIter, &mJumpAtEdge);
    mSnapRadius /= 1000.0f;
    mJumpDirectionInfluence /= 1000.0f;
    mMomentumInfluence /= 1000.0f;
    

    MR::useStageSwitchReadA(this, rIter);

    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initHitSensor(1);
    MR::addHitSensorBinder(this, "SpinDetector", 1, mSnapRadius, TVec3f(0.0f, 0.0f, 0.0f));

    initNerve(&NrvGrindRail::NrvWait::sInstance, 0);
    makeActorAppeared();
    
}


void GrindRail::control() {}

void GrindRail::exeWait(){

    OSReport("Waiting\n");
    TVec3f playerPos = *MR::getPlayerPos();

    MR::calcNearestRailPos(&mNearestPos, this, playerPos);

    mTranslation = mNearestPos;
    mRailRider->moveToNearestPos(mNearestPos);

}

bool GrindRail::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgPlayerSpinAttack(msg) && isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance)) {
        mHasSpinned = true;
        mAnimWait = 34;
        return true;
    }
    return false;
}

bool GrindRail::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver){
    OSReport("receiveOtherMsg %X\n", msg);

    if(MR::isMsgAutoRushBegin(msg) && !MR::isPlayerJumpRising() && MR::getPlayerLife() > 0 && ((MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) || !MR::isValidSwitchA(this))){
        setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);
        return true;
    }
    else if (MR::isMsgUpdateBaseMtx(msg)){
        updatePlayerMtx();
        return true;
    }
    else if (MR::isMsgRushCancel(msg)){
        
        return true;
    }
    else{
        return false;
    }
}

void GrindRail::exeSnapPlayerToRail() {
    MR::setPlayerPos(mNearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
    MR::getCurrentRailPointArg0NoInit(this, &mCurrentSpeed);
}


void GrindRail::exePlayerOnRail() {

    OSReport("OnRail\n");
    bool jumpedFromEdge = false;

    const char *currentBckName = MR::getPlayerCurrentBckName();
    bool isSpinBck = strcmp(currentBckName, "SpinGround") == 0;

    // rip terry

    if (MR::isPlayerDamaging() && MR::getPlayerLife() > 0 && !MR::isPlayerParalyzing()) {

        if (mDamageResetDelay <= 0) {
            mDamageResetDelay = 5;
        }
        MR::endBindAndPlayerWait(this);

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

    if (MR::isPlayerParalyzing()) {
        MR::endBindAndPlayerElectricDamage(this);
    }

    if(MR::getPlayerLife() <= 0){
        MR::endBindAndPlayerWait(this);
    }

    if (MR::isFirstStep(this)) {
        
        MR::startBckPlayer("SlidingRopeWait", static_cast< const char* >(nullptr));
        MR::becomeContinuousBckPlayer();
    
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

    
    if (MR::getPlayerTriggerA() && !jumpedFromEdge) {
        MR::changePlayerAnimAndStartBvaIfExist("JumpBack");
        OSReport("Normal Jump\n");
        MR::endBindAndPlayerJump(this, getJumpVec(mCurrentSpeed, 3), 0);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        //MR::setPlayerJumpVec(getJumpVec(mCurrentSpeed, 3));
        //MR::endBindAndPlayerWait(this);
        return;
    }

    if(mRailRider->isReachedGoal() || mRailRider->isReachedEdge()){

        jumpedFromEdge = true;

        if (mJumpAtEdge & 1) {
            MR::changePlayerAnimAndStartBvaIfExist("GrowPlantJump");
        }
        else {
            MR::changePlayerAnimAndStartBvaIfExist("Fall");
        }

        OSReport("Edge Jump\n");
        MR::endBindAndPlayerJump(this, getJumpVec(mCurrentSpeed, mJumpAtEdge), 0);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        
    }

}

void GrindRail::exeJumpingOff() {
    OSReport("OffRail\n");
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
    TVec3f orthogonalJumpVec;
    if (railDirection.dot(jumpVec) >= 0.999f || railDirection.dot(jumpVec) <= -0.999f) {
        orthogonalJumpVec = TVec3f(0.0, 0.0, 0.0);
    }
    else{
        f32 parallelJumpVec = railDirection.dot(jumpVec);
        orthogonalJumpVec = railDirection - (jumpVec * parallelJumpVec);
        orthogonalJumpVec.normalize(orthogonalJumpVec);

        orthogonalJumpVec.scale(currentSpeed * mMomentumInfluence);
    }

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

void GrindRail::updatePlayerMtx(){

    mRailRider->setSpeed(mCurrentSpeed);
    mRailRider->move();
    MR::moveTransToCurrentRailPos(this);

  
    TVec3f railDir = MR::getRailDirection(this);
    railDir.normalize(railDir);
    TVec3f sideVec;
    TVec3f upVec;
    

    if (railDir.dot(-mGravity) >= 0.999f || railDir.dot(-mGravity) <= -0.999f) {

        if(mLastSideVec.length() <= 0.001f || mLastUpVec.length() <= 0.001f){

            TVec3f helperVec;
            if(abs(mGravity.x) <= abs(mGravity.y) && abs(mGravity.x) <= abs(mGravity.z)){
                helperVec = TVec3f(1, 0, 0);
            } 
            else if(abs(mGravity.y) <= abs(mGravity.z) && abs(mGravity.y) <= abs(mGravity.x)){
                helperVec = TVec3f(0, 1, 0);
            }
            else if (abs(mGravity.z) <= abs(mGravity.x) && abs(mGravity.z) <= abs(mGravity.y)){
                helperVec = TVec3f(0, 0, 1);
            }
            
            PSVECCrossProduct(-mGravity, helperVec, sideVec);
            sideVec.normalize(sideVec);
            mLastSideVec = sideVec;

            PSVECCrossProduct(-mGravity, sideVec, upVec);
            upVec.normalize(upVec);
            mLastUpVec = upVec;

        }
        else{

            sideVec = mLastSideVec;
            upVec = mLastUpVec;

        }
    }
    else{

        PSVECCrossProduct(-mGravity, railDir, sideVec);
        sideVec.normalize(sideVec);
        mLastSideVec = sideVec;

        
        PSVECCrossProduct(railDir, sideVec, upVec);
        upVec.normalize(upVec);
        mLastUpVec = upVec;
    }

    TPos3f baseMtx(getBaseMtx());
    TPos3f playerMtx;
    playerMtx.identity();

    TVec3f trans;
    baseMtx.getTrans(trans);
    playerMtx.setTrans(trans);
    playerMtx.setZDir(railDir);
    playerMtx.setYDir(upVec);
    playerMtx.setXDir(sideVec);

    MR::setBaseTRMtx(MarioAccess::getPlayerActor(), playerMtx);

}