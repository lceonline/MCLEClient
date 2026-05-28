#pragma once
class BufferedImage;
class MemTextureProcessor;
using namespace std;

class MemTexture {
public:
	BufferedImage *loadedImage;
    int count;
    int id;
    bool isLoaded;
	int ticksSinceLastUse;
	// @CDevJoud
	// changing the lifetime of the texture from 20 ticks(1 sec) to 200 ticks (10 sec)
	// as it helps the texture to have longer lifetime and reducing the usage of loadTexture for every second/frame
	// note that we dont remove the code that removes the textures from `tick()` as it is required in memory limited environment such as older Consoles(PS3/XBOX360)
	static const int UNUSED_TICKS_TO_FREE = 200;

	//default ctor for int Texture::getHeight(const wstring& url, int backup)
	MemTexture() = default;

    MemTexture(const wstring& _name, PBYTE pbData, DWORD dwBytes, MemTextureProcessor *processor);
	~MemTexture();
};