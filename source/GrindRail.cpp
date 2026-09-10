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

/*
Obj_Arg 0: Snapping Radius (Default: 100.000f)
Obj_Arg 1: Jump Behavior at Goal (Default: 1)
Obj_Arg 2: Reattach Delay (Default: 30)
Obj_Arg 3: SW_B Behavior (While Riding or On Reaching Goal) (Default: 1)

Point_Arg 0: Speed (Default: 20.0f)
Point_Arg 1: Acceleration (Default: 1.000f)
Point_Arg 2: Jump Momentum Type (Orthogonal or Full) (Default: 0)
Point_Arg 3: Jump Momentum Influence (Default: 1.000f)
Point_Arg 4: Left-Right Jumping Strength (Default: 5.000f)
Point_Arg 5: Allow Jumping (Default: 1)
Point_Arg 6: Jump Strength (Default: 25.000f)
Point_Arg 7: Allow Spinning (Default: 1)
*/

GrindRail::GrindRail(const char *pName) : LiveActor(pName) {
    mSnapRadius = 100000.0f;
    mJumpAtEdge = 1;
    mReattachDelay = 30;
    mSWBBehavior = 1;

    mPointSpeed = 20.0f;
    mPointAccel = 1000.0f;
    mMomentumType = 0;
    mMomentumInfluence = 1000.0f;
    mLRJumpingStrength = 5000.0f;
    mAllowJumping = 1;
    mJumpStrength = 25000.0f;
    mAllowSpinning = 1;

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

    MR::onCalcGravity(this);
    MR::connectToScene(this, 0x28, -1, -1, -1);

    MR::getJMapInfoArg0NoInit(rIter, &mSnapRadius);
    MR::getJMapInfoArg1NoInit(rIter, &mJumpAtEdge);
    MR::getJMapInfoArg2NoInit(rIter, &mReattachDelay);
    MR::getJMapInfoArg3NoInit(rIter, &mSWBBehavior);

    if(mSnapRadius <= 0.0f){
        mSnapRadius = 100000.0f;
    }
    if(mJumpAtEdge < 0 || mJumpAtEdge > 3){
        mJumpAtEdge = 1;
    }
    if(mReattachDelay <= 0){
        mReattachDelay = 30;
    }
    if(mSWBBehavior < 0 || mSWBBehavior > 1){
        mSWBBehavior = 1;
    }

    mSnapRadius /= 1000.0f;

    MR::useStageSwitchReadA(this, rIter);
    MR::useStageSwitchWriteB(this, rIter);
    if(MR::isValidSwitchB(this)){
        MR::offSwitchB(this);
    }
        

    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initHitSensor(3);
    MR::addHitSensorBinder(this, "Snap", 4, mSnapRadius, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensor(this, "Spinning", ATYPE_PLAYER, 6, 200.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensor(this, "Damage", ATYPE_PLAYER, 4, 75.0f, TVec3f(0.0f, 0.0f, 0.0f));

    initNerve(&NrvGrindRail::NrvWait::sInstance, 0);
    makeActorAppeared();
    
}


void GrindRail::control() {}

void GrindRail::exeWait(){

    TVec3f playerPos = *MR::getPlayerPos();

    MR::calcNearestRailPos(&mNearestPos, this, playerPos);

    mTranslation = mNearestPos;
    mRailRider->moveToNearestPos(mNearestPos);

}



void GrindRail::attackSensor(HitSensor* pSender, HitSensor* pReceiver){

    if(mHasSpinned && pSender == getSensor("Spinning")){
        bool out = MR::sendMsgPlayerPunch(pReceiver, pSender);
        OSReport("Spin Attack BANG, %d\n", out);
    }

}

bool GrindRail::receiveMsgEnemyAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver){

    if(isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance) && pReceiver == getSensor("Damage")){
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        MR::endBindAndPlayerWait(this);
        bool out = MR::getPlayerBodySensor()->receiveMessage(msg, pSender);
        return out;
    }
    
    return false;

}



