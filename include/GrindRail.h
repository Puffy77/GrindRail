#pragma once
#include "syati.h"

class GrindRail : public LiveActor {
public:

    GrindRail(const char *pName);
    virtual ~GrindRail();

    virtual void init(const JMapInfoIter &rIter);
    virtual void control();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgEnemyAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    
    void exeSnapPlayerToRail();
    void exePlayerOnRail();
    void exeWait();
    void exeJumpingOff();

    TVec3f getJumpVec(f32 currentSpeed, s32 includeJump);
    void updatePlayerMtx();
    void getPointArgs();

    f32 mSnapRadius;
    s32 mJumpAtEdge;
    s32 mReattachDelay;
    s32 mSWBBehavior;

    f32 mPointSpeed;
    f32 mPointAccel;
    s32 mMomentumType;
    f32 mMomentumInfluence;
    f32 mLRJumpingStrength;
    s32 mAllowJumping;
    f32 mJumpStrength;
    s32 mAllowSpinning;

    f32 mCurrentSpeed;
    TVec3f mNearestPos;
    bool mHasSpinned;
    s32 mAnimWait;
    s32 mDamageResetDelay;
    TVec3f mLastUpVec;
    TVec3f mLastSideVec;

};