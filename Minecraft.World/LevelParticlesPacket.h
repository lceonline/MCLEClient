
#pragma once
#include "Packet.h"
#include "../Minecraft.Client/ParticleType.h"

class LevelParticlesPacket : public Packet, public enable_shared_from_this<LevelParticlesPacket>
{
private:
   
    const ParticleType* type;   
    float x;                    
    float y;                    
    float z;                    
    float xDist;                
    float yDist;                
    float zDist;                
    float maxSpeed;             
    int   count;                
    bool  overrideLimiter;      
    int*  params;               
    int   paramCount;           

public:
    LevelParticlesPacket();
    LevelParticlesPacket(const ParticleType* type, bool overrideLimiter,
                         float x, float y, float z,
                         float xDist, float yDist, float zDist,
                         float maxSpeed, int count,
                         arrayWithLength<int> data);
    ~LevelParticlesPacket();

    void read(DataInputStream* dis) override;
    void write(DataOutputStream* dos) override;

    const ParticleType* getType()       { return type; }
    double getX()                       { return x; }
    double getY()                       { return y; }
    double getZ()                       { return z; }
    double getXDist()                   { return xDist; }
    double getYDist()                   { return yDist; }
    double getZDist()                   { return zDist; }
    double getMaxSpeed()                { return maxSpeed; }
    int    getCount()                   { return count; }
    bool   isOverrideLimiter()          { return overrideLimiter; }
    int*   getParams()                  { return params; }
    int    getParamCount()              { return paramCount; }

    void handle(PacketListener* listener) override;
    int  getEstimatedSize() override;

    static shared_ptr<Packet> create() { return std::make_shared<LevelParticlesPacket>(); }
    virtual int getId() override        { return 63; }
};