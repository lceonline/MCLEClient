#pragma once
#include "Animal.h"

class Rabbit : public Animal
{
public:
    enum class Variant
    {
        BROWN           = 0,
        WHITE           = 1,
        BLACK           = 2,
        GOLD            = 3,
        SALT            = 4,
        WHITE_SPLOTCHED = 5,
        EVIL            = 99,
    };

    eINSTANCEOF GetType() { return eTYPE_RABBIT; }
    static Entity *create(Level *level) { return new Rabbit(level); }

private:
    static const int DATA_TYPE_ID = 16;

    float jumpCompletion;
    int   jumpTicks;
    int   moreCarrotTicks;

public:
    Rabbit(Level *level);
    virtual ~Rabbit() {}

    float getJumpCompletion(float partialTick) const;

    Variant getVariant() const;
    void    setVariant(Variant v);

    virtual bool useNewAi() override;
    virtual void tick() override;

protected:
    virtual void registerAttributes() override;
    virtual void defineSynchedData() override;

    virtual int   getAmbientSound() override;
    virtual int   getHurtSound()    override;
    virtual int   getDeathSound()   override;
    virtual int   getHopSound()   ;
    virtual float getSoundVolume()  override { return 0.6f; }
    virtual bool  makeStepSound()   override { return true; }
    virtual void  dropDeathLoot(bool wasKilledByPlayer, int playerBonusLevel) override;

public:
    virtual bool isFood(shared_ptr<ItemInstance> item) override;
    virtual shared_ptr<AgableMob> getBreedOffspring(shared_ptr<AgableMob> target) override;

    virtual void readAdditionalSaveData(CompoundTag *tag) override;
    virtual void addAdditonalSaveData(CompoundTag *tag) override;

    virtual MobGroupData *finalizeMobSpawn(MobGroupData *groupData, int extraData = 0) override;
};