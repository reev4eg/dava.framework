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


#ifndef __DAVAENGINE_RENDERMANAGER_H__
#define __DAVAENGINE_RENDERMANAGER_H__

#include "Render/RenderBase.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/2D/Sprite.h"
#include "Platform/Mutex.h"
#include "Platform/Thread.h"
#include "Render/RenderEffect.h"
#include "Core/DisplayMode.h"
#include "Core/Core.h"
#include "Render/Cursor.h"
#include "Render/RenderState.h"
#include "Render/RenderOptions.h"
#include "Render/Shader.h"
#include "Render/UniqueStateSet.h"
#include "Render/RenderStateData.h"
#include "Render/TextureStateData.h"

#include <stack>


namespace DAVA
{

	
class Texture;
class Shader;
class RenderState;
class Image;
struct ScreenShotCallbackDelegate
{
    void operator()(Image *image)
    {
        return OnScreenShot(image);
    }

protected:
    virtual void OnScreenShot(Image *image) = 0;

};
/** 
	\ingroup render
	\brief Main class that implements rendering abstraction layer.
	
	If you want to write portable code you'll need to use this class and other classes that rely on our RenderManager. 
	Do not use native drawing functions, to avoid portability problems in the future. 
 */
class RenderManager : public Singleton<RenderManager>
{
public:
    static FastName FLAT_COLOR_SHADER;
    static FastName TEXTURE_MUL_FLAT_COLOR_SHADER;
    
    static Shader * FLAT_COLOR;
    static Shader * TEXTURE_MUL_FLAT_COLOR;
    static Shader * TEXTURE_MUL_FLAT_COLOR_ALPHA_TEST;
    static Shader * TEXTURE_MUL_FLAT_COLOR_IMAGE_A8;
    
    
    struct Caps
	{
		Caps() 
		{
			isHardwareCursorSupported = false;
			isPVRTCSupported = isETCSupported = isDXTSupported = isATCSupported = false;
			isBGRA8888Supported = isFloat16Supported = isFloat32Supported = false;
		}

        Core::eRenderer renderer;
		bool isHardwareCursorSupported;
        bool isPVRTCSupported;
        bool isETCSupported;
        bool isBGRA8888Supported;
        bool isFloat16Supported;
        bool isFloat32Supported;
		bool isDXTSupported;
		bool isATCSupported;
	};
    
    struct Stats
    {
        void Clear();
        
        uint32 drawArraysCalls;
        uint32 drawElementsCalls;
        uint32 shaderBindCount;
        uint32 occludedRenderBatchCount;
        uint32 primitiveCount[PRIMITIVETYPE_COUNT];
		
		uint32 renderStateSwitches;
		uint32 renderStateFullSwitches;
		uint32 textureStateFullSwitches;
		
		uint32 attachRenderDataCount;
        uint32 dynamicParamUniformBindCount;
        uint32 materialParamUniformBindCount;
        uint32 spriteDrawCount;
    };
    
    static void Create(Core::eRenderer renderer);

	RenderManager(Core::eRenderer renderer);
	~RenderManager();
    
    
    Core::eRenderer GetRenderer() { return renderer; };

	/** 
	 \brief Inits
	 \param[in] orientation
	 \param[in] screenWidth
	 \param[in] screenHeight
	 */
#ifdef __DAVAENGINE_OPENGL__ 
	void Init(int32 _frameBufferWidth, int32 _frameBufferHeight);
#ifdef __DAVAENGINE_ANDROID__    
	void InitFBSize(int32 _frameBufferWidth, int32 _frameBufferHeight);
#endif //    #ifdef __DAVAENGINE_ANDROID__    
#else
	void Init(int32 _frameBufferWidth, int32 _frameBufferHeight);
	LPDIRECT3D9		  GetD3D();
	LPDIRECT3DDEVICE9 GetD3DDevice();
	void SetupDefaultDeviceState();
	void OnDeviceLost();
	void OnDeviceRestore();
	HWND hWnd;
	HINSTANCE hInstance;
	D3DPRESENT_PARAMETERS	presentParams;

	LPDIRECT3DSURFACE9 backBufferSurface;
	LPDIRECT3DSURFACE9 depthStencilSurface;

#endif 

#ifdef __DAVAENGINE_WIN32__
	bool Create(HINSTANCE hInstance, HWND hWnd);
#else
	bool Create();
#endif 
	bool ChangeDisplayMode(DisplayMode mode, bool isFullscreen);
	
#if defined(__DAVAENGINE_ANDROID__)
	void Lost();
	void Invalidate();
#endif

