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



#include "WebViewControlWin32.h"
#include "CorePlatformWin32.h"
using namespace DAVA;

#pragma warning(push)
#pragma warning(disable: 4717) //'ATL::CCRTAllocator::free' : recursive on all control paths, function will cause runtime stack overflow
#include <atlbase.h>
#pragma warning(pop)
#include <atlcom.h>
#include <ExDisp.h>
#include <ExDispid.h>
#include "Utils/Utils.h"

#include <ObjIdl.h>
#include <Shlwapi.h>

extern _ATL_FUNC_INFO BeforeNavigate2Info;
_ATL_FUNC_INFO BeforeNavigate2Info = {CC_STDCALL, VT_EMPTY, 7, {VT_DISPATCH,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_BOOL}};

extern _ATL_FUNC_INFO DocumentCompleteInfo;
_ATL_FUNC_INFO DocumentCompleteInfo =  {CC_STDCALL,VT_EMPTY,2,{VT_DISPATCH,VT_BYREF | VT_VARIANT}};
namespace DAVA 
{

template <class T>
class ScopedComPtr {
protected:
    T *ptr;
public:
    ScopedComPtr() : ptr(NULL) { }
    explicit ScopedComPtr(T *ptr) : ptr(ptr) { }
    ~ScopedComPtr() {
        if (ptr)
            ptr->Release();
    }
    bool Create(const CLSID clsid) {
        CrashIf(ptr);
        if (ptr) return false;
        HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_PPV_ARGS(&ptr));
        return SUCCEEDED(hr);
    }
    operator T*() const { return ptr; }
    T** operator&() { return &ptr; }
    T* operator->() const { return ptr; }
    T* operator=(T* newPtr) {
        if (ptr)
            ptr->Release();
        return (ptr = newPtr);
    }
};

template <class T>
class ScopedComQIPtr : public ScopedComPtr<T> {
public:
    ScopedComQIPtr() : ScopedComPtr() { }
    explicit ScopedComQIPtr(IUnknown *unk) {
        HRESULT hr = unk->QueryInterface(&ptr);
        if (FAILED(hr))
            ptr = NULL;
    }
    T* operator=(IUnknown *newUnk) {
        if (ptr)
            ptr->Release();
        HRESULT hr = newUnk->QueryInterface(&ptr);
        if (FAILED(hr))
            ptr = NULL;
        return ptr;
    }
};

class HtmlMoniker : public IMoniker
{
public:
    HtmlMoniker() :
        refCount(1),
        htmlStream(NULL)
    {

    }

    virtual ~HtmlMoniker()
    {
        if (htmlStream)
            htmlStream->Release();
    }

    HRESULT HtmlMoniker::SetHtml(const String& _htmlData)
    {
        htmlData = _htmlData;
        if (htmlStream)
            htmlStream->Release();
        htmlStream = CreateStreamFromData(htmlData.c_str(), htmlData.size());
        return S_OK;
    }

