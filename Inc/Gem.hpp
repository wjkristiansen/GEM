//================================================================================================
// Generic
//================================================================================================

#pragma once

#define GEMAPI __stdcall
#define GEMNOTHROW __declspec(nothrow)
#define GEMMETHOD(method) virtual GEMNOTHROW Gem::Result GEMAPI method
#define GEMMETHOD_(retType, method) virtual GEMNOTHROW retType GEMAPI method
#define GEMMETHODIMP Gem::Result
#define GEMMETHODIMP_(retType) retType
#define GEM_INTERFACE_DECLARE(iid) static constexpr Gem::InterfaceId IId{iid}

#define GEM_IID_PPV_ARGS(ppObj) \
    std::remove_reference_t<decltype(**ppObj)>::IId, reinterpret_cast<void **>(ppObj)

#define BEGIN_GEM_INTERFACE_MAP0() \
    GEMMETHOD(InternalQueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) { \
    *ppObj = nullptr; \
    switch(iid) { \
    default: \
        return Gem::Result::NoInterface; \

#define BEGIN_GEM_INTERFACE_MAP() \
    GEMMETHOD(InternalQueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) { \
    *ppObj = nullptr; \
    switch(iid) { \
    default: \
        return Gem::Result::NoInterface; \
    case XGeneric::IId: \
        *ppObj = reinterpret_cast<XGeneric *>(this); \
        break; \

#define GEM_INTERFACE_ENTRY(IFace) \
    case IFace::IId: \
        *ppObj = reinterpret_cast<IFace *>(this); \
        break; \

#define GEM_CONTAINED_INTERFACE_ENTRY(IFace, member) \
    case IFace::IId: \
        *ppObj = &member; \
        break;

#define END_GEM_INTERFACE_MAP() \
    } \
    AddRef(); \
    return Gem::Result::Success; } \

namespace Gem
{
struct InterfaceId
{
	const UINT64 Value;
	InterfaceId() = default;
	InterfaceId(const InterfaceId& o) = default;
	constexpr InterfaceId(UINT64 i) :
		Value(i) {}
	bool operator==(const InterfaceId& o) const { return Value == o.Value; }
	constexpr operator UINT64() const { return Value; }
};

//------------------------------------------------------------------------------------------------
_Return_type_success_(return >= 0)
enum class Result : INT32
{
    Success = 0,
    End = 1,
    Fail = -1, // Must be first failure
    InvalidArg = -2,
    NotFound = -3,
    OutOfMemory = -4,
    NoInterface = -5,
    BadPointer = -6,
    NotImplemented = -7,
    Unavailable = -8,
    Uninitialized = -9,
};

//------------------------------------------------------------------------------------------------
inline PCSTR GemResultString(Result res)
{
    switch (res)
    {
    case Gem::Result::Success:
        return "Success";
    case Gem::Result::End:
        return "End";
    case Gem::Result::Fail:
        return "Fail";
    case Gem::Result::InvalidArg:
        return "IncalidArg";
    case Gem::Result::NotFound:
        return "NotFound";
    case Gem::Result::OutOfMemory:
        return "OutOfMemory";
    case Gem::Result::NoInterface:
        return "NoInterface";
    case Gem::Result::BadPointer:
        return "BadPointer";
    case Gem::Result::NotImplemented:
        return "NotImplemented";
    case Gem::Result::Unavailable:
        return "Unavailable";
    case Gem::Result::Uninitialized:
        return "Uninitialized";
    }
    return "(Unknown)";
}


//------------------------------------------------------------------------------------------------
inline Gem::Result GemResult(HRESULT hr)
{
    switch (hr)
    {
    case S_OK:
        return Gem::Result::Success;

    case E_FAIL:
        return Gem::Result::Fail;

    case E_OUTOFMEMORY:
        return Gem::Result::OutOfMemory;

    case E_INVALIDARG:
    case DXGI_ERROR_INVALID_CALL:
        return Gem::Result::InvalidArg;

    case DXGI_ERROR_DEVICE_REMOVED:
        // BUGBUG: TODO...
        return Gem::Result::Fail;

    case E_NOINTERFACE:
        return Gem::Result::NoInterface;

    default:
        return Gem::Result::Fail;
    }
}

//------------------------------------------------------------------------------------------------
inline bool Succeeded(Result result)
{
    return result >= Gem::Result::Success;
}

//------------------------------------------------------------------------------------------------
template<class _Type>
class TGemPtr
{
    _Type *m_p = nullptr;

public:
    TGemPtr() = default;
    TGemPtr(_Type *p) :
        m_p(p)
    {
        if (m_p)
        {
            m_p->AddRef();
        }
    }
    TGemPtr(const TGemPtr &o) :
        m_p(o.m_p)
    {
        if (m_p)
        {
            m_p->AddRef();
        }
    }
    TGemPtr(TGemPtr &&o) noexcept :
        m_p(o.m_p)
    {
        o.m_p = nullptr;
    }

    ~TGemPtr()
    {
        if (m_p)
        {
            m_p->Release();
        }
    }

    TGemPtr &Attach(_Type *p)
    {
        if (m_p)
        {
            m_p->Release();
        }

        m_p = p;

        return *this;
    }

    _Type *Detach()
    {
        _Type *pOut = m_p;
        m_p = nullptr;
        return pOut;
    }

    TGemPtr &operator=(_Type *p)
    {
        if (m_p)
        {
            m_p->Release();
        }

        m_p = p;
        if (p)
        {
            m_p->AddRef();
        }

        return *this;
    }

    TGemPtr &operator=(const TGemPtr &o)
    {
        auto temp = m_p;

        m_p = o.m_p;

        if (temp != m_p)
        {
            if (m_p)
            {
                m_p->AddRef();
            }

            if (temp)
            {
                temp->Release();
            }
        }

        return *this;
    }

    TGemPtr &operator=(TGemPtr &&o) noexcept
    {
        if (m_p != o.m_p)
        {
            auto temp = m_p;

            m_p = o.m_p;

            if (temp)
            {
                temp->Release();
            }
        }
        o.m_p = nullptr;

        return *this;
    }

    _Type **operator&()
    {
        return &m_p;
    }

    _Type &operator*() const
    {
        return *m_p;
    }

    _Type *Get() const { return m_p; }

    operator _Type *() const { return m_p; }
    
    _Type *operator->() const { return m_p; }
};

//------------------------------------------------------------------------------------------------
inline constexpr bool Failed(Gem::Result result) noexcept
{
    return result < Gem::Result::Success;
}

//------------------------------------------------------------------------------------------------
class GemError
{
    const Result m_result;
public:
    GemError() = delete;
    GemError(Result result) :
        m_result(result >= Gem::Result::Success ? Gem::Result::Fail : result) {}

    Result Result() const { return m_result; }
};

//------------------------------------------------------------------------------------------------
inline void ThrowGemError(Result result)
{
    if (Failed(result))
    {
        throw(GemError(result));
    }
}

//------------------------------------------------------------------------------------------------
struct XGeneric
{
    GEM_INTERFACE_DECLARE(0xffffffffU);

    GEMMETHOD_(ULONG, AddRef)() = 0;
    GEMMETHOD_(ULONG, Release)() = 0;
    GEMMETHOD(QueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) = 0;

    template<class _XFace>
    Gem::Result QueryInterface(_XFace **ppObj)
    {
        return QueryInterface(_XFace::IId, reinterpret_cast<void **>(ppObj));
    }
};

//------------------------------------------------------------------------------------------------
template<class _Base>
class TGeneric : public _Base
{
    ULONG m_RefCount = 0;

public:
    template<typename... Arguments>
    TGeneric(Arguments&&... args) : _Base(args ...)
    {
    }

    GEMMETHOD_(ULONG,AddRef)() final
    {
        return InternalAddRef();
    }

    GEMMETHOD_(ULONG, Release)() final
    {
        return InternalRelease();
    }

    ULONG GEMNOTHROW GEMAPI InternalAddRef()
    {
        return InterlockedIncrement(&m_RefCount);
    }

    ULONG GEMNOTHROW GEMAPI InternalRelease()
    {
        auto result = InterlockedDecrement(&m_RefCount);

        if (0UL == result)
        {
            delete(this);
        }

        return result;
    }

    GEMMETHOD(QueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) final
    {
        if (!ppObj)
        {
            return Gem::Result::BadPointer;
        }

        return _Base::InternalQueryInterface(iid, ppObj);
    }
};

//------------------------------------------------------------------------------------------------
template<class _Base>
class TInnerGeneric :
    public _Base
{
public:
    template<typename... Arguments>
    TInnerGeneric(_In_ XGeneric *pOuterGeneric, Arguments... params) :
        _Base(pOuterGeneric, params...)
    {
    }

    // Delegate AddRef to outer generic
    GEMMETHOD_(ULONG,AddRef)() final
    {
        return _Base::m_pOuterGeneric->AddRef();
    }

    // Delegate Release to outer generic
    GEMMETHOD_(ULONG, Release)() final
    {
        return _Base::m_pOuterGeneric->Release();
    }

    // Delegate Query interface to outer generic
    GEMMETHOD(QueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) final
    {
        return _Base::m_pOuterGeneric->QueryInterface(iid, ppObj);
    }
};

//------------------------------------------------------------------------------------------------
// Custom interfaces must derive from CGenericBase
class CGenericBase
{
public:
    virtual ~CGenericBase() = default;
    GEMMETHOD(InternalQueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppUnk)
    {
        *ppUnk = nullptr;
        return Gem::Result::NoInterface;
    }
};

//------------------------------------------------------------------------------------------------
class CInnerGenericBase :
    public CGenericBase
{
public:
    XGeneric *m_pOuterGeneric = nullptr; // weak pointer
    CInnerGenericBase(XGeneric *pOuterGeneric) :
        m_pOuterGeneric(pOuterGeneric) {}
};
    
}
