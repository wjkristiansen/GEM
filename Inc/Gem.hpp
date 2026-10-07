//================================================================================================
// GeM (Generic Model) - Portable COM-like Interface System
//
// This header provides a portable, cross-platform interface system inspired by COM:
// - Reference counting through AddRef/Release
// - Interface querying through QueryInterface  
// - Smart pointer support via TGemPtr
// - Interface aggregation support
// - Custom result codes for error handling
//
// Key design goals:
// - Cross-platform portability (no Windows-specific dependencies)
// - COM-like semantics and patterns for familiarity
// - Uses 64-bit interface IDs instead of GUIDs for simplicity
// - Custom Result enum instead of HRESULT for platform independence
// - Modern C++ conveniences while maintaining interface-based design
//================================================================================================

#pragma once

#include <cstdint>
#include <new>

#ifdef _MSC_VER
    #include <intrin.h>
#endif

#ifdef _WIN32
    #define GEMNOTHROW __declspec(nothrow)
#else
    #define GEMNOTHROW
#endif

// Method declaration macros for COM-style interfaces
#define GEMMETHOD(method) virtual GEMNOTHROW Gem::Result method
#define GEMMETHOD_(retType, method) virtual GEMNOTHROW retType method

// Implementation macros for concrete classes
#define GEMMETHODIMP Gem::Result
#define GEMMETHODIMP_(retType) retType

// Interface ID declaration macro
#define GEM_INTERFACE_DECLARE(xface, iid) \
    using XFace = xface; \
    static inline const char *XFaceName = #xface; \
    static constexpr Gem::InterfaceId IId{iid}

// Helper macro for QueryInterface calls with type safety
#define GEM_IID_PPV_ARGS(ppObj) \
    std::remove_reference_t<decltype(**ppObj)>::IId, reinterpret_cast<void **>(ppObj)

// Use BEGIN_GEM_INTERFACE_MAP() for classes that implement XGeneric
#define BEGIN_GEM_INTERFACE_MAP() \
    GEMMETHOD(InternalQueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) { \
    if (!ppObj) { \
        return Gem::Result::BadPointer; \
    } \
    *ppObj = nullptr; \
    switch(iid) { \
    default: \
        return Gem::Result::NoInterface; \
    case Gem::XGeneric::IId: \
        *ppObj = static_cast<Gem::XGeneric *>(this); \
        break; \

// Add an interface entry to the interface map
#define GEM_INTERFACE_ENTRY(IFace) \
    case IFace::IId: \
        *ppObj = static_cast<IFace *>(this); \
        break; \

// Add an aggregated interface entry to the interface map
// The aggregated object must delegate AddRef/Release to this outer object.
// This can be accomplished using TAggregateImpl in inheretance chain.
// This macro returns the aggregated interface pointer and the outer object's
#define GEM_INTERFACE_ENTRY_AGGREGATE(IFace, pObj) \
    case IFace::IId: \
        if (pObj) { \
            *ppObj = pObj; \
        } else { \
            return Gem::Result::BadPointer; \
        } \
        break;

// Complete the interface map
#define END_GEM_INTERFACE_MAP() \
    } \
    AddRef(); \
    return Gem::Result::Success; } \

namespace Gem
{
//------------------------------------------------------------------------------------------------
// 64-bit interface identifier
struct InterfaceId
{
	const uint64_t Value;
	InterfaceId() = default;
	constexpr InterfaceId(const InterfaceId& o) = default;
	constexpr InterfaceId(uint64_t i) :
		Value(i) {}
	constexpr bool operator==(const InterfaceId& o) const { return Value == o.Value; }
	constexpr bool operator==(uint64_t v) const { return Value == v; }
	constexpr operator uint64_t() const { return Value; }
};

//------------------------------------------------------------------------------------------------
_Return_type_success_(return >= 0)
enum class Result : int32_t
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
    PluginLoadFailed = -10,
    PluginProcNotFound = -11,
    CorruptedData = -12,
    AlreadyExists = -13,
    OutOfRange = -14,
    IoError = -15,
    NotOpen = -16,
};