    HRESULT HtmlMoniker::SetBaseUrl(const WideString& _baseUrl)
    {
        baseUrl = _baseUrl;
        return S_OK;
    }
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
    {
        static const QITAB qit[] = {
            QITABENT(HtmlMoniker, IMoniker),
            QITABENT(HtmlMoniker, IPersistStream),
            QITABENT(HtmlMoniker, IPersist),
            { 0 }
        };
        return QISearch(this, qit, riid, ppvObject);
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&refCount);
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        LONG res = InterlockedDecrement(&refCount);
        DVASSERT(res >= 0);
        if (0 == res)
            delete this;
        return res;
    }

    // IMoniker
    STDMETHODIMP BindToStorage(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj)
    {
        LARGE_INTEGER seek = {0};
        htmlStream->Seek(seek, STREAM_SEEK_SET, NULL);
        return htmlStream->QueryInterface(riid, ppvObj);
    }

    STDMETHODIMP GetDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
    {
        if (!ppszDisplayName)
            return E_POINTER;
        DVASSERT(!baseUrl.empty())

        *ppszDisplayName = OleStrDup(baseUrl.c_str(), baseUrl.length());

        return S_OK;
    }

    STDMETHODIMP BindToObject(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riidResult, void **ppvResult) { return E_NOTIMPL; }
    STDMETHODIMP Reduce(IBindCtx *pbc, DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced) { return E_NOTIMPL; }
    STDMETHODIMP ComposeWith(IMoniker *pmkRight, BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite) { return E_NOTIMPL; }
    STDMETHODIMP Enum(BOOL fForward, IEnumMoniker **ppenumMoniker) { return E_NOTIMPL; }
    STDMETHODIMP IsEqual(IMoniker *pmkOtherMoniker) { return E_NOTIMPL; }
    STDMETHODIMP Hash(DWORD *pdwHash) { return E_NOTIMPL; }
    STDMETHODIMP IsRunning(IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning) { return E_NOTIMPL; }
    STDMETHODIMP GetTimeOfLastChange(IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime) { return E_NOTIMPL; }
    STDMETHODIMP Inverse(IMoniker **ppmk) { return E_NOTIMPL; }
    STDMETHODIMP CommonPrefixWith(IMoniker *pmkOther, IMoniker **ppmkPrefix) { return E_NOTIMPL; }
    STDMETHODIMP RelativePathTo(IMoniker *pmkOther, IMoniker **ppmkRelPath) { return E_NOTIMPL; }
    STDMETHODIMP ParseDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft,LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut) { return E_NOTIMPL; }

    STDMETHODIMP IsSystemMoniker(DWORD *pdwMksys) {
        if (!pdwMksys)
            return E_POINTER;
        *pdwMksys = MKSYS_NONE;
        return S_OK;
    }

    // IPersistStream methods
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty)  { return E_NOTIMPL; }
    STDMETHODIMP IsDirty() { return E_NOTIMPL; }
    STDMETHODIMP Load(IStream *pStm) { return E_NOTIMPL; }
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize) { return E_NOTIMPL; }

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID) { return E_NOTIMPL; }

private:
    LPOLESTR OleStrDup(const WCHAR *s, int len)
    {
        size_t cb = sizeof(WCHAR) * (len + 1);
        LPOLESTR ret = (LPOLESTR)CoTaskMemAlloc(cb);
        memcpy(ret, s, cb);
        return ret;
    }

    IStream *CreateStreamFromData(const void *data, size_t len)
    {
        if (!data)
            return NULL;

        ScopedComPtr< IStream > stream;
        if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &stream)))
            return NULL;

        ULONG written;
        if (FAILED(stream->Write(data, (ULONG)len, &written)) || written != len)
            return NULL;

        LARGE_INTEGER zero = { 0 };
        stream->Seek(zero, STREAM_SEEK_SET, NULL);

        stream->AddRef();

        return stream;
    }

    LONG                refCount;
    String        htmlData;
    IStream *           htmlStream;
    WideString    baseUrl;
};

struct EventSink : public IDispEventImpl<1, EventSink, &DIID_DWebBrowserEvents2>
{
private:
	IUIWebViewDelegate* delegate;
	UIWebView* webView;
    WebBrowserContainer* container;
public:
	EventSink()
	{
		delegate = NULL;
		webView = NULL;
		container = NULL;
	};

	void SetDelegate(IUIWebViewDelegate *delegate, UIWebView* webView, WebBrowserContainer* container)
	{
		if (delegate && webView && container)
		{
			this->delegate = delegate;
			this->webView = webView;
            this->container = container;
		}
	}

	void  __stdcall DocumentComplete(IDispatch* pDisp, VARIANT* URL)
	{
		if (delegate && webView)
		{
            if (!container->DoOpenBuffer())
                delegate->PageLoaded(webView);
		}
	}

