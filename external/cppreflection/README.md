# cppreflection

A tiny and extensible C++ reflection framework with implementation of Serialization and AutoImGui.

## Example

![example](example.gif)

```c++
class Shapes : public ISerialization, public IAutoImGui {
public:
    std::vector<std::unique_ptr<Shape>> data;

FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(data)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(data)
FIELD_DECLARATION_END()
};
```

```c++
class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape() {};
};

HAS_SUBCLASS(Shape)

class Circle : public Shape {
public:
    float radius{};

FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(radius)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(radius, d.Min = 0.0f, d.Max = 100.0f)
FIELD_DECLARATION_END()
};

class Rectangle : public Shape {
public:
    float width{};
    float height{};

FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(width)
    DECLARE(height)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(width, d.Min = 0.0f, d.Max = 200.0f)
    DECLARE(height, d.Min = 0.0f, d.Max = 200.0f)
FIELD_DECLARATION_END()
};

SUBCLASS_DECLARATION_BEGIN(Shape)
    SUBCLASS_DECLARATION(Circle)
    SUBCLASS_DECLARATION(Rectangle)
SUBCLASS_DECLARATION_END()
```