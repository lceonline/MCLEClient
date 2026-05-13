#include "stdafx.h"
#include "CustomizableSourceSettings.h"

CustomizableSourceSettings::Builder::Builder()
{
	setDefaults(this);
}

CustomizableSourceSettings::Builder* CustomizableSourceSettings::Builder::setDefaults(Builder* b)
{
	b->coordinateScale = 684.412f;
	b->heightScale = 684.412f;
	b->upperLimitScale = 512.0f;
	b->lowerLimitScale = 512.0f;
	b->mainNoiseScaleX = 80.0f;
	b->mainNoiseScaleY = 160.0f;
	b->mainNoiseScaleZ = 80.0f;
	b->depthNoiseScaleX = 200.0f;
	b->depthNoiseScaleZ = 200.0f;
	b->depthNoiseExponent = 0.5f;
	b->biomeDepthWeight = 1.0f;
	b->biomeDepthOffset = 0.0f;
	b->biomeScaleWeight = 1.0f;
	b->biomeScaleOffset = 0.0f;
	b->baseSize = 8.5f;
	b->stretchY = 12.0f;
	b->seaLevel = 63;
	b->useCaves = true;
	b->useStrongholds = true;
	b->biomeOverride = -1;
	b->useDungeons = true;
	b->dungeonChance = 8;
	b->useTemples = true;
	b->useMineShafts = true;
	b->useVillages = true;
	b->useRavines = true;
	b->useWaterLakes = true;
	b->waterLakeChance = 4;
	b->useLavaLakes = true;
	b->lavaLakeChance = 80;
	b->useNether = false;
	b->dirtSize = 33;
	b->dirtCount = 10;
	b->gravelSize = 33;
	b->gravelCount = 8;
	b->graniteSize = 33;
	b->graniteCount = 10;
	b->dioriteSize = 33;
	b->dioriteCount = 10;
	b->andesiteSize = 33;
	b->andesiteCount = 10;
	b->coalOreSize = 17;
	b->coalOreCount = 20;
	b->ironOreSize = 9;
	b->ironOreCount = 20;
	b->goldOreSize = 9;
	b->goldOreCount = 2;
	b->redstoneOreSize = 8;
	b->redstoneOreCount = 8;
	b->diamondOreSize = 8;
	b->diamondOreCount = 1;
	b->lapisOreSize = 7;
	b->lapisOreCount = 1;
	b->lapisOreHeight = 16;
	b->lapisOreSpread = 16;
	return b;
}

CustomizableSourceSettings::Builder* CustomizableSourceSettings::Builder::fromString(void* superflatConfig)
{
	Builder* b = new Builder();
	setDefaults(b);
	// Logic to parse superflatConfig should go here if needed.
	return b;
}

CustomizableSourceSettings* CustomizableSourceSettings::Builder::build(Builder* b)
{
	CustomizableSourceSettings* s = new CustomizableSourceSettings();
	s->coordinateScale = b->coordinateScale;
	s->heightScale = b->heightScale;
	s->upperLimitScale = b->upperLimitScale;
	s->lowerLimitScale = b->lowerLimitScale;
	s->mainNoiseScaleX = b->mainNoiseScaleX;
	s->mainNoiseScaleY = b->mainNoiseScaleY;
	s->mainNoiseScaleZ = b->mainNoiseScaleZ;
	s->depthNoiseScaleX = b->depthNoiseScaleX;
	s->depthNoiseScaleZ = b->depthNoiseScaleZ;
	s->depthNoiseExponent = b->depthNoiseExponent;
	s->biomeDepthWeight = b->biomeDepthWeight;
	s->biomeDepthOffset = b->biomeDepthOffset;
	s->biomeScaleWeight = b->biomeScaleWeight;
	s->biomeScaleOffset = b->biomeScaleOffset;
	s->baseSize = b->baseSize;
	s->stretchY = b->stretchY;
	s->seaLevel = b->seaLevel;
	s->useCaves = b->useCaves;
	s->useStrongholds = b->useStrongholds;
	s->biomeOverride = b->biomeOverride;
	s->useDungeons = b->useDungeons;
	s->dungeonChance = b->dungeonChance;
	s->useTemples = b->useTemples;
	s->useMineShafts = b->useMineShafts;
	s->useVillages = b->useVillages;
	s->useRavines = b->useRavines;
	s->useWaterLakes = b->useWaterLakes;
	s->waterLakeChance = b->waterLakeChance;
	s->useLavaLakes = b->useLavaLakes;
	s->lavaLakeChance = b->lavaLakeChance;
	s->useNether = b->useNether;
	s->dirtSize = b->dirtSize;
	s->dirtCount = b->dirtCount;
	s->gravelSize = b->gravelSize;
	s->gravelCount = b->gravelCount;
	s->graniteSize = b->graniteSize;
	s->graniteCount = b->graniteCount;
	s->dioriteSize = b->dioriteSize;
	s->dioriteCount = b->dioriteCount;
	s->andesiteSize = b->andesiteSize;
	s->andesiteCount = b->andesiteCount;
	s->coalOreSize = b->coalOreSize;
	s->coalOreCount = b->coalOreCount;
	s->ironOreSize = b->ironOreSize;
	s->ironOreCount = b->ironOreCount;
	s->goldOreSize = b->goldOreSize;
	s->goldOreCount = b->goldOreCount;
	s->redstoneOreSize = b->redstoneOreSize;
	s->redstoneOreCount = b->redstoneOreCount;
	s->diamondOreSize = b->diamondOreSize;
	s->diamondOreCount = b->diamondOreCount;
	s->lapisOreSize = b->lapisOreSize;
	s->lapisOreCount = b->lapisOreCount;
	s->lapisOreHeight = b->lapisOreHeight;
	s->lapisOreSpread = b->lapisOreSpread;
	return s;
}
