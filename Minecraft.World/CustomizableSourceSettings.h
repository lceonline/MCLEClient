
#pragma once
#include "FlatGeneratorInfo.h"


class CustomizableSourceSettings
{
public:
   
    float mainNoiseScaleX;        
    float mainNoiseScaleY;       
    float mainNoiseScaleZ;        
    float depthNoiseScaleX;         
    float depthNoiseScaleZ;         
    float depthNoiseExponent;       
    float coordinateScale;          
    float heightScale;             
    float upperLimitScale;         
    float lowerLimitScale;         
    float biomeDepthWeight;         
    float biomeDepthOffset;         
    float biomeScaleWeight;         
    float biomeScaleOffset;         
    float baseSize;                
    float stretchY;                

   
    int   seaLevel;                 
    bool  useCaves;                 
    bool  useStrongholds;           
   
    int   biomeOverride;            
    bool  useDungeons;              
    bool  useTemples;               
    bool  useMineShafts;            
    bool  useVillages;              
    bool  useRavines;               
    bool  useWaterLakes;            
    bool  useLavaLakes;             
   
    int   dungeonChance;            
    int   waterLakeChance;         
    
    int   lavaLakeChance;          
    bool  useNether;              

    
    int   dirtSize;                
    int   dirtCount;
    int   gravelSize;
    int   gravelCount;            
    int   graniteSize;     
    int   graniteCount;
    int   dioriteSize;
    int   dioriteCount;
    int   andesiteSize;      
    int   andesiteCount;         
    int   coalOreSize;            
    int   coalOreCount;     
    int   ironOreSize;       
    int   ironOreCount;     
    int   goldOreSize;        
    int   goldOreCount;        
    int   redstoneOreSize;       
    int   redstoneOreCount;        
    int   diamondOreSize;        
    int   diamondOreCount;        
    int   lapisOreSize;           
    int   lapisOreCount;       
    int   lapisOreHeight;        
    int   lapisOreSpread;         
    int   goldOreExtraSize;     
    int   goldOreExtraCount;       
    int   goldOreExtraHeight;      
    int   emeraldOreSize;           
    int   emeraldOreCount;        
    int   silverfishSize;         
    int   silverfishCount;        
    int   silverfishHeight;       
    int   quartzOreSize;          
    int   quartzOreCount;        
    int   quartzOreHeight;        
    int   magmaSize;              
    int   magmaCount;             
    int   netherGoldSize;          
    int   netherGoldCount;         
    int   netherQuartzSize;        
    int   netherQuartzCount;        
    int   netherQuartzHeight;     
    int   netherBrickSize;         
    int   netherBrickCount;        
    int   soulSandSize;          
    int   soulSandCount;          
    int   glowstoneSize;           


   
    int getBiomeOverride() const
    {
        
           if (biomeOverride == 8)
            return -1;
            return biomeOverride;
    }
     int getBiomeSize() { return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x68); }
        int getRiverSize() { return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x6C); }

    class Builder
    {
    public:
       
        float mainNoiseScaleX;
        float mainNoiseScaleY;
        float mainNoiseScaleZ;
        float depthNoiseScaleX;
        float depthNoiseScaleZ;
        float depthNoiseExponent;
        float coordinateScale;
        float heightScale;
        float upperLimitScale;
        float lowerLimitScale;
        float biomeDepthWeight;
        float biomeDepthOffset;
        float biomeScaleWeight;
        float biomeScaleOffset;
        float baseSize;
        float stretchY;
        int   seaLevel;
        bool  useCaves;
        bool  useStrongholds;
        int   biomeOverride;
        bool  useDungeons;
        bool  useTemples;
        bool  useMineShafts;
        bool  useVillages;
        bool  useRavines;
        bool  useWaterLakes;
        bool  useLavaLakes;
        bool  useNether;
        int   dungeonChance;
        int   waterLakeChance;
        int   lavaLakeChance;
        int   dirtSize,         dirtCount;
        int   gravelSize,       gravelCount;
        int   graniteSize,      graniteCount;
        int   dioriteSize,      dioriteCount;
        int   andesiteSize,     andesiteCount;
        int   coalOreSize,      coalOreCount;
        int   ironOreSize,      ironOreCount;
        int   goldOreSize,      goldOreCount;
        int   redstoneOreSize,  redstoneOreCount;
        int   diamondOreSize,   diamondOreCount;
        int   lapisOreSize,     lapisOreCount;
        int   lapisOreHeight,   lapisOreSpread;
        int   goldOreExtraSize, goldOreExtraCount, goldOreExtraHeight;
        int   emeraldOreSize,   emeraldOreCount;
        int   silverfishSize,   silverfishCount,   silverfishHeight;
        int   quartzOreSize,    quartzOreCount,    quartzOreHeight;
        int   magmaSize,        magmaCount;
        int   netherGoldSize,   netherGoldCount;
        int   netherQuartzSize, netherQuartzCount, netherQuartzHeight;
        int   netherBrickSize,  netherBrickCount;
        int   soulSandSize,     soulSandCount;
        int   glowstoneSize;

        Builder();  

        static Builder* setDefaults(Builder* b);
        static Builder* fromString(void* superflatConfig);
        static CustomizableSourceSettings* build(Builder* b);
       

    };
};