	void __stdcall BeforeNavigate2(IDispatch* pDisp, VARIANT* URL, VARIANT* Flags,
								   VARIANT* TargetFrameName, VARIANT* PostData,
								   VARIANT* Headers, VARIANT_BOOL* Cancel)
	{
		bool process = true;

		if (delegate && webView)
		{
			BSTR bstr = V_BSTR(URL);
			int32 len = SysStringLen(bstr) + 1;
			char* str = new char[len];
			WideCharToMultiByte(CP_ACP, 0, bstr, -1, str, len, NULL, NULL);
			String s = str;
			delete[] str;
			bool isRedirectedByMouseClick  = Flags->intVal == navHyperlink ;
			IUIWebViewDelegate::eAction action = delegate->URLChanged(webView, s, isRedirectedByMouseClick);

			switch (action)
			{
				case IUIWebViewDelegate::PROCESS_IN_WEBVIEW:
					Logger::FrameworkDebug("PROCESS_IN_WEBVIEW");
					break;

				case IUIWebViewDelegate::PROCESS_IN_SYSTEM_BROWSER:
					Logger::FrameworkDebug("PROCESS_IN_SYSTEM_BROWSER");
					process = false;
					ShellExecute(NULL, L"open", bstr, NULL, NULL, SW_SHOWNORMAL);
					break;

				case IUIWebViewDelegate::NO_PROCESS:
					Logger::FrameworkDebug("NO_PROCESS");

				default:
					process = false;
					break;
			}
		}

		*Cancel = process ? VARIANT_FALSE : VARIANT_TRUE;
	}

	BEGIN_SINK_MAP(EventSink)
		SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, BeforeNavigate2, &BeforeNavigate2Info)
		SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, DocumentComplete, &DocumentCompleteInfo)
	END_SINK_MAP()
};


WebBrowserContainer::WebBrowserContainer() :
	hwnd(0),
	webBrowser(NULL),
    openFromBufferQueued(false)
{
}

WebBrowserContainer::~WebBrowserContainer()
{
	EventSink* s = (EventSink*)sink;
	s->DispEventUnadvise(webBrowser, &DIID_DWebBrowserEvents2);
	delete s;

	if (webBrowser)
	{
		webBrowser->Release();
		webBrowser = NULL;
	}
}

void WebBrowserContainer::SetDelegate(IUIWebViewDelegate *delegate, UIWebView* webView)
{
	EventSink* s = (EventSink*)sink;
	s->SetDelegate(delegate, webView, this);
}

bool WebBrowserContainer::Initialize(HWND parentWindow)
{
	this->hwnd = parentWindow;

	IOleObject* oleObject = NULL;
	HRESULT hRes = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_INPROC, IID_IOleObject, (void**)&oleObject);
	if (FAILED(hRes))
	{
		Logger::Error("WebBrowserContainer::Inititalize(), CoCreateInstance(CLSID_WebBrowser) failed!, error code %i", hRes);
		return false;
	}

	hRes = oleObject->SetClientSite(this);
	if (FAILED(hRes))
	{
		Logger::Error("WebBrowserContainer::Inititalize(), IOleObject::SetClientSite() failed!, error code %i", hRes);
		oleObject->Release();
		return false;
	}

	// Activating the container.
	RECT rect = {0};
	GetClientRect(hwnd, &rect);
	hRes = oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, this->hwnd, &rect);
	if (FAILED(hRes))
	{
		Logger::Error("WebBrowserContainer::InititalizeBrowserContainer(), IOleObject::DoVerb() failed!, error code %i", hRes);
		oleObject->Release();
		return false;
	}

	// Prepare the browser itself.
	hRes = oleObject->QueryInterface(IID_IWebBrowser2, (void**)&this->webBrowser);
	if (FAILED(hRes))
	{
		Logger::Error("WebViewControl::InititalizeBrowserContainer(), IOleObject::QueryInterface(IID_IWebBrowser2) failed!, error code %i", hRes);
		oleObject->Release();
		return false;
	}

	sink = new EventSink();
	EventSink* s = (EventSink*)sink;
	hRes = s->DispEventAdvise(webBrowser, &DIID_DWebBrowserEvents2);
	if (FAILED(hRes))
	{
		Logger::Error("WebViewControl::InititalizeBrowserContainer(), EventSink::DispEventAdvise(&DIID_DWebBrowserEvents2) failed!, error code %i", hRes);
		return false;
	}

	// Initialization is OK.
	oleObject->Release();
	return true;
}

HRESULT __stdcall WebBrowserContainer::QueryInterface(REFIID riid, void** ppvObject)
{
	if( !ppvObject )
	{
		return E_POINTER;
	}

	if( riid==IID_IUnknown || riid==IID_IOleWindow || riid==IID_IOleInPlaceSite )
	{
		return *ppvObject = (void*)static_cast<IOleInPlaceSite*>(this), S_OK;
	}

	if( riid==IID_IOleClientSite )
	{
		return *ppvObject = (void*)static_cast<IOleClientSite*>(this), S_OK;
	}

	*ppvObject = NULL;
	return E_NOINTERFACE;
}

