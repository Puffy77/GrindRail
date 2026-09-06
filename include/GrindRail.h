#pragma once
#include "syati.h"

class GrindRail : public LiveActor {
public:

    GrindRail(const char *pName);
    virtual ~GrindRail();

    virtual void init(const JMapInfoIter &rIter);
    virtual void control();

    void exeSnapPlayerToRail();
    void exePlayerOnRail();
    void exeWait();
    void exeJumpingOff();

    bool receiveMsgPlayerAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    TVec3f getJumpVec(f32 currentSpeed, s32 includeJump);

    f32 mSnapRadius;
    f32 mJumpDirectionInfluence;
    f32 mMomentumInfluence;
    s32 mJumpAtEdge;

    f32 mCurrentSpeed;
    TVec3f mNearestPos;
    bool mSkateBackwards;
    bool mHasSpinned;
    s32 mAnimWait;
    s32 mDamageResetDelay;
    
};