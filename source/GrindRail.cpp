#include "GrindRail.h"
#include "JSystem/JMATrigonometric.h"
#include "revolution/gx/GXDispList.h"
#include "revolution/gx/GXEnum.h"
#include "revolution/gx/GXGeometry.h"
#include "revolution/gx/GXLighting.h"
#include "revolution/gx/GXStruct.h"
#include "revolution/gx/GXTransform.h"
#include "revolution/gx/GXVert.h"
#include "revolution/gd/GDBase.h"

namespace {

    inline f32 toRadian(f32 angle) {
        return angle * (JMath::PI / 180.0f);
    }

    static const f32 sTexRateU0 = 0.05f;
    static const f32 sTexRateV0 = 0.0001f;
    static const f32 sTexRateU1 = 0.05f;
    static const f32 sTexRateV1 = 0.0001f;
    static const f32 sTexRateU2 = 0.1f;
    static const f32 sTexRateV2 = 0.0001f;

}

namespace pt {
    extern void initRailToNearestAndRepositionWithGravity(LiveActor *pActor);
}

namespace NrvGrindRail {
    FULL_NERVE(NrvWait, GrindRail, Wait);
    FULL_NERVE(NrvSnapPlayerToRail, GrindRail, SnapPlayerToRail);
    FULL_NERVE(NrvPlayerOnRail, GrindRail, PlayerOnRail);
    FULL_NERVE(NrvJumpingOff, GrindRail, JumpingOff);
}