bool GrindRail::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver){

    if(isNerve(&NrvGrindRail::NrvWait::sInstance) && MR::isMsgAutoRushBegin(msg) && !MR::isPlayerJumpRising() && MR::getPlayerLife() > 0 && ((MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) || !MR::isValidSwitchA(this))){
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

    bool jumpedFromEdge = false;

    if (MR::isValidSwitchB(this) && mSWBBehavior == 0) {
        MR::onSwitchB(this);
    }

    // rip terry

    MR::stopPlayerFpView();

    if (MR::isPlayerDamaging() && MR::getPlayerLife() > 0 && !MR::isPlayerParalyzing()) {

        if (mDamageResetDelay <= 0) {
            mDamageResetDelay = 5;
        }
        TVec3f damageDir = MR::getRailDirection(this);
        damageDir.scale(-1.0f);
        damageDir.normalize(damageDir);
        MR::getRailDirection(this);
        MR::endBindAndPlayerDamage(this, damageDir);

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

    if (MR::isFirstStep(this) || mAnimWait == 26) {
        
        MR::startBckPlayer("SlidingRopeWait", static_cast< const char* >(nullptr));
        MR::becomeContinuousBckPlayer();
    
    }

    getPointArgs();

    if(mHasSpinned){
        mHasSpinned = false;
    }

    if(MR::isPadSwing(0) && mAllowSpinning == 1 && mAnimWait == 0){
        MR::changePlayerAnimAndStartBvaIfExist("SpinGround");

        mHasSpinned = true;
        mAnimWait = 60;
    }

    mAnimWait--;
    if(mAnimWait <= 0){
        mAnimWait = 0;
    }

    if (mCurrentSpeed < mPointSpeed) {
        mCurrentSpeed += mPointAccel;
        if (mCurrentSpeed > mPointSpeed) {
            mCurrentSpeed = mPointSpeed;
        }
    }
    else if (mCurrentSpeed > mPointSpeed) {
        mCurrentSpeed -= mPointAccel;
        if (mCurrentSpeed < mPointSpeed) {
            mCurrentSpeed = mPointSpeed;
        }
    }
    
    
    if (MR::getPlayerTriggerA() && !jumpedFromEdge && mAllowJumping == 1) {
        MR::changePlayerAnimAndStartBvaIfExist("JumpBack");
        MR::endBindAndPlayerJump(this, getJumpVec(mCurrentSpeed, 3), 0);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
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

        if(MR::isValidSwitchB(this) && mSWBBehavior == 1){
            MR::onSwitchB(this);
        }
        
        MR::endBindAndPlayerJump(this, getJumpVec(mCurrentSpeed, mJumpAtEdge), 0);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        
    }

}



void GrindRail::exeJumpingOff() {
    
    if (MR::isGreaterEqualStep(this, mReattachDelay)) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }

    if (MR::isValidSwitchB(this) && mSWBBehavior == 0) {
        MR::offSwitchB(this);
    }

    mHasSpinned = false;
    mAnimWait = 0;
}



TVec3f GrindRail::getJumpVec(f32 currentSpeed, s32 includeJump){

    TVec3f railDirection;
    railDirection = MR::getRailDirection(this);
    railDirection.normalize(railDirection);

    TVec3f jumpVec = -mGravity;
    jumpVec.normalize(jumpVec);

    // Left-Right momentum
    TVec3f orthogonalLRVec;
    if (railDirection.dot(jumpVec) >= 0.999f || railDirection.dot(jumpVec) <= -0.999f) {
        orthogonalLRVec = TVec3f(0.0, 0.0, 0.0);
    }
    else{
        PSVECCrossProduct(railDirection, jumpVec, orthogonalLRVec);
        orthogonalLRVec.normalize(orthogonalLRVec);
        TVec3f stick;
        MR::calcWorldStickDirectionXZ(&stick, 0);
        if(stick.length() >= 0.05f){
            stick.normalize(stick);
            orthogonalLRVec.scale(stick.dot(orthogonalLRVec));
            orthogonalLRVec.normalize(orthogonalLRVec);
            orthogonalLRVec.scale(mLRJumpingStrength);
        }
        else{
            orthogonalLRVec = TVec3f(0.0, 0.0, 0.0);
        }
    }

    // Jump momentum
    TVec3f momentumJumpVec;
    if (railDirection.dot(jumpVec) >= 0.999f || railDirection.dot(jumpVec) <= -0.999f) {
        momentumJumpVec = TVec3f(0.0, 0.0, 0.0);
    }
    else if (mMomentumType == 0){
        f32 parallelJumpVec = railDirection.dot(jumpVec);
        momentumJumpVec = railDirection - (jumpVec * parallelJumpVec);
        momentumJumpVec.normalize(momentumJumpVec);

        momentumJumpVec.scale(currentSpeed * mMomentumInfluence);
    }
    else {
        momentumJumpVec = railDirection;
        momentumJumpVec.scale(currentSpeed * mMomentumInfluence);
    }

    jumpVec.scale(mJumpStrength);


    TVec3f finalVec;
    finalVec = momentumJumpVec;

    if(includeJump & 1) {
        finalVec += jumpVec;
    }
    
    if(includeJump & (1 << 1)){
        finalVec += orthogonalLRVec;
    }

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

    TPos3f baseMtx;
    MR::makeMtxTRS(baseMtx, this);
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



void GrindRail::getPointArgs(){

    f32 pointSpeed = 20.0f;
    f32 pointAccel = 1000.0f;
    f32 momentumType = 0.0f;
    f32 momentumInfluence = 1000.0f;
    s32 lrJumpingStrength = 5000;
    s32 allowJumping = 1;
    f32 jumpStrength = 25000.0f;
    s32 allowSpinning = 1;

    // For some odd reason Arg 2 has to be Float and arg 4 has to be bool when using currentrailpointnoinit so im working around it.
    s32 railPoint = MR::getCurrentRailPointNo(this);

    MR::getCurrentRailPointArg0NoInit(this, &pointSpeed);
    MR::getCurrentRailPointArg1NoInit(this, &pointAccel);
    MR::getCurrentRailPointArg2NoInit(this, &momentumType);
    MR::getCurrentRailPointArg3NoInit(this, &momentumInfluence);
    MR::getRailPointArg4NoInit(this, railPoint, &lrJumpingStrength);
    MR::getCurrentRailPointArg5NoInit(this, &allowJumping);
    MR::getCurrentRailPointArg6NoInit(this, &jumpStrength);
    MR::getCurrentRailPointArg7NoInit(this, &allowSpinning);

    

    
    mPointSpeed = pointSpeed;
    mPointAccel = pointAccel / 1000.0f;
    mMomentumType = (s32)momentumType;
    mMomentumInfluence = momentumInfluence / 1000.0f;
    mLRJumpingStrength = lrJumpingStrength / 1000.0f;
    mAllowJumping = allowJumping;
    mJumpStrength = jumpStrength / 1000.0f;
    mAllowSpinning = allowSpinning;

    if(mPointSpeed <= 0.0f) {
        mPointSpeed = 20.0f;
    }
    if(mPointAccel <= 0.0f) {
        mPointAccel = 1.0f;
    }
    if(mMomentumType < 0 || mMomentumType > 1) {
        mMomentumType = 0;
    }
    if(mMomentumInfluence <= 0.0f) {
        mMomentumInfluence = 1.0f;
    }
    if(mLRJumpingStrength < 0.0f) {
        mLRJumpingStrength = 5000.0f;
    }
    if(mAllowJumping < 0 || mAllowJumping > 1) {
        mAllowJumping = 1;
    }
    if(mJumpStrength <= 0.0f) {
        mJumpStrength = 25000.0f;
    }
    if(mAllowSpinning < 0 || mAllowSpinning > 1) {
        mAllowSpinning = 1;
    }

    
}