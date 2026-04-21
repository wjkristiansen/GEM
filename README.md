# GeM - Generic Model

GeM is a portable, header-only C++ interface system inspired by Microsoft's [Component Object Model (COM)](https://learn.microsoft.com/en-us/windows/win32/com/component-object-model--com--portal). It provides reference counting, interface querying, smart pointers, and aggregation - without any platform-specific dependencies.

## Features

- **Reference counting** via `AddRef` / `Release`
- **Interface querying** via `QueryInterface`
- **Smart pointer** support with `TGemPtr<T>`
- **Interface aggregation** for composing objects
- **Custom result codes** for cross-platform error handling
- **Header-only** - single header (`Gem.hpp`), no build step required

## Design Philosophy

GeM brings COM's proven interface-based design to cross-platform C++. The mapping from COM concepts is intentionally direct:

| COM | GeM | Notes |
|-----|-----|-------|
| `IUnknown` | `Gem::XGeneric` | Base interface: AddRef / Release / QueryInterface |
| `GUID` | `Gem::InterfaceId` | 64-bit integer instead of 128-bit GUID |
| `HRESULT` | `Gem::Result` | Scoped enum with `Succeeded()` / `Failed()` helpers |
| `CComPtr<T>` | `Gem::TGemPtr<T>` | RAII reference-counting smart pointer |
| `COM_INTERFACE_ENTRY` | `GEM_INTERFACE_ENTRY` | Interface map macros |

Unlike COM, which targets C compatibility, GeM is C++-only and leverages templates (`TGeneric<T>`, `TGenericImpl<T>`, `TGemPtr<T>`, `TAggregate<T,U>`) to reduce the boilerplate typically associated with implementing COM-style interfaces.

## Integration

GeM is a header-only library. Simply `#include` it in your source.

```cpp
#include <Gem.hpp>
```

## Core API

### XGeneric - The Base Interface

All GeM interfaces ultimately derive from `Gem::XGeneric`, analogous to COM's `IUnknown`:

```cpp
struct XGeneric {
    GEM_INTERFACE_DECLARE(XGeneric, 0xffffffffffffffffU);

    virtual unsigned long AddRef()  = 0;
    virtual unsigned long Release() = 0;
    virtual Gem::Result QueryInterface(Gem::InterfaceId iid, void **ppObj) = 0;
};
```

### InterfaceId

Each interface is identified by a unique 64-bit `InterfaceId`, declared with the `GEM_INTERFACE_DECLARE` macro:

```cpp
struct XMyInterface : public Gem::XGeneric {
    GEM_INTERFACE_DECLARE(XMyInterface, 0x1234567890ABCDEF);
    // interface methods ...
};
```

### Result Codes

`Gem::Result` is a scoped enum for error handling. Success codes are >= 0; failure codes are < 0.

| Code                 | Value | Meaning                    |
|----------------------|------:|----------------------------|
| `Success`            |     0 | Operation succeeded        |
| `End`                |     1 | Iteration complete         |
| `Fail`               |    -1 | General failure            |
| `InvalidArg`         |    -2 | Invalid argument           |
| `NotFound`           |    -3 | Item not found             |
| `OutOfMemory`        |    -4 | Allocation failed          |
| `NoInterface`        |    -5 | QueryInterface miss        |
| `BadPointer`         |    -6 | Null pointer passed        |
| `NotImplemented`     |    -7 | Not yet implemented        |
| `Unavailable`        |    -8 | Resource unavailable       |
| `Uninitialized`      |    -9 | Object not initialized     |
| `PluginLoadFailed`   |   -10 | Plugin DLL load error      |
| `PluginProcNotFound` |   -11 | Plugin entry-point missing |

Use the free-function helpers to test results:

```cpp
if (Gem::Succeeded(result)) { /* ... */ }
if (Gem::Failed(result))    { /* ... */ }
```

### TGemPtr - Smart Pointer

`Gem::TGemPtr<T>` is an RAII smart pointer that calls `AddRef` on copy and `Release` on destruction:

```cpp
Gem::TGemPtr<XMyInterface> ptr;
result = obj->QueryInterface(GEM_IID_PPV_ARGS(&ptr));
ptr->DoSomething();
// Release is called automatically when ptr goes out of scope
```

`Attach` / `Detach` transfer ownership without touching the reference count.

## Defining Interfaces

Use `GEMMETHOD` (returns `Gem::Result`) and `GEMMETHOD_` (returns another type) to declare pure-virtual interface methods:

```cpp
struct XEditor : public Gem::XGeneric {
    GEM_INTERFACE_DECLARE(XEditor, 0x3A7C9E2F1B8D4052);

    GEMMETHOD(OpenFile)(const char *path) = 0;  // returns Gem::Result
    GEMMETHOD_(bool, IsDirty)() = 0;            // returns bool
};
```

## Implementing Interfaces

### 1. Create an Implementation Class

Derive from `Gem::TGeneric<YourInterface>` and declare an interface map:

```cpp
class CEditor : public Gem::TGeneric<XEditor> {
    bool m_dirty = false;

public:
    BEGIN_GEM_INTERFACE_MAP()
        GEM_INTERFACE_ENTRY(XEditor)
    END_GEM_INTERFACE_MAP()

    void Initialize() {}
    void Uninitialize() {}

    GEMMETHODIMP(OpenFile)(const char *path) override {
        // open the file ...
        return Gem::Result::Success;
    }

    GEMMETHODIMP_(bool, IsDirty)() override {
        return m_dirty;
    }
};
```

`TGeneric<T>` provides a default `InternalQueryInterface` (returns `NoInterface`) and a virtual `Uninitialize` hook. The `BEGIN / END` interface-map macros override `InternalQueryInterface` with a compile-time switch over the declared interface IDs.

### 2. Instantiate with TGenericImpl

`Gem::TGenericImpl<T>` layers reference counting and a type-safe factory on top of your implementation class:

```cpp
Gem::TGemPtr<XEditor> pEditor;
Gem::Result hr = Gem::TGenericImpl<CEditor>::Create(&pEditor);
```

The `Create` factory:
- Allocates the object (`std::bad_alloc` -> `Result::OutOfMemory`)
- Calls the constructor, then `Initialize()`
- Converts any thrown `GemError` to the corresponding `Result`

### 3. Query for Other Interfaces

```cpp
Gem::TGemPtr<XSerializable> pSerial;
if (Gem::Succeeded(pEditor->QueryInterface(GEM_IID_PPV_ARGS(&pSerial)))) {
    // pSerial is valid
}
```

## Interface Aggregation

GeM supports COM-style aggregation. An inner object delegates `AddRef`, `Release`, and `QueryInterface` to the outer object so that identity rules are preserved.

Use `Gem::TAggregate<InnerBase, OuterClass>` for the inner object and `GEM_INTERFACE_ENTRY_AGGREGATE` in the outer object's interface map:

```cpp
class COuter : public Gem::TGeneric<XOuter> {
    Gem::TAggregate<CInner, COuter> m_inner{this};

public:
    BEGIN_GEM_INTERFACE_MAP()
        GEM_INTERFACE_ENTRY(XOuter)
        GEM_INTERFACE_ENTRY_AGGREGATE(XInnerFace, &m_inner)
    END_GEM_INTERFACE_MAP()
};
```

## Error Handling

`GemError` is a lightweight exception used during object construction to propagate failure codes through `TGenericImpl::Create`:

```cpp
// Inside Initialize():
Gem::ThrowGemError(result);  // throws GemError if Failed(result)
```

`Create` catches `GemError` and returns the contained `Result`, keeping error handling exception-free for callers.

## Why `X` Instead of `I`?

COM conventionally prefixes interfaces with `I` (e.g. `IUnknown`). GeM uses `X` instead (e.g. `XGeneric`). Visual Studio's Class View assumes that any class whose name begins with a capital `I` is a COM interface and applies special handling that breaks the viewer for non-COM types. Rather than fight this assumption, GeM adopts the `X` prefix for all interface names.

## License

MIT License - see [LICENSE](LICENSE) for details.

