/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#ifndef __DAVAENGINE_SPRITE_H__
#define __DAVAENGINE_SPRITE_H__

// TODO: убрать всю инициализацию объекта из конструктора и добавить в другое место которое сможет вернуть 0 в случае ошибки

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Render/Texture.h"
#include "Render/RenderDataObject.h"

#include "FileSystem/FilePath.h"

#include "Render/UniqueStateSet.h"
#include "Platform/Mutex.h"

namespace DAVA
{

enum eSpriteModification
{
	ESM_HFLIP = 1,
	ESM_VFLIP = 1<<1
};

class Shader;
class Texture;

/**
	\ingroup render_2d
	\brief Main class to draw 2d objects on screen.
	This object is fast and efficient for drawing 2d objects on screen.
	It supports texture-atlassing and it uses texture atlasses automatically.
	You can set scale, rotate, pivot point, angle and position for any sprite object before rendering.
 */
class Sprite : public BaseObject
{
public:
	class DrawState
	{
        friend class Sprite;
        
    public:
    
		DrawState();
        ~DrawState();
        
		Vector2 position;
		Vector2 pivotPoint;
		Vector2 scale;
		float32 angle;
		int32	frame;
		uint32  flags;
		float32 sinA;
		float32 cosA;
		float32 precomputedAngle;
		bool usePerPixelAccuracy;

		inline void Reset();
		inline void SetPosition(float32 x, float32 y);
        inline void SetPosition(const Vector2 &drawPos);
		inline void SetPivotPoint(float32 x, float32 y);
		inline void SetScale(float32 x, float32 y);
        inline void SetScaleSize(float32 x, float32 y,
                                float32 width, float32 height);
		inline void SetAngle(float32 a);
		inline void SetFrame(uint32 frame);
		inline void SetFlags(uint32 flags);
		inline void SetPerPixelAccuracyUsage(bool needToUse);
		void BuildStateFromParentAndLocal(const Sprite::DrawState &parentState, const Sprite::DrawState &localState);
        
        void SetShader(Shader* _shader);
        
        inline Shader* GetShader() const
        {
            return shader;
        }
        
        void SetRenderState(UniqueHandle _renderState);
        
        inline UniqueHandle GetRenderState()
        {
            return renderState;
        }
        
    private:
    
        Shader* shader;
        UniqueHandle renderState;

	};

    friend class DrawState;

	enum eSpriteType
	{
			SPRITE_FROM_FILE = 0
		,	SPRITE_FROM_TEXTURE
		,	SPRITE_RENDER_TARGET
	};

	enum eRectsAndOffsets
	{
			X_POSITION_IN_TEXTURE = 0
		,	Y_POSITION_IN_TEXTURE
		,	ACTIVE_WIDTH
		,	ACTIVE_HEIGHT
		,	X_OFFSET_TO_ACTIVE
		,	Y_OFFSET_TO_ACTIVE
	};

	/**
	 \brief Function to create sprite. This is convinience function and it return sprite in any case.
	 If sprite is not available on disk it return 'purple' rect sprite. It will give ability for
	 app to work stable and gave testers ability to find such errors fast.

	 \param spriteName path to sprite name relative to application bundle
	 \return sprite pointer in any case will be returned
	 */
	static Sprite* Create(const FilePath &spriteName);// Creating sprite by name

	/**
	 \brief Function to create sprite as render target.

	 \param sprWidth width of requested render target
	 \param sprHeight height of requested render target
	 \param textureFormat texture pixel format
	 \param contentScaleIncluded set true if content scale already taken into account. Just send false if you don't know how it's works.

	 \return sprite pointer or 0 if it will be impossible to create such render target
	 */
	static Sprite* CreateAsRenderTarget(float32 sprWidth, float32 sprHeight, PixelFormat textureFormat, bool contentScaleIncluded = false);
	void InitAsRenderTarget(float32 sprWidth, float32 sprHeight, PixelFormat textureFormat, bool contentScaleIncluded = false);

	/*
		\brief Function to create sprite
		\param spriteName path to sprite name
		\param forPointer you can create sprite in the allready allocated space or send NULL and memory will be allocated.
		\return 0 if sprite is unavailable and ptr to sprite if sprite is available
	 */
	static Sprite* PureCreate(const FilePath & spriteName, Sprite* forPointer = NULL);
	void InitFromFile(File *file, const FilePath &pathName);

