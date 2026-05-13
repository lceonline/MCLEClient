
#include "stdafx.h"
#include "PacketListener.h"
#include "LevelParticlesPacket.h"
#include "../Minecraft.Client/ParticleType.h"

LevelParticlesPacket::LevelParticlesPacket()
{
    
    this->overrideLimiter = false;  
    this->type            = nullptr; 
    this->count           = 0;      
    this->y               = 0.0f;   
    this->paramCount      = 0;      
    this->x               = 0.0f;   
    this->yDist           = 0.0f;   
    this->zDist           = 0.0f;   
    this->z               = 0.0f;   
    this->maxSpeed        = 0.0f;   
    this->xDist           = 0.0f;   
    this->params          = nullptr; 
}

LevelParticlesPacket::LevelParticlesPacket(const ParticleType* type, bool overrideLimiter,
                                            float x, float y, float z,
                                            float xDist, float yDist, float zDist,
                                            float maxSpeed, int count,
                                            arrayWithLength<int> data)
{
    this->type            = type;           
    this->overrideLimiter = overrideLimiter; 
    this->x               = x;             
    this->y               = y;             
    this->z               = z;             
    this->xDist           = xDist;         
    this->yDist           = yDist;         
    this->zDist           = zDist;         
    this->maxSpeed        = maxSpeed;      
    this->count           = count;         
    this->paramCount      = data.length;   

    if (data.data && data.length > 0)
    {
        this->params = new int[data.length];
        memcpy(this->params, data.data, data.length * sizeof(int));
    }
    else
    {
        this->params = nullptr; 
    }
}

LevelParticlesPacket::~LevelParticlesPacket()
{
   
    if (params)
    {
        delete[] params;
        params = nullptr;
    }
}

void LevelParticlesPacket::read(DataInputStream* dis)
{
    int particleId = dis->readInt();
    const ParticleType* resolved = ParticleType::byId(particleId);
    if (!resolved)
        resolved = ParticleType::getDefault();
    this->type = resolved;

    this->overrideLimiter = dis->readBoolean();

    this->x        = dis->readFloat();
    this->y        = dis->readFloat();
    this->z        = dis->readFloat();
    this->xDist    = dis->readFloat();
    this->yDist    = dis->readFloat();
    this->zDist    = dis->readFloat();
    this->maxSpeed = dis->readFloat();
    this->count    = dis->readInt();

    int paramCount = this->type ? this->type->getParamCount() : 0;
    this->paramCount = paramCount;

    if (paramCount > 0)
    {
        this->params = new int[paramCount]();
        for (int i = 0; i < paramCount; i++)
        {
            this->params[i] = dis->readInt();
        }
    }
    else
    {
        this->params = nullptr;
    }
}

void LevelParticlesPacket::write(DataOutputStream* dos)
{
    dos->writeInt(this->type ? this->type->getId() : 0);
    dos->writeBoolean(this->overrideLimiter);
    dos->writeFloat(this->x);
    dos->writeFloat(this->y);
    dos->writeFloat(this->z);
    dos->writeFloat(this->xDist);
    dos->writeFloat(this->yDist);
    dos->writeFloat(this->zDist);
    dos->writeFloat(this->maxSpeed);
    dos->writeInt(this->count);

    int toWrite = this->type ? this->type->getParamCount() : 0;
    for (int i = 0; i < toWrite; i++)
    {
        dos->writeInt(this->params[i]);
    }
}

void LevelParticlesPacket::handle(PacketListener* listener)
{
    listener->handleParticleEvent(shared_from_this());
}

int LevelParticlesPacket::getEstimatedSize()
{
    return 0x40; 
}