HRESULT __stdcall WebBrowserContainer::GetWindow(HWND *phwnd)
{
	if (!phwnd)
	{
		return E_INVALIDARG;
	}

	*phwnd = this->hwnd;
	return S_OK;
}

HRESULT __stdcall WebBrowserContainer::GetWindowContext( IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc,
		LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	if( !(ppFrame && ppDoc && lprcPosRect && lprcClipRect && lpFrameInfo) )
	{
		return E_INVALIDARG;
	}

	*ppFrame = NULL;
	*ppDoc = NULL;

	GetClientRect( this->hwnd, lprcPosRect );
	GetClientRect( this->hwnd, lprcClipRect );

	lpFrameInfo->fMDIApp = false;
	lpFrameInfo->hwndFrame = this->hwnd;
	lpFrameInfo->haccel = 0;
	lpFrameInfo->cAccelEntries = 0;

	return S_OK;
}

bool WebBrowserContainer::OpenUrl(const WCHAR* urlToOpen)
{
	if (!webBrowser)
	{
		return false;
	}

    openFromBufferQueued = false;
    bufferToOpen = "";

	BSTR url = SysAllocString(urlToOpen);
	VARIANT empty = {0};
	VariantInit(&empty);

	webBrowser->Navigate(url, &empty, &empty, &empty, &empty);
	SysFreeString(url);

	return true;
}

bool WebBrowserContainer::LoadHtmlString(LPCTSTR pszHTMLContent)
{
	if (!webBrowser || !pszHTMLContent)
	{
		return false;
	}
	// Initialize html document
	webBrowser->Navigate( L"about:blank", NULL, NULL, NULL, NULL); 

	IDispatch * m_pDoc;
	IStream * pStream = NULL;
	IPersistStreamInit * pPSI = NULL;
	HGLOBAL hHTMLContent;
	HRESULT hr;
	bool bResult = false;

	// allocate global memory to copy the HTML content to
	hHTMLContent = ::GlobalAlloc( GPTR, ( ::_tcslen( pszHTMLContent ) + 1 ) * sizeof(TCHAR) );
	if (!hHTMLContent)
		return false;

	::_tcscpy( (TCHAR *) hHTMLContent, pszHTMLContent );

	// create a stream object based on the HTML content
	hr = ::CreateStreamOnHGlobal( hHTMLContent, TRUE, &pStream );
	if (SUCCEEDED(hr))
	{

		IDispatch * pDisp = NULL;

		// get the document's IDispatch*
		hr = this->webBrowser->get_Document( &pDisp );
		if (SUCCEEDED(hr))
		{
			m_pDoc = pDisp;
		}
		else
		{
			return false;
		}

		// request the IPersistStreamInit interface
		hr = m_pDoc->QueryInterface( IID_IPersistStreamInit, (void **) &pPSI );

		if (SUCCEEDED(hr))
		{
			// initialize the persist stream object
			hr = pPSI->InitNew();

			if (SUCCEEDED(hr))
			{
				// load the data into it
				hr = pPSI->Load( pStream );

				if (SUCCEEDED(hr))
				{
					bResult = true;
				}
			}

			pPSI->Release();
		}

		// implicitly calls ::GlobalFree to free the global memory
		pStream->Release();
	}

	return bResult;
}

String WebBrowserContainer::GetCookie(const String& targetUrl, const String& name)
{
	if (!webBrowser)
	{
		return String();
	}

	LPTSTR lpszData = NULL;   // buffer to hold the cookie data
	DWORD dwSize = 4096; // Initial size of buffer		
	String retCookie;

	if (GetInternetCookies(targetUrl, name, lpszData, dwSize))
	{
		retCookie = WStringToString(lpszData);

		Vector<String> cookie;
		Split(retCookie, "=", cookie);
		// Get only cookie value
		if (cookie.size() == 2)
		{
			retCookie = cookie[1];
		}
	}
	else
	{
		delete [] lpszData;
	}

	return retCookie;
}