	void Release();
	
	bool IsDeviceLost();
	void BeginFrame();
	void EndFrame();	
	bool IsInsideDraw(); // function returns true if we are between calls to BeginFrame & EndFrame

	// RenderManager capabilities & performance statistics
	void DetectRenderingCapabilities();
	const RenderManager::Caps & GetCaps();
    
    RenderManager::Stats & GetStats();
    void ClearStats();
    void EnableOutputDebugStatsEveryNFrame(int32 frameToShowDebugStats);
    void ProcessStats();
    
	/** 
	 \brief Init FBO system function
	 \param[in] viewRenderbuffer
	 \param[in] viewFramebuffer
	 */
	/*
     GLuint should be equal size as uint32
     */
	void InitFBO(uint32 viewRenderbuffer, uint32 viewFramebuffer);
#if defined (__DAVAENGINE_OPENGL__)
    void InitGL20();
    void ReleaseGL20();
    Shader * colorOnly;
    Shader * colorWithTexture;
#endif 
    
protected:
	RenderManager::Caps caps;
    RenderManager::Stats stats;
    int32 statsFrameCountToShowDebug;
    int32 frameToShowDebugStats;
    Core::eRenderer renderer;
	
	uint64 renderContextId;
    
public:
    
    /**
        === Render Manager threading functions 
     */
	/** 
	 \brief 
	 */
	void Lock();
	/** 
	 \brief 
	 */
	void Unlock();
	
    /**
     === Viewport and orientation 
     */

	/** 
	 \brief 
	 \param[in] orientation
	 */
	void SetRenderOrientation(int32 orientation);
	/** 
	 \brief 
	 \returns 
	 */
	int32 GetRenderOrientation();
    
    /**
        \brief 
        
     */
    void SetViewport(const Rect & rect, bool precaleulatedCoordinates);
    
    
    void SetCullOrder(eCullOrder cullOrder);

    const Rect & GetViewport()
    {
        return viewport;
    }

    /**
     *** State Management
     */

	/** 
	 \brief 
	 */
	void Reset();
	
	/** 
	 \brief 
	 */
	void FlushState();

	void FlushState(RenderState * stateBlock);
	
	/** 
	 \brief 
	 \param[in] r
	 \param[in] g
	 \param[in] b
	 \param[in] a
	 */
	void SetColor(float32 r, float32 g, float32 b, float32 a);
	void SetColor(const Color & color);
	float32 GetColorR() const;
	float32 GetColorG() const;
    float32 GetColorB() const;
    float32 GetColorA() const;
    const Color & GetColor() const;
	void ResetColor();

	// 
	//void SetTexture(Texture *texture, uint32 textureLevel = 0);
	//Texture * GetTexture(uint32 textureLevel = 0);
    void SetShader(Shader * shader);
    Shader * GetShader();
    
    
    void EnableVertexArray(bool isEnabled);
    void EnableNormalArray(bool isEnabled);
	void EnableTextureCoordArray(bool isEnabled, int32 textureLevel);
	void EnableColorArray(bool isEnabled);
	
	/*void EnableBlending(bool isEnabled);
	void EnableTexturing(bool isEnabled);

    bool IsDepthTestEnabled();
    bool IsDepthWriteEnabled();

    void EnableDepthTest(bool isEnabled);
    void EnableDepthWrite(bool isEnabled);
    
    void EnableAlphaTest(bool isEnabled);
    void EnableCulling(bool isEnabled);
    */
    
    
    void SetRenderData(RenderDataObject * object);
	void AttachRenderData();
    
	/** 
        \brief 
	 */
	void DrawArrays(ePrimitiveType type, int32 first, int32 count);
	void HWDrawArrays(ePrimitiveType type, int32 first, int32 count);
	
	void DrawElements(ePrimitiveType type, int32 count, eIndexFormat indexFormat, void * indices); 
	void HWDrawElements(ePrimitiveType type, int32 count, eIndexFormat indexFormat, void * indices); 
		
	/** 
	 \brief Sets the clip rect
	 \param[in] rect
	 */
	void SetClip(const Rect &rect);

