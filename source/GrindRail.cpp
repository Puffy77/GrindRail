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
    snapRadius = 100.0f;
    nearestPos.set(0.0f, 0.0f, 0.0f);
    bool skateBackwards = false;
    
}

GrindRail::~GrindRail() { }

void GrindRail::init(const JMapInfoIter &rIter) {
    OSReport("Init\n");

    MR::processInitFunction(this, rIter, false);
    MR::onCalcGravity(this);
    MR::connectToSceneMapObjStrongLight(this);

    initRailRider(rIter);
    pt::initRailToNearestAndRepositionWithGravity(this);

    initHitSensor(1);
    MR::addHitSensorMapObj(this, "SpinDetector", 1, 100.0f, TVec3f(0.0f, 0.0f, 0.0f));

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
        if (delta.length() < snapRadius && !MR::isPlayerJumpRising() ) {
            OSReport("Snap to rail\n");
            setNerve(&NrvGrindRail::NrvSnapPlayerToRail::sInstance);
        }
    }

}

void GrindRail::exeWait(){}

bool GrindRail::receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    if (MR::isMsgPlayerSpinAttack(msg) && isNerve(&NrvGrindRail::NrvPlayerOnRail::sInstance)) {
        //MR::startBckPlayerJ("IceSkateSpin");
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
}


void GrindRail::exePlayerOnRail() {
    if (MR::isFirstStep(this) || MR::isPlayerHipDropFalling() || MR::isPlayerSquat()) {
        OSReport("Player on rail\n");
        if(skateBackwards) {
            MR::startBckPlayerJ("SkateBackR");
        }
        else {
            MR::startBckPlayerJ("SkateR");
        }
         
        MR::becomeContinuousBckPlayer();
        
    }

    if(!MR::isPlayerJumpRising()){
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

    TVec3f jumpVec = getJumpVec();
    MR::setPlayerJumpVec(jumpVec);

    if(MR::isPlayerJumpRising()) {
        MR::startBckPlayerJ("IceJump");
        setNerve(&NrvGrindRail::NrvJumpingOff::sInstance);
    }


}

void GrindRail::exeJumpingOff() {
    if (MR::isGreaterEqualStep(this, 30)) {
        setNerve(&NrvGrindRail::NrvWait::sInstance);
    }
}

TVec3f GrindRail::getJumpVec(){
    TVec3f railDirection;
    MR::calcNearestRailDirection(&railDirection, this, mTranslation);
    TVec3f upVec;
    upVec = -mGravity;
    TVec3f parallelVec = (railDirection.dot(upVec) / upVec.length()) * upVec;
    TVec3f perpVec = railDirection - parallelVec;
    return upVec + perpVec;
}