Map<String, String> WebBrowserContainer::GetCookies(const String& targetUrl)
{
	if (!webBrowser)
	{
		return Map<String, String>();
	}

	LPTSTR lpszData = NULL;   // buffer to hold the cookie data
	DWORD dwSize = 4096; // Initial size of buffer
	Map<String, String> cookiesMap;

	if (GetInternetCookies(targetUrl, "", lpszData, dwSize))
	{
		String cookiesString = WStringToString(lpszData);
		// Split cookies string into vector - each value corresponds to one cookie name-value pair
		Vector<String> cookiesVector;
		Split(cookiesString, ";", cookiesVector);

		for (uint32 i = 0; i < (int32)cookiesVector.size(); ++i)
		{
			Vector<String> cookie;
			Split(cookiesVector[i], "=", cookie);
			// Add cookie to resulting map
			if (cookie.size() == 2)
			{
				String cookieName = cookie[0];
				// Remove all spaces in cookie name
				cookieName.erase(std::remove(cookieName.begin(), cookieName.end(), ' '), cookieName.end());
				cookiesMap[cookieName] = cookie[1];
			}
		}
	}
	else
	{
		delete [] lpszData;
	}

	return cookiesMap;
}

bool WebBrowserContainer::GetInternetCookies(const String& targetUrl, const String& name, LPTSTR &lpszData, DWORD &dwSize)
{
	// Setup initial cache entry size
	lpszData = new TCHAR[dwSize];

	BOOL bResult = InternetGetCookieEx(StringToWString(targetUrl).c_str(), 
											name.empty() ? NULL : StringToWString(name).c_str(),
											lpszData,
											&dwSize,
											INTERNET_COOKIE_HTTPONLY,
											NULL);
	// Encrease buffer if its size is not enough
	if (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		delete [] lpszData;
		lpszData = new TCHAR[dwSize];

		bResult = InternetGetCookieEx(StringToWString(targetUrl).c_str(), 
											name.empty() ? NULL : StringToWString(name).c_str(),
											lpszData,
											&dwSize,
											INTERNET_COOKIE_HTTPONLY,
											NULL);
	}

	return bResult ? true : false;
}

bool WebBrowserContainer::DeleteCookies(const String& targetUrl)
{
	if (!webBrowser)
	{
		return false;
	}

	WideString url = StringToWString(targetUrl);
	LPINTERNET_CACHE_ENTRY_INFO cacheEntry = NULL;  
 	// Initial buffer size
    DWORD  entrySize = 4096;     
	HANDLE cacheEnumHandle = NULL; 

	// Get first entry and enum handle
	cacheEnumHandle = GetFirstCacheEntry(cacheEntry, entrySize);
	if (!cacheEnumHandle)
	{	
		delete [] cacheEntry; 
		return false;
	}

	bool bResult = false;
    bool bDone = false;

	do
	{
		// Delete only cookies for specific site
		if ((cacheEntry->CacheEntryType & COOKIE_CACHE_ENTRY))
		{            
			// If cache entry url do have target URL - do not remove that cookie
			if (StrStr(cacheEntry->lpszSourceUrlName, url.c_str()))
			{
				DeleteUrlCacheEntry(cacheEntry->lpszSourceUrlName);
			}
		}
		// Try to get next cache entry - in case when we can't do it - exit the cycle
		if (!GetNextCacheEntry(cacheEnumHandle, cacheEntry, entrySize))
		{
			// ERROR_NO_MORE_FILES means search is finished successfully.
			bResult = (GetLastError() == ERROR_NO_MORE_ITEMS);
			bDone = true;            
		}
	} while (!bDone);

	// clean up		
	delete [] cacheEntry; 
	FindCloseUrlCache(cacheEnumHandle);  
	// Syncronize cookies storage
	InternetSetOption(0, INTERNET_OPTION_END_BROWSER_SESSION, 0, 0); 
    
	return bResult;
}

HANDLE WebBrowserContainer::GetFirstCacheEntry(LPINTERNET_CACHE_ENTRY_INFO &cacheEntry, DWORD &size)
{
	// Setup initial cache entry size
	cacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[size];
    cacheEntry->dwStructSize = size;

	// Create handle for cache entries with tag "Cookie:"
	HANDLE cacheEnumHandle = FindFirstUrlCacheEntry(L"cookie:", cacheEntry, &size);
	// If handle was not created with error - ERROR_INSUFFICIENT_BUFFER - enlarge cacheEntry size and try again
	if ((cacheEnumHandle == NULL) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		delete [] cacheEntry;            
        cacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[size];
		cacheEntry->dwStructSize = size;

		cacheEnumHandle = FindFirstUrlCacheEntry(L"cookie:", cacheEntry, &size);
	}

	return cacheEnumHandle;
}