	/** 
	 \brief Sets the clip rect as an intersection of the current rect and rect sent to method
	 \param[in] rect
	 */
	void ClipRect(const Rect &rect);
	
	/** 
	 \brief Sets clip yo the full screen
	 */
	void RemoveClip();

	/** 
	 \brief Store current clip
	 */
	void ClipPush();

	/** 
	 \brief Restore current screen
	 */
	void ClipPop();
    
    void Clear(const Color & color, float32 depth, int32 stencil);
	
	/** 
        \brief Clear rendering surface with required color 
        \param[in] r,g,b,a Clear color components
	 */
	void ClearWithColor(float32 r, float32 g, float32 b, float32 a);
    
    /** 
        \brief Clear attached depth buffer with requested depth
        \param[in] depth by default 1.0f, means clear the depth
     */
    void ClearDepthBuffer(float32 depth = 1.0f);

	/** 
        \brief Clear stencil buffer with requested value
        \param[in] stencil specifies the index used when the stencil buffer is cleared
     */
	void ClearStencilBuffer(int32 stencil = 0);

	/** 
	 \brief Sets the sprite to use as a render target. Sprite should be created with CreateAsRenderTarget method.
			Call RestoreRenderTarget when you finish drawing to your sprite 
	 \param[in] renderTarget - Render target sprite. If NULL 0 render manager will draw to the screen.
	 */
	void SetRenderTarget(Sprite *renderTarget);

	/** 
	 \brief Sets the texture to use as a render target. Texture should be created with CreateFBO method.
			Call RestoreRenderTarget when you finish drawing to your texture 
	 \param[in] renderTarget - Render target texture.
	 */
	void SetRenderTarget(Texture * renderTarget);

	/** 
        \brief Restores the previous render target
	 */
	void RestoreRenderTarget();

	/** 
	 \brief Checks is render target using for drawing now
	 \param[out] true if render manager sets to a render targe. false if render manager draws to the screen now
	 */
	bool IsRenderTarget();
	
	/** 
        \brief Sets the effect for the rendering. 
        \param[in] renderEffect - if 0, sets the effect to none
	 */
	void SetRenderEffect(Shader * shader);

	/** 
	 \brief Sets the requested framerate. For iPhone can be set to 60, 30, 20, 15
	 \param[in] newFps requested frames per second
	 */
	void SetFPS(int32 newFps);

	/** 
	 \brief Returns current requested framerate
	 \returns frames per second
	 */
    int32 GetFPS();

	void SetDebug(bool isDebugEnabled);


	/** 
	 \brief 
	 \param[in] offset
	 */
	void SetDrawTranslate(const Vector2 &offset);
    
	void SetDrawTranslate(const Vector3 &offset);

	/** 
	 \brief 
	 \param[in] offset
	 */
	void SetDrawScale(const Vector2 &scale);

	void IdentityDrawMatrix();
	void IdentityMappingMatrix();
	void IdentityModelMatrix();
	
	/*
		TODO:	Hottych - напиши пожалуйста что делают эти функции детально, 
				чтобы было понятно как и в каких случаях их надо использовать
				Думаю что пока воспоминания свежи, напиши документацию по системе виртуальных преобразований
				Можешь писать на русском - я переведу потом.
	 */
	void SetPhysicalViewScale();
	void SetPhysicalViewOffset();
	void SetVirtualViewScale();
	void SetVirtualViewOffset();

	void PushDrawMatrix();
    void PopDrawMatrix();

    void PushMappingMatrix();
	void PopMappingMatrix();
    
    void SetRenderContextId(uint64 contextId);
	uint64 GetRenderContextId();
	void VerifyRenderContext();
        
    /*  
        Matrix support
     */
    enum eMatrixType
    {
        MATRIX_MODELVIEW = 0,
        MATRIX_PROJECTION,
        MATRIX_COUNT,
    };
    
    enum eUniformMatrixType
    {
        UNIFORM_MATRIX_MODELVIEWPROJECTION = 0,
        UNIFORM_MATRIX_NORMAL,
        UNIFORM_MATRIX_COUNT,
    };
    
    static AutobindVariableData dynamicParameters[DYNAMIC_PARAMETERS_COUNT];
    static uint32  dynamicParamersRequireUpdate;
    static Matrix4 worldViewMatrix;
    static Matrix4 viewProjMatrix;
    static Matrix4 worldViewProjMatrix;
    static Matrix4 invWorldViewMatrix;
    static Matrix3 normalMatrix;
    static Matrix4 invWorldMatrix;
    static Matrix3 worldInvTransposeMatrix;

