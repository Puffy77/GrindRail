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
    TVec3f getJumpVec(f32 currentSpeed);

    f32 snapRadius;
    f32 momentumInfluence;

    f32 currentSpeed;
    TVec3f nearestPos;
    bool skateBackwards;
    bool hasSpinned;
    s32 animWait;
    
};