bool WebBrowserContainer::GetNextCacheEntry(HANDLE cacheEnumHandle, LPINTERNET_CACHE_ENTRY_INFO &cacheEntry, DWORD &size)
{
	BOOL bResult = FindNextUrlCacheEntry(cacheEnumHandle, cacheEntry, &size);
	// If buffer size was not enough - give more memory for cacheEntry
	if ((!bResult) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		delete [] cacheEntry;            
        cacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[size];
		cacheEntry->dwStructSize = size;

		bResult = FindNextUrlCacheEntry(cacheEnumHandle, cacheEntry, &size);
	}

	return bResult ? true : false;
}

int32_t WebBrowserContainer::ExecuteJScript(const String& targetScript)
{
	IDispatch *m_pDisp = NULL; 
    webBrowser->get_Document(&m_pDisp);
	DVASSERT(m_pDisp);

	IHTMLDocument2* pHtmDoc2 = NULL;
	m_pDisp->QueryInterface(IID_IHTMLDocument2, (LPVOID*)&pHtmDoc2);
	DVASSERT(pHtmDoc2);

	IHTMLWindow2* pHtmWin2 = NULL;
	pHtmDoc2->get_parentWindow(&pHtmWin2);
	DVASSERT(pHtmWin2); 
	
	BSTR scriptBody = SysAllocString(StringToWString(targetScript).c_str());
	WideString JSl = L"JavaScript";
	BSTR scriptType = SysAllocString(JSl.c_str());
	VARIANT vResult;
	HRESULT hr = pHtmWin2->execScript(scriptBody, scriptType, &vResult);

	::SysFreeString(scriptBody);
	::SysFreeString(scriptType);

	if (m_pDisp) m_pDisp->Release(); 
	if (pHtmWin2) pHtmWin2->Release();
	if (pHtmDoc2) pHtmDoc2->Release();

	return 0;
}
	
bool WebBrowserContainer::DoOpenBuffer()
{
    if (bufferToOpenPath.IsEmpty() || !openFromBufferQueued)
        return false;

    ScopedComPtr<HtmlMoniker> moniker(new HtmlMoniker());
    moniker->SetHtml(bufferToOpen);
    moniker->SetBaseUrl(StringToWString(bufferToOpenPath.GetAbsolutePathname()));

    ScopedComPtr<IDispatch> docDispatch;
    HRESULT hr = webBrowser->get_Document(&docDispatch);
    if (FAILED(hr) || !docDispatch)
        return true;

    ScopedComQIPtr<IHTMLDocument2> doc(docDispatch);
    if (!doc)
        return true;

    ScopedComQIPtr<IPersistMoniker> perstMon(doc);
    if (!perstMon)
        return true;
    ScopedComQIPtr<IMoniker> htmlMon(moniker);

    hr = perstMon->Load(TRUE, htmlMon, NULL, STGM_READ);

    bufferToOpen = "";
    openFromBufferQueued = false;

    return true;
}

bool WebBrowserContainer::OpenFromBuffer(const String& buffer, const FilePath& basePath)
{
    if (!webBrowser)
    {
        return false;
    }

    OpenUrl(L"about:blank"); // webbrowser2 needs about:blank being open before loading from stream

    openFromBufferQueued = true;
    bufferToOpen = buffer;
    bufferToOpenPath = basePath;

    return true;
}

void WebBrowserContainer::UpdateRect()
{
	IOleInPlaceObject* oleInPlaceObject = NULL;
	HRESULT hRes = webBrowser->QueryInterface(IID_IOleInPlaceObject, (void**)&oleInPlaceObject);
	if (FAILED(hRes))
	{
		Logger::Error("WebBrowserContainer::SetSize(), IOleObject::QueryInterface(IID_IOleInPlaceObject) failed!, error code %i", hRes);
		return;
	}

	// Update the browser window according to the holder window.
	RECT rect = {0};
	GetClientRect(this->hwnd, &rect);

	hRes = oleInPlaceObject->SetObjectRects(&rect, &rect);
	if (FAILED(hRes))
	{
		Logger::Error("WebBrowserContainer::SetSize(), IOleObject::SetObjectRects() failed!, error code %i", hRes);
		return;
	}

	oleInPlaceObject->Release();
}

