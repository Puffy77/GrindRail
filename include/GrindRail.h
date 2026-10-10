#pragma once
#include "syati.h"
class GrindRailDrawer;

class GrindRail : public LiveActor {
public:

    GrindRail(const char *pName);
    virtual ~GrindRail();

    virtual void init(const JMapInfoIter &rIter);
    virtual void draw() const;
    virtual void control();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgEnemyAttack(u32 msg, HitSensor *pSender, HitSensor *pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver);

    void initDraw();

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
    s32 mCollisionBehavior;
    s32 mDrawRails;

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
    TVec3f mLastUpVec;
    TVec3f mLastSideVec;
    bool mWallBonkDeath;

    GrindRailDrawer* mDrawer;
};

class GrindRailDrawer {
public:

    s32 mNumPoints;
    s32 mNumLinePoints;
    s32 mNumLoopPoints;
    TVec3f* mPoints;
    TVec3s* mNormals;
    f32* mRailCoords;
    u32 mDispListLength;
    u8* mDispList;

    s32 calcPointIndex(int i, int j) const {
        return j + i * mNumLoopPoints;
    }

    GrindRailDrawer(GrindRail *pGrindRail);

    void initPoints(GrindRail* pGrindRail);
    void initDisplayList();
    void sendGD() const;
    void drawGD() const;
    void loadMaterialHigh() const;
};