    static inline void SetDynamicParam(eShaderSemantic shaderSemantic, const void * value, uint32 updateSemantic);
    
    //void SetMatrix(eShaderSemantic type, void * value, uint32 updateSemantic);
    //void SetMatrix(eShaderSemantic type, const Matrix4 & matrix, uint32 cacheValue);

    static inline const void * GetDynamicParam(eShaderSemantic shaderSemantic);
    static inline const Matrix4 & GetDynamicParamMatrix(eShaderSemantic shaderSemantic);
    static inline void ComputeWorldViewMatrixIfRequired();
    static inline void ComputeViewProjMatrixIfRequired();
    static inline void ComputeWorldViewProjMatrixIfRequired();
    static inline void ComputeInvWorldViewMatrixIfRequired();
    static inline void ComputeWorldViewInvTransposeMatrixIfRequired();
    
    static inline void ComputeInvWorldMatrixIfRequired();
    static inline void ComputeWorldInvTransposeMatrixIfRequired();

    
    //const Matrix4 & GetUniformMatrix(eUniformMatrixType type);
    //const Matrix3 & GetNormalMatrix();
    //void  ClearUniformMatrices();


	/**
		\brief This function sets hardware cursor to render manager
		It acts differently in different operation systems but idea is common.
		When you call this function on next refresh cursor will be changed to the new one.
	*/
	void SetCursor(Cursor * cursor);

	/**
		\brief This function get hardware cursor that actively set
		By default at application launch function returns zero. If this function returns zero 
		we use default cursor that is provided by operational system. 
		\returns pointer to custom cursor or null if there is no cursor set by default.
	 */
	Cursor * GetCursor();

	RenderOptions * GetOptions();

    uint32 GetFBOViewFramebuffer() const;
    
#if defined(__DAVAENGINE_OPENGL__)
    void HWglBindBuffer(GLenum target, GLuint  	buffer);
    GLuint bufferBindingId[2];
    
    int32 HWglGetLastTextureID(int textureType);
	void HWglBindTexture(int32 tId, uint32 textureType = Texture::TEXTURE_2D);
	void HWglForceBindTexture(int32 tId, uint32 textureType = Texture::TEXTURE_2D);
    int32 lastBindedTexture[Texture::TEXTURE_TYPE_COUNT];
	uint32 lastBindedTextureType;

    
    int32 HWglGetLastFBO();
    void HWglBindFBO(const int32 fbo);
    int32 lastBindedFBO;
#endif //#if defined(__DAVAENGINE_OPENGL__)
    
    void RequestGLScreenShot(ScreenShotCallbackDelegate *screenShotCallback);
	
	inline void RetainRenderState(UniqueHandle handle)
	{
		uniqueRenderStates.RetainUnique(handle);
	}
	
	inline UniqueHandle CreateRenderState(const RenderStateData& data)
	{
		return uniqueRenderStates.MakeUnique(data);
	}

	inline const RenderStateData& GetRenderStateData(UniqueHandle handle)
	{
		return uniqueRenderStates.GetUnique(handle);
	}
	
	inline void ReleaseRenderState(UniqueHandle handle)
	{
		uniqueRenderStates.ReleaseUnique(handle);
	}
		
			
	inline UniqueHandle SubclassRenderState(UniqueHandle parentStateHandle, uint32 renderStateFlags)
	{
		const RenderStateData& parentState = RenderManager::Instance()->GetRenderStateData(parentStateHandle);
		RenderStateData derivedState;
		memcpy(&derivedState, &parentState, sizeof(derivedState));
		
		derivedState.state = renderStateFlags;
		return CreateRenderState(derivedState);
	}
	
	inline UniqueHandle SubclassRenderState(UniqueHandle parentStateHandle,
										  eBlendMode srcBlend,
										  eBlendMode dstBlend)
	{
		const RenderStateData& parentState = RenderManager::Instance()->GetRenderStateData(parentStateHandle);
		RenderStateData derivedState;
		memcpy(&derivedState, &parentState, sizeof(derivedState));
		
		derivedState.sourceFactor = srcBlend;
		derivedState.destFactor = dstBlend;
		return CreateRenderState(derivedState);
	}

