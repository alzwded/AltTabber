#pragma once

#include "stdafx.h"
#include "resource.h"

typedef enum {
    APP_THUMB_COMPAT = 0,
    APP_THUMB_AERO,
} app_thumb_e_t;

typedef struct {
    app_thumb_e_t type;
    HWND hwnd;
    union {
        HTHUMBNAIL thumb;
        HICON icon;
    };
} AppThumb_t;

typedef struct {
    HWND hwnd;
    RECT r;
    int moveUpDownAmount;
} SlotThing_t;

#define JAT_HACK_DEXPOT 1

class AltTabberUIAProvider;

typedef struct {
    BOOL showing;
    HWND prevActiveWindow;
    signed long activeSlot;
    BOOL logging;
    FILE* freopened;
    struct {
        UINT modifiers;
        UINT key;
    } hotkey;
    DWORD compatHacks;
    BOOL resetOnClose;
    AltTabberUIAProvider* uiaProvider;

    HWND hWnd;
    std::map<HMONITOR, std::vector<AppThumb_t> > thumbnails;
    std::vector<SlotThing_t> slots;
    std::wstring filter;

    ULONG sleptThroughNWindows;
} ProgramState_t;

typedef struct {
    HMONITOR hMonitor;
    RECT extent;
} MonitorInfo_t;

typedef struct {
    RECT r;
    std::vector<MonitorInfo_t> monitors;
} MonitorGeom_t;

class ThumbnailUIAProvider :
    public IRawElementProviderSimple,
    public IRawElementProviderFragment,
    public IInvokeProvider
{
public:
    // Constructor / destructor
    ThumbnailUIAProvider(HWND hwnd, ProgramState_t* state, int index, AltTabberUIAProvider*);

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void**ppInterface);

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(ProviderOptions * pRetVal);
    IFACEMETHODIMP GetPatternProvider(PATTERNID iid,IUnknown * * pRetVal );
    IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp,VARIANT * pRetVal );
    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple ** pRetVal );

    // IRawElementProviderFragment methods
    IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment ** pRetVal );
    IFACEMETHODIMP GetRuntimeId(SAFEARRAY ** pRetVal );
    IFACEMETHODIMP get_BoundingRectangle(UiaRect * pRetVal );
    IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY ** pRetVal );
    IFACEMETHODIMP SetFocus();
    IFACEMETHODIMP get_FragmentRoot( IRawElementProviderFragmentRoot * * pRetVal );

    // IInvokeProvider methods
    IFACEMETHODIMP Invoke();

private:
    virtual ~ThumbnailUIAProvider();
    bool Invalid();

    // Ref Counter for this COM object
    ULONG m_refCount;

    HWND m_hwnd;
    ProgramState_t* m_programState;
    int m_index;
    // weak ref
    AltTabberUIAProvider* m_parent;
};

class AltTabberUIAProvider :
    public IRawElementProviderSimple,
    public IRawElementProviderFragmentRoot,
    public IRawElementProviderFragment
{
public:
    // Constructor/destructor.
    AltTabberUIAProvider(HWND hwnd, ProgramState_t* programState);

    // notification methods?
    void Invalidate();
    void SelectionChanged();

    // IUnknown methods
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void**ppInterface);

    // IRawElementProviderSimple methods
    IFACEMETHODIMP get_ProviderOptions(ProviderOptions * pRetVal);
    IFACEMETHODIMP GetPatternProvider(PATTERNID iid,IUnknown * * pRetVal );
    IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp,VARIANT * pRetVal );
    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple ** pRetVal );

    // IRawElementProviderFragment methods
    IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment ** pRetVal );
    IFACEMETHODIMP GetRuntimeId(SAFEARRAY ** pRetVal );
    IFACEMETHODIMP get_BoundingRectangle(UiaRect * pRetVal );
    IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY ** pRetVal );
    IFACEMETHODIMP SetFocus();
    IFACEMETHODIMP get_FragmentRoot( IRawElementProviderFragmentRoot * * pRetVal );

    // IRawElementProviderFragmenRoot methods
    IFACEMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment ** pRetVal );
    IFACEMETHODIMP GetFocus(IRawElementProviderFragment ** pRetVal );

    // Various methods.
    ThumbnailUIAProvider* GetProviderByIndex(int index);

private:
    virtual ~AltTabberUIAProvider();

    void CleanupChildren();
    void BuildChildren();

    // Ref counter for this COM object.
    ULONG m_refCount;

    HWND m_hwnd;
    ProgramState_t* m_programState;
    std::vector<ThumbnailUIAProvider*> m_slotProviders;

    friend class ThumbnailUIAProvider;

};