//------------------------------------------------------------------------------------------------
inline const char * GemResultString(Result res)
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
        return "InvalidArg";
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
    case Gem::Result::PluginLoadFailed:
        return "PluginLoadFailed";
    case Gem::Result::PluginProcNotFound:
        return "PluginProcNotFound";
    case Gem::Result::CorruptedData:
        return "CorruptedData";
    case Gem::Result::AlreadyExists:
        return "AlreadyExists";
    case Gem::Result::OutOfRange:
        return "OutOfRange";
    case Gem::Result::IoError:
        return "IoError";
    case Gem::Result::NotOpen:
        return "NotOpen";
    }
    return "(Unknown)";
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
        if(m_p)
        {
            m_p->Release();
        }

        m_p = o.m_p;
        m_p->AddRef();

        return *this;
    }

    TGemPtr &operator=(TGemPtr &&o) noexcept
    {
        if (m_p)
        {
            m_p->Release();
        }

        m_p = o.m_p;
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
// Base interface for all GEM interfaces
struct XGeneric
{
    GEM_INTERFACE_DECLARE(XGeneric, 0xffffffffffffffffU);

    GEMMETHOD_(unsigned long, AddRef)() = 0;
    GEMMETHOD_(unsigned long, Release)() = 0;
    GEMMETHOD(QueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) = 0;

    template<class _XFace>
    Gem::Result QueryInterface(_XFace **ppObj)
    {
        return QueryInterface(_XFace::IId, reinterpret_cast<void **>(ppObj));
    }
};

//------------------------------------------------------------------------------------------------
// Reference count atomics. These use compiler intrinsics rather than std::atomic so that Gem.hpp
// stays free of standard library types and of <windows.h>.
inline unsigned long AtomicIncrement(unsigned long *pValue)
{
#ifdef _MSC_VER
    static_assert(sizeof(unsigned long) == sizeof(long), "_InterlockedIncrement operates on long");
    return static_cast<unsigned long>(_InterlockedIncrement(reinterpret_cast<long *>(pValue)));
#else
    // Taking a new reference requires an existing one, so no ordering is needed.
    return __atomic_add_fetch(pValue, 1UL, __ATOMIC_RELAXED);
#endif
}

inline unsigned long AtomicDecrement(unsigned long *pValue)
{
#ifdef _MSC_VER
    return static_cast<unsigned long>(_InterlockedDecrement(reinterpret_cast<long *>(pValue)));
#else
    // Release orders this thread's writes before the decrement; acquire makes every other
    // thread's writes visible to the thread that destroys the object.
    return __atomic_sub_fetch(pValue, 1UL, __ATOMIC_ACQ_REL);
#endif
}

//------------------------------------------------------------------------------------------------
template<class _Base>
class TGenericImpl : public _Base
{
    unsigned long m_RefCount = 0;

public:
    template<typename... Arguments>
    TGenericImpl(Arguments&&... args) : _Base(args ...)
    {
        this->Initialize();
    }

    virtual ~TGenericImpl()
    {
    }

    // Factory function for proper two-phase initialization.
    // Constructors and Initialize() should report failure by throwing GemError (see
    // ThrowGemError) or by letting std::bad_alloc propagate. Any other exception is
    // caught here and reported as Result::Fail, losing the specific cause.
    template<typename... Args>
    static Result Create(_Outptr_result_nullonfailure_ _Base **ppObject, Args... args)
    {
        if (!ppObject)
            return Result::BadPointer;
        
        *ppObject = nullptr;
        
        try
        {
            // Phase 1: Construction
            TGemPtr<_Base> obj = new TGenericImpl<_Base>(args...); // throw std::bad_alloc            
            *ppObject = obj.Detach();
            return Result::Success;
        }
        catch (const std::bad_alloc &)
        {
            return Result::OutOfMemory;
        }
        catch (const GemError &e)
        {
            return e.Result();
        }
        catch (...)
        {
            return Result::Fail;
        }
    }

    GEMMETHOD_(unsigned long,AddRef)() final
    {
        return InternalAddRef();
    }

    GEMMETHOD_(unsigned long, Release)() final
    {
        return InternalRelease();
    }

    unsigned long GEMNOTHROW InternalAddRef()
    {
        return AtomicIncrement(&m_RefCount);
    }

    unsigned long GEMNOTHROW InternalRelease()
    {
        auto result = AtomicDecrement(&m_RefCount);

        if (0UL == result)
        {
            this->Uninitialize();
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
template<class _Base, class _OuterClass>
struct TAggregate : public _Base
{
    _OuterClass *m_pOuter;

    template<typename... Arguments>
    TAggregate(_OuterClass *pOuter, Arguments... params) :
        _Base(params...),
        m_pOuter(pOuter)
    {
    }

    // Always delegate AddRef to outer generic for proper aggregation
    GEMMETHOD_(unsigned long,AddRef)() final
    {
        return m_pOuter->AddRef();
    }

    // Always delegate Release to outer generic for proper aggregation
    GEMMETHOD_(unsigned long, Release)() final
    {
        return m_pOuter->Release();
    }
    // Delegate ALL QueryInterface calls to outer object for proper COM aggregation identity
    GEMMETHOD(QueryInterface)(Gem::InterfaceId iid, _Outptr_result_nullonfailure_ void **ppObj) final
    {
        if (!ppObj)
        {
            return Gem::Result::BadPointer;
        }

        // Always delegate to outer object - it knows about both outer and inner interfaces
        return m_pOuter->QueryInterface(iid, ppObj);
    }
};

//------------------------------------------------------------------------------------------------
// Custom interfaces must derive from TGeneric<_Xface>
template<class _Xface>
class TGeneric : public _Xface
{
public:
    virtual ~TGeneric() = default;
    GEMMETHOD(InternalQueryInterface)(Gem::InterfaceId /*iid*/, _Outptr_result_nullonfailure_ void **ppUnk)
    {
        *ppUnk = nullptr;
        return Gem::Result::NoInterface;
    }

    virtual void Uninitialize() {}
};

}