	/**
	 \brief Function to create sprite from the already created texture.

	 \param fromTexture texture to create sprite from
	 \param xOffset horizontal offset in texture to the sprite
	 \param yOffset vertical offset in texture to the sprite
	 \param sprWidth width of requested sprite
	 \param sprHeight height of requested sprite
	 \param contentScaleIncluded set true if content scale allready taken into account. Just send false if you don't know how it's works.

	 \return sprite pointer or 0 if it will be impossible to create such render target
	 */
	static Sprite* CreateFromTexture(Texture *fromTexture, int32 xOffset, int32 yOffset, float32 sprWidth, float32 sprHeight, bool contentScaleIncluded = false);

	static Sprite* CreateFromTexture(const Vector2 & spriteSize, Texture * fromTexture, const Vector2 & textureRegionOffset, const Vector2 & textureRegionSize, const FilePath &spriteName = FilePath());

	void InitFromTexture(Texture *fromTexture, int32 xOffset, int32 yOffset, float32 sprWidth, float32 sprHeight, int32 targetWidth, int32 targetHeight, bool contentScaleIncluded = false, const FilePath &spriteName = FilePath());


	/*
	 \brief Function to prepare sprite tiling. Shifts texture coordinates by approximately 1 pixel to the center. Tiled sprites can be drawn using scale and there will be no empty pixels between them.
	 */
	void PrepareForTiling();

	void SetOffsetsForFrame(int frame, float32 xOff, float32 yOff);


	virtual int32 Release();

	Texture* GetTexture();
	Texture* GetTexture(int32 frameNumber);

	UniqueHandle GetTextureHandle(int32 frameNumber);

	int32 GetFrameCount() const;

	float32 GetWidth() const;
	float32 GetHeight() const;
	const Vector2 &GetSize() const;

	const Vector2 &GetDefaultPivotPoint() const;

	//void SetFrame(int32 frm);

	void SetDefaultPivotPoint(float32 x, float32 y);
	void SetDefaultPivotPoint(const Vector2 &newPivotPoint);

	//void SetPivotPoint(float32 x, float32 y);
	//void SetPivotPoint(const Vector2 &newPivotPoint);

	//void SetPosition(float32 x, float32 y);
	//void SetPosition(const Vector2 &drawPos);

	//void SetAngle(float32 angleInRadians);

	//void SetScale(float32 xScale, float32 yScale);
	//void SetScale(const Vector2 &newScale);

	//void SetScaleSize(float32 width, float32 height);//scale size overrides standart scale
	//void SetScaleSize(const Vector2 &drawSize);

	void SetModification(int32 modif);

	//void ResetPivotPoint();

	//void ResetAngle();
	void ResetModification();
	//void ResetScale();
	void Reset();//Reset do not resets the pivot point

	void BeginBatching();
	void EndBatching();

	//void Draw();
	void Draw(DrawState * state);
	/**
	 \brief	Draw sprite by the 4 verticies.
		The vertices sequence is (xLeft,yTop), (xRight,yTop), (xLeft,yBottom), (xRight,yBottom)
	 \param v poiterto the array of the four Vector2 objects.
	 */
	void DrawPoints(Vector2 *verticies, DrawState* drawState);

	void DrawPoints(Vector2 *verticies, Vector2 *textureCoordinates, DrawState* drawState);

	inline int32 GetResourceSizeIndex() const;


	/**
		\brief Function to get rect & offset of sprite frame position in texture
	 */
	float32 GetRectOffsetValueForFrame(int32 frame, eRectsAndOffsets valueType) const;
	const float32 * GetFrameVerticesForFrame(int32 frame) const;
	const float32 * GetTextureCoordsForFrame(int32 frame) const;

	/**
		\brief Access to texCoords private field
	 */
	float32 ** GetTextureCoordinates() { return texCoords; }

	/**
		\brief Function to dump the sprites currently loaded in memory

	 */
	static void DumpSprites();

	/**
	 \brief Function prepares all sprites for the new screen size

	 */
	static void ValidateForSize();

	/**
		\brief Set polygon to draw the sprite with specific clip. Resets to null after draw.
	 */
	void SetClipPolygon(Polygon2 * clipPolygon);

	/**
	 \brief Returns multiplyer to convert sprite to the physical coordinates.
	 */
	inline float32 GetResourceToPhysicalFactor() const;

	/**
	 \brief Returns texture coordinates for the requested frame.
	 */
	float32 *GetTextureVerts(int32 frame);

	/**
	\brief Convert sprite size as if it was created by Sprite::Create.
	Useful when you create sprite by Sprite::CreateFromTexture and want to use it as normal (virtual sized).
	Converts only first frame.
	*/
	void ConvertToVirtualSize();

