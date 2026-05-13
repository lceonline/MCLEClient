#include "stdafx.h"
#include "TileEntityRenderer.h"
#include "TileEntityRenderDispatcher.h"

void TileEntityRenderer::bindTexture(ResourceLocation *location)
{
    Textures *t = tileEntityRenderDispatcher->textures;
    if(t != nullptr) t->bind(t->loadTexture(location->getTexture()));
}

void TileEntityRenderer::bindTexture(const wstring& urlTexture, ResourceLocation *location)
{
    Textures *t = tileEntityRenderDispatcher->textures;
    if(t != nullptr) t->bind(t->loadHttpTexture(urlTexture, location->getTexture()));
}

int TileEntityRenderer::getHeight(ResourceLocation *location)
{
    Textures *t = tileEntityRenderDispatcher->textures;
    if(t != nullptr) return t->getHeight(location->getTexture());
    return 32;
}

Level *TileEntityRenderer::getLevel()
{
	return tileEntityRenderDispatcher->level;
}

void TileEntityRenderer::init(TileEntityRenderDispatcher *tileEntityRenderDispatcher)
{
	this->tileEntityRenderDispatcher = tileEntityRenderDispatcher;
}

Font *TileEntityRenderer::getFont()
{
	return tileEntityRenderDispatcher->getFont();
}