WebViewControl::WebViewControl()
{
	browserWindow = 0;
	browserContainer = NULL;
}

WebViewControl::~WebViewControl()
{
	if (browserWindow != 0)
	{
		::DestroyWindow(browserWindow);
	}

	SafeDelete(browserContainer);
}

void WebViewControl::SetDelegate(IUIWebViewDelegate *delegate, UIWebView* webView)
{
	browserContainer->SetDelegate(delegate, webView);
}

void WebViewControl::Initialize(const Rect& rect)
{
	CoreWin32PlatformBase *core = static_cast<CoreWin32PlatformBase *>(Core::Instance());
	DVASSERT(core);

	// Create the browser holder window.
	browserWindow = ::CreateWindowEx(0, L"Static", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		0, 0, 0, 0, core->GetWindow(), NULL, core->GetInstance(), NULL);
	SetRect(rect);

	// Initialize the browser itself.
	InititalizeBrowserContainer();
}

bool WebViewControl::InititalizeBrowserContainer()
{
	HRESULT hRes = ::CoInitialize(NULL);
	if (FAILED(hRes))
	{
		Logger::Error("WebViewControl::InititalizeBrowserContainer(), CoInitialize() failed!");
		return false;
	}

	browserContainer= new WebBrowserContainer();
	return browserContainer->Initialize(this->browserWindow);
}

void WebViewControl::OpenURL(const String& urlToOpen)
{
	if (browserContainer)
	{
		browserContainer->OpenUrl(StringToWString(urlToOpen.c_str()).c_str());
	}
}

void WebViewControl::LoadHtmlString(const WideString& htmlString)
{
	if (browserContainer)
	{
		browserContainer->LoadHtmlString(htmlString.c_str());
	}
}

void WebViewControl::DeleteCookies(const String& targetUrl)
{
	if (browserContainer)
	{
		browserContainer->DeleteCookies(targetUrl);
	}
}

String WebViewControl::GetCookie(const String& targetUrl, const String& name) const
{
	if (browserContainer)
	{
		return browserContainer->GetCookie(targetUrl, name);
	}

	return String();
}

Map<String, String> WebViewControl::GetCookies(const String& targetUrl) const
{
	if (browserContainer)
	{
		return browserContainer->GetCookies(targetUrl);
	}

	return Map<String, String>();
}

int32_t WebViewControl::ExecuteJScript(const String& targetScript)
{
	if (browserContainer)
	{
		return browserContainer->ExecuteJScript(targetScript);
	}
	return 0;
}

void WebViewControl::OpenFromBuffer(const String& string, const FilePath& basePath)
{
    if (browserContainer)
    {
        browserContainer->OpenFromBuffer(string, basePath);
    }
}

void WebViewControl::SetVisible(bool isVisible, bool /*hierarchic*/)
{
	if (browserWindow != 0)
	{
		::ShowWindow(browserWindow, isVisible);
	}
}

void WebViewControl::SetRect(const Rect& rect)
{
	if (browserWindow == 0)
	{
		return;
	}

	RECT browserRect = {0};
	::GetWindowRect(browserWindow, &browserRect);

	browserRect.left = (LONG)(rect.x * Core::GetVirtualToPhysicalFactor());
	browserRect.top  = (LONG)(rect.y * Core::GetVirtualToPhysicalFactor());
	browserRect.left  += (LONG)Core::Instance()->GetPhysicalDrawOffset().x;
	browserRect.top += (LONG)Core::Instance()->GetPhysicalDrawOffset().y;

	browserRect.right = (LONG)(browserRect.left + rect.dx * Core::GetVirtualToPhysicalFactor());
	browserRect.bottom = (LONG)(browserRect.top + rect.dy * Core::GetVirtualToPhysicalFactor());

	::SetWindowPos(browserWindow, NULL, browserRect.left, browserRect.top,
		browserRect.right - browserRect.left, browserRect.bottom - browserRect.top, SWP_NOZORDER );

	if (browserContainer)
	{
		browserContainer->UpdateRect();
	}
}

}