	inline UniqueHandle Subclass3DRenderState(eBlendMode srcBlend,
										  eBlendMode dstBlend)
	{
		return SubclassRenderState(RenderState::RENDERSTATE_3D_BLEND, srcBlend, dstBlend);
	}
	
	inline UniqueHandle Subclass3DRenderState(uint32 renderStateFlags)
	{
		return SubclassRenderState(RenderState::RENDERSTATE_3D_BLEND, renderStateFlags);
	}
	
	inline UniqueHandle Subclass2DRenderState(uint32 renderStateFlags)
	{
		return SubclassRenderState(RenderState::RENDERSTATE_2D_BLEND, renderStateFlags);
	}
	
	inline void SetRenderState(UniqueHandle requestedState)
	{
		currentState.stateHandle = requestedState;
	}
	
	inline UniqueHandle CreateTextureState(const TextureStateData& data)
	{
		return uniqueTextureStates.MakeUnique(data);
	}
	
	inline const TextureStateData& GetTextureState(UniqueHandle handle)
	{
		return uniqueTextureStates.GetUnique(handle);
	}

    inline void RetainTextureState(UniqueHandle handle)
	{
		uniqueTextureStates.RetainUnique(handle);
	}
	
	inline void ReleaseTextureState(UniqueHandle handle)
	{
		uniqueTextureStates.ReleaseUnique(handle);
	}

	inline void SetTextureState(UniqueHandle requestedState)
	{
		currentState.textureState = requestedState;
	}
		
protected:
    //
    // general matrices for rendermanager 
    // 
    
//    Matrix4 matrices[MATRIX_COUNT];
//    uint32 projectionMatrixCache;
//    uint32 modelViewMatrixCache;
//    int32   uniformMatrixFlags[UNIFORM_MATRIX_COUNT];
//    Matrix4 uniformMatrices[UNIFORM_MATRIX_COUNT];
//    Matrix3 uniformMatrixNormal;
//    

    //do nothing right now
    DAVA_DEPRECATED(void RectFromRenderOrientationToViewport(Rect & rect));
    
	// SHOULD BE REPLACED WITH MATRICES IN FUTURE
	Vector2 userDrawOffset;
	Vector2 userDrawScale;

	//need to think about optimization two matrices (userDraw matrix and mapping matrix) to the one matrix
	Vector2 viewMappingDrawOffset;
	Vector2 viewMappingDrawScale;

	Vector2 currentDrawOffset;
	Vector2 currentDrawScale;
    
    bool mappingMatrixChanged;
	
	void PrepareRealMatrix();
    
    struct Renderer2D
    {
        
        Matrix4 viewMatrix;
        Matrix4 projMatrix;
        Matrix4 viewProjMatrix;
        
	};
    Renderer2D renderer2d;
public:
    void Setup2DMatrices();
    Renderer2D * GetRenderer2D() { return &renderer2d; };

    
    
	/** 
	 \brief 
	 \returns 
	 */
	int32 GetScreenWidth();
	
	/** 
	 \brief 
	 \returns 
	 */
	int32 GetScreenHeight();
	
	
	typedef struct RenderTarget_t 
		{
			Sprite *spr;
			int orientation;
		} RenderTarget;

	typedef struct DrawMatrix_t 
	{
		Vector2 userDrawOffset;
		Vector2 userDrawScale;
	} DrawMatrix;
	

	// fbo data
	uint32 fboViewRenderbuffer;
	uint32 fboViewFramebuffer;

	// state information
//	Color oldColor;                 // UNIFORM - can be used or not used by RenderEffect
//	Color newColor;                 // UNIFORM - can be used or not used by RenderEffect
    
//	eBlendMode oldSFactor, oldDFactor;  // STATE
//	eBlendMode newSFactor, newDFactor;  // STATE
    
//  static const uint32 MAX_TEXTURE_LEVELS = 4;
//	Texture *currentTexture[MAX_TEXTURE_LEVELS];                        // Texture that was set
//  Shader * shader;
	
	UniqueStateSet<RenderStateData> uniqueRenderStates;
	UniqueStateSet<TextureStateData> uniqueTextureStates;
    
    void InitDefaultRenderStates();
	   
    RenderState currentState;
    RenderState hardwareState;

