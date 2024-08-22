#include "stdafx.h"
#include "AltTabber.h"

extern void QuitOverlay();

#define RETURN_IF_INVALID() do { \
    if(Invalid()) return E_FAIL; \
} while(0)

/////////////////////////////////////////////
// AltTabberUIAProvider
/////////////////////////////////////////////

AltTabberUIAProvider::AltTabberUIAProvider(
    HWND hwnd,
    ProgramState_t* programState)
: m_refCount(1)
, m_hwnd(hwnd)
, m_programState(programState)
, m_slotProviders()
{
    BuildChildren();
}

void AltTabberUIAProvider::BuildChildren()
{
    m_slotProviders.resize(m_programState->slots.size());
    for (int i = 0; i < m_programState->slots.size(); ++i) {
        m_slotProviders[i] = new ThumbnailUIAProvider(m_hwnd, m_programState, i, this);
        m_slotProviders[i]->AddRef();
    }
}

void AltTabberUIAProvider::CleanupChildren()
{
    for(auto* provider : m_slotProviders) {
        provider->Release();
    }
}

AltTabberUIAProvider::~AltTabberUIAProvider()
{
    CleanupChildren();
}

ThumbnailUIAProvider* AltTabberUIAProvider::GetProviderByIndex(int index)
{
    if(index < 0 || index >= m_slotProviders.size())
        return NULL;

    auto* provider = m_slotProviders[index];
    provider->AddRef();

    return provider;
}