/*
Doing division by 1000 to get around the integer-only limitations ohoho

Obj_Arg 0: Snapping Radius (Default: 100.000f)
Obj_Arg 1: Jump Behavior at Goal (Default: 1)
Obj_Arg 2: Reattach Delay (Default: 30)
Obj_Arg 3: SW_B Behavior (While Riding or On Reaching Goal) (Default: 1)
Obj_Arg 4: Collision Behavior (Damage or Death) (Default: 0)

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
    mCollisionBehavior = 0;
    mDrawRails = 1;

    mPointSpeed = 20000.0f;
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
    mLastUpVec.set(0.0f, 0.0f, 0.0f);
    mLastSideVec.set(0.0f, 0.0f, 0.0f);
    mWallBonkDeath = false;

    mDrawer = NULL;
}

GrindRailDrawer::GrindRailDrawer(GrindRail *pGrindRail){

    mNumPoints = 0;
    mNumLinePoints = 0;
    mNumLoopPoints = 8;
    mPoints = NULL;
    mNormals = NULL;
    mRailCoords = NULL;
    mDispListLength = 0;
    mDispList = 0;

    initPoints(pGrindRail);
    initDisplayList();

}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {

    MR::onCalcGravity(this);
    MR::connectToScene(this, 0x28, -1, -1, MR::DrawType_WaterRoad);

    MR::getJMapInfoArg0NoInit(rIter, &mSnapRadius);
    MR::getJMapInfoArg1NoInit(rIter, &mJumpAtEdge);
    MR::getJMapInfoArg2NoInit(rIter, &mReattachDelay);
    MR::getJMapInfoArg3NoInit(rIter, &mSWBBehavior);
    MR::getJMapInfoArg4NoInit(rIter, &mCollisionBehavior);
    MR::getJMapInfoArg5NoInit(rIter, &mDrawRails);

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
    if(mCollisionBehavior < 0 || mCollisionBehavior > 1){
        mCollisionBehavior = 0;
    }
    if(mDrawRails < 0 || mDrawRails > 8){
        mDrawRails = 1;
    }

    mSnapRadius /= 1000.0f;

    MR::useStageSwitchReadA(this, rIter);
    MR::useStageSwitchWriteB(this, rIter);
    if(MR::isValidSwitchB(this)){
        MR::offSwitchB(this);
    }
        
    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initEffectKeeper(1, "GrindRail", false);
    initSound(2, "GrindRail", &mTranslation, TVec3f(0.0f, 0.0f, 0.0f));

    initHitSensor(3);
    MR::addHitSensorBinder(this, "Snap", 4, mSnapRadius, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensorRide(this, "Spinning", 6, 200.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensorRide(this, "Damage", 4, 75.0f, TVec3f(0.0f, 0.0f, 0.0f));
    initBinder(75.0f, 0.0f, 0);

    if(mDrawRails >= 1){
        mDrawer = new GrindRailDrawer(this);
    }
    
    initNerve(&NrvGrindRail::NrvWait::sInstance, 0);
    makeActorAppeared();

}

void GrindRail::draw() const {
        OSReport("Running draw. \n");
    if (!MR::isValidDraw(this)) {
        return;
    }

    if (mDrawer){
        OSReport("Calling drawGD. \n");
        mDrawer->drawGD();
    }

}

void GrindRail::control() {}


void GrindRail::exeWait(){

    TVec3f playerPos = *MR::getPlayerPos();

    MR::calcNearestRailPos(&mNearestPos, this, playerPos);

    mTranslation = mNearestPos;
    mRailRider->moveToNearestPos(mNearestPos);

}



void GrindRail::attackSensor(HitSensor* pSender, HitSensor* pReceiver){

    if(isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance) && mHasSpinned && pSender == getSensor("Spinning")){
        MR::sendMsgPlayerPunch(pReceiver, pSender);
    }

    if(isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance) && pSender == getSensor("Damage")){
        MR::tryGetItem(pSender, pReceiver);
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

void GrindRail::initDraw(){

}

void GrindRail::exeSnapPlayerToRail() {

    MR::setPlayerPos(mNearestPos);
    setNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance);
    MR::startActionSound(this, "Attach", -1, -1, -1);

}



void GrindRail::exePlayerOnRail() {

    if (MR::isValidSwitchB(this) && mSWBBehavior == 0) {
        MR::onSwitchB(this);
    }

    if (MR::isBindedWall(this) || MR::isBindedRoof(this) || MR::isBindedGround(this)) {
        if(mCollisionBehavior == 1){
            mWallBonkDeath = true;
        }

        TVec3f oppVec = MR::getRailDirection(this);
        oppVec.scale(-1.0f);
        MR::endBindAndPlayerDamage(this, oppVec);
        MR::emitEffect(this, "Collision");
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        return;

    }

    MR::stopPlayerFpView();
    MR::emitEffect(this, "Spark");

    if (MR::isFirstStep(this) || mAnimWait == 26) {
        if(MR::isFirstStep(this)){
            MR::startSound(this, "SE_PM_LV_SKATE_SLIP");
        }
        MR::startBckPlayer("SlidingRopeWait", static_cast< const char* >(nullptr));
        MR::becomeContinuousBckPlayer();
    
    }

    getPointArgs();

    if(mHasSpinned && mAnimWait == 40){
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
    
    if (MR::getPlayerTriggerA() && mAllowJumping == 1) {
        MR::changePlayerAnimAndStartBvaIfExist("JumpBack");
        MR::endBindAndPlayerJump(this, getJumpVec(mCurrentSpeed, 3), 0);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
        return;
    }

    if(mRailRider->isReachedGoal()){

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

    if(MR::isCurrentRushSpinDriver()){
        MR::endBindAndPlayerWait(this);
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
    }

}



void GrindRail::exeJumpingOff() {

    if(MR::isFirstStep(this)){
        MR::startActionSound(this, "JumpingOff", -1, -1, -1);
        MR::stopSound(this, "SE_PM_LV_SKATE_SLIP", 0);
    }

    if(!mWallBonkDeath){
        if (MR::isGreaterEqualStep(this, mReattachDelay)) {
            setNerve(&NrvGrindRail::NrvWait::sInstance);
        }

        if (MR::isValidSwitchB(this) && mSWBBehavior == 0) {
            MR::offSwitchB(this);
        }

        mHasSpinned = false;
        mAnimWait = 0;
    }
    else{
        if (MR::isGreaterEqualStep(this, 10)) {
            MR::forceKillPlayerByAbyss();
        }
    }

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

    f32 pointSpeed = 20000.0f;
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

    mPointSpeed = pointSpeed / 1000.0f;
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
        mLRJumpingStrength = 5.0f;
    }
    if(mAllowJumping < 0 || mAllowJumping > 1) {
        mAllowJumping = 1;
    }
    if(mJumpStrength <= 0.0f) {
        mJumpStrength = 25.0f;
    }
    if(mAllowSpinning < 0 || mAllowSpinning > 1) {
        mAllowSpinning = 1;
    }

}



void GrindRailDrawer::initPoints(GrindRail* pGrindRail){

    s32 numPointsHolder;
    s32 pointInterval = 25;

    MR::moveCoordToStartPos(pGrindRail);

    // Adjust the 100 as needed later 
    s32 numTestPoints = static_cast< s32 >(MR::getRailTotalLength(pGrindRail) / pointInterval) + 1;
    f32 testDelta = MR::getRailTotalLength(pGrindRail) / (numTestPoints - 1);

    TVec3f testRailDir = MR::getRailDirection(pGrindRail);

    s32 numPoints = 1;
    for (s32 idx = 1; idx < numTestPoints; idx++) {
        MR::moveCoord(pGrindRail, testDelta);

        if (MR::getRailDirection(pGrindRail).dot(testRailDir) < 0.995f) {
            numPoints++;
            testRailDir.set(MR::getRailDirection(pGrindRail));
        }

    }

    numPointsHolder = numPoints;

    static const f32 sFloatToShortShift = 32768.0f;

    mNumLinePoints = numPoints < 2 ? 2 : numPoints;
    mNumPoints = mNumLinePoints * mNumLoopPoints;

    f32 lineDelta = MR::getRailTotalLength(pGrindRail) / static_cast< s32 >(MR::getRailTotalLength(pGrindRail) / pointInterval);


    TVec3f up = TVec3f(0.0, 1.0, 0.0);

    f32 loopInterval = ::toRadian(360.0f / mNumLoopPoints);

    mPoints = new (0x20) TVec3f[mNumPoints];
    mNormals = new (0x20) TVec3s[mNumPoints];
    mRailCoords = new f32[mNumPoints];

    MR::moveCoordToStartPos(pGrindRail);
    TVec3f front = MR::getRailDirection(pGrindRail);

    if(front.dot(up) >= 0.999f || front.dot(up) <= -0.999f){
        up = TVec3f(0.05, 0.95, 0.00);
        up.normalize(up);
    }

    s32 pointIdx = 0;
    for (s32 lineIdx = 0; lineIdx < mNumLinePoints; lineIdx++) {
        if (lineIdx == mNumLinePoints - 1) {
            MR::moveCoordToEndPos(pGrindRail);
        }

        TVec3f side = MR::getRailDirection(pGrindRail).cross(up);
        MR::normalize(&side);

        TPos3f loopRot;
        loopRot.identity();
        TVec3f rotAxis = MR::getRailDirection(pGrindRail) * -1.0f;
        loopRot.setRotate(rotAxis, loopInterval);

        TVec3f raildir = MR::getRailDirection(pGrindRail);
        PSVECCrossProduct(side, raildir, up);

        mRailCoords[lineIdx] = MR::getRailCoord(pGrindRail);

        for (s32 loopIdx = 0; loopIdx < mNumLoopPoints; loopIdx++) {
            TVec3f pos = side;
            pos.scale(10.0f);
            pos.add(MR::getRailPos(pGrindRail));
            mPoints[pointIdx].set(pos);
            mNormals[pointIdx].set(side.x * sFloatToShortShift, side.y * sFloatToShortShift, side.z * sFloatToShortShift);
            pointIdx++;
            loopRot.mult(side, side);
        }

        while (pointIdx < mNumPoints && !MR::isRailReachedGoal(pGrindRail) && MR::getRailDirection(pGrindRail).dot(front) > 0.995f) {
            MR::moveCoord(pGrindRail, lineDelta);
        }

        front.set(MR::getRailDirection(pGrindRail));

    }

}



void GrindRailDrawer::initDisplayList(){

    MR::ProhibitSchedulerAndInterrupts scheduler(false);

    u32 GDDataSize = (((((false ? sizeof(f32) * 4 : sizeof(f32) * 6) + sizeof(u16) * 2) * 2) * mNumLinePoints) + sizeof(u8) + sizeof(u16)) * mNumLoopPoints;

    u32 length = ((GDDataSize / 32) + 2) * 32;
    mDispList = new (0x20) u8[length];
    DCInvalidateRange(mDispList, length);
    GDLObj obj;
    GDInitGDLObj(&obj, mDispList, length);
    GDSetCurrent(&obj);
    sendGD();
    GDPadCurr32();
    mDispListLength = GDGetGDLObjOffset(&obj);
    DCStoreRange(mDispList, length);

}

void GrindRailDrawer::sendGD() const{

    f32 texU0A = 0.0f;
    f32 texU0B = ::sTexRateU0;
    f32 texU1A = 0.0f;
    f32 texU1B = ::sTexRateU1;
    f32 texU2A = 0.0f;
    f32 texU2B = ::sTexRateU2;

    for (s32 loopIdx = 0; loopIdx < mNumLoopPoints; loopIdx++) {
        s32 nextLoopIdx = loopIdx + 1;
        if (loopIdx == mNumLoopPoints - 1) {
            nextLoopIdx = 0;
        }

        u16 numPoints = mNumLinePoints * 2;
        GDWrite_u8(GX_TRIANGLESTRIP);
        GDWrite_u16(numPoints);

        for (s32 lineIdx = 0; lineIdx < mNumLinePoints; lineIdx++) {
            f32 texV0 = mRailCoords[lineIdx] * ::sTexRateV0;
            f32 texV1 = mRailCoords[lineIdx] * ::sTexRateV1;
            f32 texV2 = mRailCoords[lineIdx] * ::sTexRateV2;

            u16 pointIdx = calcPointIndex(lineIdx, loopIdx);
            u16 nextIdx = calcPointIndex(lineIdx, nextLoopIdx);

            GDWrite_u16(pointIdx);
            GDWrite_u16(pointIdx);
            GDWrite_f32(texU0A);
            GDWrite_f32(texV0);
            GDWrite_f32(texU1A);
            GDWrite_f32(texV1);
            
            GDWrite_f32(texU2A);
            GDWrite_f32(texV2);
            

            GDWrite_u16(nextIdx);
            GDWrite_u16(nextIdx);
            GDWrite_f32(texU0B);
            GDWrite_f32(texV0);
            GDWrite_f32(texU1B);
            GDWrite_f32(texV1);
        
            GDWrite_f32(texU2B);
            GDWrite_f32(texV2);
            
        }

        texU0A = texU0B;
        texU1A = texU1B;
        texU2A = texU2B;
        texU0B += ::sTexRateU0;
        texU1B += ::sTexRateU1;
        texU2B += ::sTexRateU2;

    }

}

void GrindRailDrawer::drawGD() const{

    loadMaterialHigh();
    GXCallDisplayList(mDispList, mDispListLength);

}

void GrindRailDrawer::loadMaterialHigh() const{

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_POS_XY, GX_S16, 16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_POS_XYZ, GX_F32, 0);

    GXClearVtxDesc();

    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX2, GX_DIRECT);

    GXSetArray(GX_VA_POS, mPoints, sizeof(TVec3f));
    GXSetArray(GX_VA_NRM, mNormals, sizeof(TVec3s));

    GXLoadPosMtxImm(MR::getCameraViewMtx(), 0);
    GXLoadNrmMtxImm(MR::getCameraViewMtx(), 0);

    GXSetCurrentMtx(0);
    GXSetNumChans(0);

    GXSetNumTexGens(0);
    GXSetNumIndStages(0);

    // Everything below here should be fine
    GXSetNumTevStages(1);
        
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ONE);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);

    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_OR, GX_GEQUAL, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetZCompLoc(GX_TRUE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetClipMode(GX_CLIP_ENABLE);

}