	const FilePath & GetRelativePathname() const;

	inline void PrepareSpriteRenderData(Sprite::DrawState * drawState);
	RenderDataObject * spriteRenderObject;

	void Reload();
	
	static void SetSpriteClipping(bool clipping);

protected:
	Sprite();
	Sprite(int32 sprWidth, int32 sprHeight, PixelFormat format);
	virtual ~Sprite();

	/**
	 \brief Removes all sprite data.
	 */
	void Clear();
	
    static Sprite* GetSpriteFromMap(const FilePath & pathname);
    static FilePath GetScaledName(const FilePath & spriteName);
    static File* LoadLocalizedFile(const FilePath & spritePathname, FilePath & texturePath);
    
    void ReloadExistingTextures();
	
	void RegisterTextureStates();
	void UnregisterTextureStates();

//private:

    static Mutex spriteMapMutex;

	enum eSpriteTransform
	{
		EST_ROTATE			= 1,
		EST_SCALE			= 1 << 1,
		EST_MODIFICATION	= 1 << 2
	};

	float32 tempVertices[8];


	FilePath  relativePathname;

	Texture ** textures;
	FilePath *textureNames;
	int32 *frameTextureIndex;
	int32 textureCount;

	Vector<UniqueHandle> textureHandles;
	
	float32 **frameVertices;
	float32 **texCoords;

//	float32 **originalVertices;

	Polygon2 * clipPolygon;

	void PrepareForNewSize();
    void SetFrame(int32 frm);

	Vector2	size;
//	Vector2 originalSize;

	int32	frameCount;
	int32	frame;

	Vector2	defaultPivotPoint;
	//Vector2	pivotPoint;

	//Vector2	drawCoord;

	//float32	rotateAngle;

	//Vector2	scale;

	bool isPreparedForTiling;

	int32 modification;

	int32 flags;

	int32 resourceSizeIndex;

	float32 resourceToVirtualFactor;
	float32 resourceToPhysicalFactor;

	eSpriteType type;

//public:
	float32 **rectsAndOffsets;

	RenderDataStream * vertexStream;
	RenderDataStream * texCoordStream;
	ePrimitiveType primitiveToDraw;
	int32 vertexCount;

	// For rendering of clipped objects
	static Vector<Vector2> clippedTexCoords;
	static Vector<Vector2> clippedVertices;
	static bool spriteClipping;

	//static bool batchingEnabled;
	//static Vector<Vector2>
	
private:
	bool IsSpriteOnScreen(DrawState * state);
};


// inline functions implementation

inline void Sprite::DrawState::Reset()
{
	position.x = 0.0f;
	position.y = 0.0f;
	pivotPoint.x = 0.0f;
	pivotPoint.y = 0.0f;
	scale.x = 1.0f;
	scale.y = 1.0f;
	angle = 0.0f;
	//frame = 0;
	flags = 0;
	usePerPixelAccuracy = false;
	precomputedAngle = 0.0f;
	sinA = 0.0f;	// values for precomputed angle
	cosA = 1.0f;	// values for precomputed angle
}

inline void Sprite::DrawState::SetPosition(float32 x, float32 y)
{
	position.x = x;
	position.y = y;
}

inline void Sprite::DrawState::SetPosition(const Vector2 &drawPos)
{
    position = drawPos;
}

inline void Sprite::DrawState::SetPivotPoint(float32 x, float32 y)
{
	pivotPoint.x = x;
	pivotPoint.y = y;
}

inline void Sprite::DrawState::SetScale(float32 x, float32 y)
{
	scale.x = x;
	scale.y = y;
}

inline void Sprite::DrawState::SetScaleSize(float32 x, float32 y, float32 width, float32 height)
{
    scale.x = x / width;
	scale.y = y / height;
}


inline void Sprite::DrawState::SetAngle(float32 a)
{
	angle = a;
}

inline void Sprite::DrawState::SetFrame(uint32 _frame)
{
	frame = _frame;
}

inline void Sprite::DrawState::SetFlags(uint32 _flags)
{
	flags = _flags;
}

inline void Sprite::DrawState::SetPerPixelAccuracyUsage(bool needToUse)
{
	usePerPixelAccuracy = needToUse;
}

inline int32 Sprite::GetResourceSizeIndex() const
{
	return resourceSizeIndex;
}

inline float32 Sprite::GetResourceToPhysicalFactor() const
{
	return resourceToPhysicalFactor;
}

};//end of namespace

#endif