IFACEMETHODIMP_(ULONG) AltTabberUIAProvider::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) AltTabberUIAProvider::Release()
{
    auto val = InterlockedDecrement(&m_refCount);
    if(val == 0) {
        delete this;
    }
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::QueryInterface(REFIID riid, void** ppInterface)
{
    if(riid == __uuidof(IUnknown)) *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    else if(riid == __uuidof(IRawElementProviderSimple)) *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    else if(riid == __uuidof(IRawElementProviderFragment)) *ppInterface =static_cast<IRawElementProviderFragment*>(this);
    else if(riid == __uuidof(IRawElementProviderFragmentRoot)) *ppInterface =static_cast<IRawElementProviderFragmentRoot*>(this);
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::GetPatternProvider(PATTERNID, IUnknown** pRetVal)
{
    *pRetVal = NULL;
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    // Although it is hard-coded for the purposes of this sample, localizable 
    // text should be stored in, and loaded from, the resource file (.rc). 
    if (propertyId == UIA_LocalizedControlTypePropertyId)
    {
        pRetVal->vt = VT_BSTR;
        TCHAR s[64];
        LoadString(hInstance, IDS_ALTTABBER_CTRL_TYPE, s, sizeof(s));
        pRetVal->bstrVal = SysAllocString(s);
    }
    else if(propertyId == UIA_HelpTextPropertyId || propertyId == UIA_FullDescriptionPropertyId) // MS Narrator doesn't pick up the HelpText property for some reason
    {
        pRetVal->vt = VT_BSTR;
        TCHAR s[1024];
        LoadString(hInstance, IDS_ALTTABBER_CTRL_DESCRIPTION, s, sizeof(s));
        pRetVal->bstrVal = SysAllocString(s);
    }
    else if (propertyId == UIA_ControlTypePropertyId)
    {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA_WindowControlTypeId;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId) 
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    // else pRetVal is empty, and UI Automation will attempt to get the property from
    //  the HostRawElementProvider, which is the default provider for the HWND.
    // Note that the Name property comes from the Caption property of the control window, 
    //  if it has one.
    else
    {
        pRetVal->vt = VT_EMPTY;
    }
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
    if (m_hwnd == NULL)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
    HRESULT hr = UiaHostProviderFromHwnd(m_hwnd, pRetVal); 
    return hr;
}

IFACEMETHODIMP AltTabberUIAProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal)
{
    if(m_programState->slots.empty()) {
        *pRetVal = NULL;
        return S_OK;
    }
    switch (direction) {
    case NavigateDirection_FirstChild:
        *pRetVal = m_slotProviders[0];
        break;
    case NavigateDirection_LastChild:
        *pRetVal = m_slotProviders[m_slotProviders.size() - 1];
        break;
    }
    if(*pRetVal) {
        (*pRetVal)->AddRef();
    }
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::GetRuntimeId(SAFEARRAY** pRetVal)
{
    *pRetVal = NULL;
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::get_BoundingRectangle(UiaRect* pRetVal)
{
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    InflateRect(&rect, -2, -2);
    POINT upperLeft;
    upperLeft.x = rect.left;  
    upperLeft.y = rect.top;
    ClientToScreen(m_hwnd, &upperLeft);

    pRetVal->left = upperLeft.x;
    pRetVal->top = upperLeft.y;
    pRetVal->width = rect.right - rect.left;
    pRetVal->height = rect.bottom - rect.top;
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::SetFocus()
{
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal)
{
    *pRetVal = NULL;
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
{
    *pRetVal = static_cast<IRawElementProviderFragmentRoot*>(this);
    AddRef();  
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal)
{
    POINT pt;
    pt.x = (LONG)x;
    pt.y = (LONG)y;

    *pRetVal = NULL;
    auto found = std::find_if(
            m_programState->slots.begin(), m_programState->slots.end(),
            [&](SlotThing_t const& thing) -> BOOL {
                return PtInRect(&thing.r, pt);
            });
    if(found != m_programState->slots.end()) {
        auto index = std::distance(m_programState->slots.begin(), found);
        if(index < (SSIZE_T)m_slotProviders.size()) {
            *pRetVal = m_slotProviders[index];
            (*pRetVal)->AddRef();
        }
    }
    return S_OK;
}

IFACEMETHODIMP AltTabberUIAProvider::GetFocus(IRawElementProviderFragment** pRetVal)
{
    if(m_programState->activeSlot >= 0 && m_programState->activeSlot < m_slotProviders.size()) {
        *pRetVal = m_slotProviders[m_programState->activeSlot];
    } else {
        *pRetVal = NULL;
    }
    return S_OK;
}

void AltTabberUIAProvider::Invalidate()
{
    CleanupChildren();
    BuildChildren();

    if (UiaClientsAreListening())
    {
        UiaRaiseStructureChangedEvent(this, StructureChangeType_ChildrenInvalidated, NULL, 0);
        if(m_programState->activeSlot >= 0 && m_programState->activeSlot < m_slotProviders.size()) {
            UiaRaiseAutomationEvent(m_slotProviders[m_programState->activeSlot], UIA_AutomationFocusChangedEventId);
        }
        /*
        // this doesn't work; leave it for later
        TCHAR s[64];
        LoadString(GetModuleHandle(NULL), IDS_ALTTABBER_LIST_UPDATED, s, sizeof(s));
        UiaRaiseNotificationEvent(this,
            NotificationKind::NotificationKind_ActionCompleted,
            NotificationProcessing::NotificationProcessing_ImportantMostRecent,
            SysAllocString(s),
            SysAllocString(_T("AltTabber")));
            */
    }
}

void AltTabberUIAProvider::SelectionChanged()
{
    if(UiaClientsAreListening()) {
        UiaRaiseAutomationEvent(this, UIA_AutomationFocusChangedEventId);
        if(m_programState->activeSlot >= 0 && m_programState->activeSlot < m_slotProviders.size()) {
            UiaRaiseAutomationEvent(m_slotProviders[m_programState->activeSlot], UIA_AutomationFocusChangedEventId);
        }
    }
}



/////////////////////////////////////////////
// ThumbnailUIAProvider
/////////////////////////////////////////////

IFACEMETHODIMP_(ULONG) ThumbnailUIAProvider::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) ThumbnailUIAProvider::Release()
{
    long val = InterlockedDecrement(&m_refCount);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP ThumbnailUIAProvider::QueryInterface(REFIID riid, void** ppInterface)
{
    if(riid == __uuidof(IUnknown)) *ppInterface =static_cast<IRawElementProviderSimple*>(this);
    else if(riid == __uuidof(IRawElementProviderSimple)) *ppInterface =static_cast<IRawElementProviderSimple*>(this);
    else if(riid == __uuidof(IRawElementProviderFragment)) *ppInterface =static_cast<IRawElementProviderFragment*>(this);
    else if(riid == __uuidof(IInvokeProvider)) *ppInterface =static_cast<IInvokeProvider*>(this);
    else
    {
        *ppInterface = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ThumbnailUIAProvider::ThumbnailUIAProvider(
    HWND hwnd,
    ProgramState_t* programState,
    int index,
    AltTabberUIAProvider* parent)
: m_refCount(1)
, m_hwnd(hwnd)
, m_programState(programState)
, m_index(index)
, m_parent(parent)
{}

ThumbnailUIAProvider::~ThumbnailUIAProvider()
{
}

IFACEMETHODIMP ThumbnailUIAProvider::Invoke()
{
    RETURN_IF_INVALID();
    m_programState->prevActiveWindow = m_programState->slots[m_index].hwnd;
    QuitOverlay();

    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal)
{
    if (patternId == UIA_InvokePatternId)
    {
        *pRetVal =static_cast<IRawElementProviderSimple*>(this);
        AddRef();
    }
    else
    {
        *pRetVal = NULL;   
    }
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
    RETURN_IF_INVALID();
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (propertyId == UIA_AutomationIdPropertyId)
    {
        pRetVal->vt = VT_BSTR;
        TCHAR s[64];
        LoadString(hInstance, IDS_THUMBNAIL_CTRL_TYPE, s, sizeof(s));

        TCHAR s1[64];
        swprintf_s(s1, 63, _T("%s %d"), s, m_index);
        pRetVal->bstrVal = SysAllocString(s1);
    }
    else if (propertyId == UIA_NamePropertyId)
    {
        pRetVal->vt = VT_BSTR;
        TCHAR str[257];
        GetWindowText(m_programState->slots[m_index].hwnd, str, 256);
        str[256] = _T('\0');
        pRetVal->bstrVal = SysAllocString(str);
    }
    else if (propertyId == UIA_ControlTypePropertyId)
    {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA_ButtonControlTypeId;
    }
    // HasKeyboardFocus is true if the list has focus, and this item is selected.
    else if(propertyId == UIA_HasKeyboardFocusPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        if(m_programState->activeSlot != m_index) {
            pRetVal->boolVal = VARIANT_FALSE;
        } else {
            pRetVal->boolVal = VARIANT_TRUE;
        }
    }
    /*
    else if (propertyId == UIA_ClickablePointPropertyId)
    {
    }
    */
    else if (propertyId == UIA_IsEnabledPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsControlElementPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsContentElementPropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;  
    }
    else
    {
        pRetVal->vt = VT_EMPTY;
    }
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
    *pRetVal = NULL; 
    return S_OK; 
}

IFACEMETHODIMP ThumbnailUIAProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment ** pRetVal)
{
    IRawElementProviderFragment* pFrag = NULL;
    switch(direction)
    {
    case NavigateDirection_Parent:
        pFrag = m_parent;
        break;

    case NavigateDirection_NextSibling:
        pFrag = m_parent->GetProviderByIndex(m_index + 1);
        break;
    case NavigateDirection_PreviousSibling:  
        pFrag = m_parent->GetProviderByIndex(m_index - 1);
        break;
    }
    *pRetVal = pFrag;
    if (pFrag != NULL) 
    {   
        pFrag->AddRef();
    }
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::GetRuntimeId(SAFEARRAY ** pRetVal)
{
    int id = m_index + 1000;
    int rId[] = { UiaAppendRuntimeId, id };

    SAFEARRAY *psa = SafeArrayCreateVector(VT_I4, 0, 2);
    for (LONG i = 0; i < 2; i++)
    {
        (void) SafeArrayPutElement(psa, &i, &(rId[i]));
    }
    *pRetVal = psa;
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::get_BoundingRectangle(UiaRect * pRetVal)
{
    RETURN_IF_INVALID();
    auto& slot = m_programState->slots[m_index];
    pRetVal->left = slot.r.left;
    pRetVal->width = slot.r.right - slot.r.left;
    pRetVal->height = slot.r.top - slot.r.bottom;
    pRetVal->top = slot.r.top;

    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::GetEmbeddedFragmentRoots(SAFEARRAY ** pRetVal)
{
    *pRetVal = NULL;
    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::SetFocus()
{
    m_programState->activeSlot = m_index;

    RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);

    return S_OK;
}

IFACEMETHODIMP ThumbnailUIAProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
{
    m_parent->AddRef();
    IRawElementProviderFragmentRoot* pRoot = static_cast<IRawElementProviderFragmentRoot*>(m_parent);
    *pRetVal = pRoot;
    return S_OK;
}

bool ThumbnailUIAProvider::Invalid()
{
    // UIA injects threads into this process, and there's a race condition when
    // rebuilding the slot list as a result of filtering, since UIA gets to the
    // structure after we've messed with the slots list but before we got to send
    // the structure changed event.
    //
    // I was aware this could happen pre UIA, but now it happens.
    //
    // I don't have the energy to figure out how to do all these things atomically,
    // so just check for some sort of validity to avoid unhandled exceptions in threads.
    return m_index < 0
        || m_index >= m_programState->slots.size() 
        || m_index >= m_parent->m_slotProviders.size()
        || m_programState->rebuildingSlots;
}