    int32 enabledAttribCount;

    
    
    int32 renderOrientation;
	Sprite *currentRenderTarget;
	std::stack<Rect> clipStack;
	std::stack<RenderTarget> renderTargetStack;
	std::stack<DrawMatrix> matrixStack;
	std::stack<DrawMatrix> mappingMatrixStack;

	Shader * currentRenderEffect;
	
    RenderDataObject * currentRenderData;

    Rect viewport;
    
    /* 
        Size of the original renderTarget created for 3D rendering.
    */
    
	int32 frameBufferWidth;
	int32 frameBufferHeight;
	int32 retScreenWidth;
	int32 retScreenHeight;
	
	int32 fps;

	bool isInsideDraw;

	Mutex glMutex;
	
	Rect currentClip;
	
	void SetHWClip(const Rect &rect);
	void SetHWRenderTargetSprite(Sprite *renderTarget);
	void SetHWRenderTargetTexture(Texture * renderTarget);
	
	bool debugEnabled;

	RenderOptions options;
	
#if defined(__DAVAENGINE_DIRECTX9__)
	// 
	bool scissorTestEnabled;
	struct BufferData
	{	
		BufferData()
		{	
			isEnabled = false;
			size = 0;
			type = TYPE_FLOAT;
			stride = 0;
			pointer = 0;
		}


		bool isEnabled;
		int32 size;
		eVertexDataType	type;
		int32 stride;
		const void * pointer;
	};
	enum
	{
		BUFFER_VERTEX = 0,
		BUFFER_COLOR,
		BUFFER_TEXCOORD0,
		BUFFER_TEXCOORD1,
		BUFFER_TEXCOORD2,
		BUFFER_TEXCOORD3,
		BUFFER_NORMAL,

		BUFFER_COUNT
	};

	BufferData buffers[BUFFER_COUNT];
	D3DCAPS9 deviceCaps;
#endif // #if defined(__DAVAENGINE_DIRECTX9__)
	
	Cursor * cursor;
    
    bool needGLScreenShot;
    ScreenShotCallbackDelegate *screenShotCallback;
    void MakeGLScreenShot();
};
    
inline void RenderManager::SetDynamicParam(eShaderSemantic shaderSemantic, const void * value, uint32 _updateSemantic)
{
    //AutobindVariableData * var = &dynamicParameters[shaderSemantic];
    //if (var->updateSemantic
    if (_updateSemantic == UPDATE_SEMANTIC_ALWAYS || dynamicParameters[shaderSemantic].updateSemantic != _updateSemantic)
    {
        
        if (_updateSemantic == UPDATE_SEMANTIC_ALWAYS)
            dynamicParameters[shaderSemantic].updateSemantic++;
        else
            dynamicParameters[shaderSemantic].updateSemantic = _updateSemantic;
        
        dynamicParameters[shaderSemantic].value = value;
        dynamicParamersRequireUpdate &= ~(1 << shaderSemantic);
        
//        PARAM_WORLD,
//        PARAM_INV_WORLD,
//        PARAM_VIEW,
//        PARAM_INV_VIEW,
//        PARAM_PROJ,
//        PARAM_INV_PROJ,
//        
//        PARAM_WORLD_VIEW,
//        PARAM_INV_WORLD_VIEW,
//        PARAM_NORMAL, // NORMAL MATRIX
//        
//        PARAM_VIEW_PROJ,
//        PARAM_INV_VIEW_PROJ,
//        
//        PARAM_WORLD_VIEW_PROJ,
//        PARAM_INV_WORLD_VIEW_PROJ,
        switch(shaderSemantic)
        {
            case PARAM_WORLD:
                dynamicParamersRequireUpdate |= ((1 << PARAM_INV_WORLD) | ( 1 << PARAM_WORLD_VIEW) | (1 << PARAM_INV_WORLD_VIEW)
                                                 | ( 1 << PARAM_WORLD_VIEW_PROJ) | (1 << PARAM_INV_WORLD_VIEW_PROJ) | (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE) | (1 << PARAM_WORLD_INV_TRANSPOSE));
            break;
            case PARAM_VIEW:
                dynamicParamersRequireUpdate |= (   (1 << PARAM_INV_VIEW)
                                                 |  (1 << PARAM_WORLD_VIEW)
                                                 |  (1 << PARAM_INV_WORLD_VIEW)
                                                 |  (1 << PARAM_WORLD_VIEW_PROJ)
                                                 |  (1 << PARAM_INV_WORLD_VIEW_PROJ)
                                                 |  (1 << PARAM_VIEW_PROJ)
                                                 |  (1 << PARAM_INV_VIEW_PROJ)
                                                 |  (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE) );
            break;
            case PARAM_PROJ:
                dynamicParamersRequireUpdate |= ((1 << PARAM_INV_PROJ) | (1 << PARAM_VIEW_PROJ) | (1 << PARAM_INV_VIEW_PROJ) |
                                                 (1 << PARAM_WORLD_VIEW_PROJ) | (1 << PARAM_INV_WORLD_VIEW_PROJ));
            break;
            default:
            break;
        }
        
    }
}

inline const Matrix4 & RenderManager::GetDynamicParamMatrix(eShaderSemantic shaderSemantic)
{
    return *(Matrix4*)dynamicParameters[shaderSemantic].value;
}
    
inline void RenderManager::ComputeWorldViewMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW))
    {
        worldViewMatrix = GetDynamicParamMatrix(PARAM_WORLD) * GetDynamicParamMatrix(PARAM_VIEW);
        SetDynamicParam(PARAM_WORLD_VIEW, &worldViewMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
    
inline void RenderManager::ComputeViewProjMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_VIEW_PROJ))
    {
        viewProjMatrix = GetDynamicParamMatrix(PARAM_VIEW) * GetDynamicParamMatrix(PARAM_PROJ);
        SetDynamicParam(PARAM_VIEW_PROJ, &viewProjMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void RenderManager::ComputeWorldViewProjMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW_PROJ))
    {
        ComputeViewProjMatrixIfRequired();
        worldViewProjMatrix = GetDynamicParamMatrix(PARAM_WORLD) * GetDynamicParamMatrix(PARAM_VIEW_PROJ);
        SetDynamicParam(PARAM_WORLD_VIEW_PROJ, &worldViewProjMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
    
inline void RenderManager::ComputeInvWorldViewMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_INV_WORLD_VIEW))
    {
        ComputeWorldViewMatrixIfRequired();
        worldViewMatrix.GetInverse(invWorldViewMatrix);
        SetDynamicParam(PARAM_INV_WORLD_VIEW, &invWorldViewMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
    
inline void RenderManager::ComputeWorldViewInvTransposeMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE))
    {
        ComputeInvWorldViewMatrixIfRequired();
        normalMatrix = invWorldViewMatrix;
        normalMatrix.Transpose();
        SetDynamicParam(PARAM_WORLD_VIEW_INV_TRANSPOSE, &normalMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
    
    
inline void RenderManager::ComputeInvWorldMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_INV_WORLD))
    {
        const Matrix4 & worldMatrix = GetDynamicParamMatrix(PARAM_WORLD);
        worldMatrix.GetInverse(invWorldMatrix);
        SetDynamicParam(PARAM_INV_WORLD, &invWorldMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
    
inline void RenderManager::ComputeWorldInvTransposeMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_INV_TRANSPOSE))
    {
        ComputeInvWorldMatrixIfRequired();
        worldInvTransposeMatrix = invWorldMatrix;
        worldInvTransposeMatrix.Transpose();
        SetDynamicParam(PARAM_WORLD_INV_TRANSPOSE, &worldInvTransposeMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}
 
const void * RenderManager::GetDynamicParam(eShaderSemantic shaderSemantic)
{
    DVASSERT(dynamicParameters[shaderSemantic].value != 0);
    return dynamicParameters[shaderSemantic].value;
}

#define SET_DYNAMIC_PARAM(x, y, z) RenderManager::SetDynamicParam(x, y, z)
#define GET_DYNAMIC_PARAM(x) RenderManager::dynamicParameters[x]
#define GET_DYNAMIC_PARAM_VALUE(x) RenderManager::dynamicParameters[x].value
#define GET_DYNAMIC_PARAM_UPDATE_SEMANTIC(x) RenderManager::dynamicParameters[x].updateSemantic
    
// Update debug stats only in debug.
#if defined(__DAVAENGINE_DEBUG__)
#define RENDERER_UPDATE_STATS(param) RenderManager::Instance()->GetStats().param
#else
#define RENDERER_UPDATE_STATS(param)
#endif
    
};
#endif