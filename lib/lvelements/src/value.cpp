#include "value.h"
#include "live/elements/engine.h"
#include "live/elements/element.h"
#include "live/elements/component.h"
#include "live/exception.h"
#include "element_p.h"
#include "context_p.h"
#include "value_p.h"
#include "v8nowarnings.h"

namespace lv{ namespace el{


// LocalValue implementation
// ----------------------------------------------------------------------

class LocalValuePrivate{

public:
    LocalValuePrivate(const v8::Local<v8::Value>& d) : data(d){}

public:
    v8::Local<v8::Value> data;
};


ScopedValue::ScopedValue(Engine *engine)
    : m_d(new LocalValuePrivate(v8::Undefined(engine->isolate())))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, bool val)
    : m_d(new LocalValuePrivate(v8::Boolean::New(engine->isolate(), val)))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, Value::Int32 val)
    : m_d(new LocalValuePrivate(v8::Integer::New(engine->isolate(), val)))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, Value::Int64 val)
    : m_d(new LocalValuePrivate(v8::Integer::New(engine->isolate(), val)))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, Value::Number val)
    : m_d(new LocalValuePrivate(v8::Number::New(engine->isolate(), val)))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, const std::string &val)
    : m_d(nullptr)
    , m_ref(new int)
{
    v8::Local<v8::Value> s = v8::String::NewFromUtf8(engine->isolate(), val.c_str());
    m_d = new LocalValuePrivate(s);
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *, Callable val)
    : m_d(new LocalValuePrivate(val.data()))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *, Object val)
    : m_d(new LocalValuePrivate(val.data()))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, const Buffer &val)
    : m_d(new LocalValuePrivate(v8::ArrayBuffer::New(engine->isolate(), val.data(), val.size())))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *, Element* val)
    : m_d(new LocalValuePrivate(ElementPrivate::localObject(val)))
    , m_ref(new int)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(Engine *engine, const Value &value)
    : m_d(nullptr)
    , m_ref(new int)
{
    ++(*m_ref);
    switch ( value.type() ){
    case Value::Stored::Boolean:
        m_d = new LocalValuePrivate(v8::Boolean::New(engine->isolate(), value.asBool()));
        break;
    case Value::Stored::Integer:
        m_d = new LocalValuePrivate(v8::Integer::New(engine->isolate(), value.asInt64()));
        break;
    case Value::Stored::Double:
        m_d = new LocalValuePrivate(v8::Number::New(engine->isolate(), value.asNumber()));
        break;
    case Value::Stored::Object:
        m_d = new LocalValuePrivate(value.asObject().data());
        break;
    case Value::Stored::Callable:
        m_d = new LocalValuePrivate(value.asCallable().data());
        break;
    case Value::Stored::Element:
        if ( value.isNull() )
            m_d = new LocalValuePrivate(v8::Undefined(engine->isolate()));
        else
            m_d = new LocalValuePrivate(ElementPrivate::localObject(value.asElement()));
        break;
    default:
        THROW_EXCEPTION(lv::Exception, "Invalid Value type", Exception::toCode("~LocalValue"));
    }
}

ScopedValue::ScopedValue(Engine *, const ScopedValue &other)
    : m_d(other.m_d)
    , m_ref(other.m_ref)
{
    ++(*m_ref);
}

ScopedValue::ScopedValue(const ScopedValue &other)
    : m_d(other.m_d)
    , m_ref(other.m_ref)
{
    ++(*m_ref);
}

ScopedValue::~ScopedValue(){
    --(*m_ref);
    if ( *m_ref == 0 ){
        delete m_ref;
        delete m_d;
    }
}

ScopedValue &ScopedValue::operator =(const ScopedValue &other){
    if ( this != &other ){
        --(*m_ref);
        if ( *m_ref == 0 ){
            delete m_d;
            delete m_ref;
        }
        m_d = other.m_d;
        m_ref = other.m_ref;
        ++(*m_ref);
    }
    return *this;
}

bool ScopedValue::operator ==(const ScopedValue &other){
    return ( m_d == other.m_d && m_ref == other.m_ref );
}

const v8::Local<v8::Value> &ScopedValue::data() const{
    return m_d->data;
}

bool ScopedValue::toBool(Engine*) const{
    return m_d->data->BooleanValue();
}

Value::Int32 ScopedValue::toInt32(Engine *engine) const{
    return m_d->data->ToInt32(engine->isolate())->Value();
}

Value::Int64 ScopedValue::toInt64(Engine *engine) const{
    return (Value::Int64)m_d->data->ToNumber(engine->isolate())->Value();
}

Value::Number ScopedValue::toNumber(Engine *engine) const{
    return m_d->data->ToNumber(engine->isolate())->Value();
}

std::string ScopedValue::toStdString(Engine *engine) const{
    return *v8::String::Utf8Value(m_d->data->ToString(engine->isolate()));
}

Callable ScopedValue::toCallable(Engine* engine) const{
    return Callable(engine, v8::Local<v8::Function>::Cast(m_d->data));
}

Buffer ScopedValue::toBuffer(Engine *) const{
    return Buffer(v8::Local<v8::ArrayBuffer>::Cast(m_d->data));
}

Object ScopedValue::toObject(Engine *engine) const{
    if ( isString() && !isObject() ){
        v8::Local<v8::String> vs = m_d->data->ToString();
        v8::Local<v8::Object> vo = v8::Local<v8::Object>::Cast(v8::StringObject::New(vs));
        return Object(engine, vo);
    } else {
        v8::Local<v8::Object> vo = v8::Local<v8::Object>::Cast(m_d->data);
        if ( vo->InternalFieldCount() == 1 )
            THROW_EXCEPTION(lv::Exception, "Converting object of Element type to Object.", 1);
        return Object(engine, vo);
    }
}

Element *ScopedValue::toElement(Engine*) const{
    if ( m_d->data->IsNullOrUndefined() )
        return nullptr;

    v8::Local<v8::Object> vo = v8::Local<v8::Object>::Cast(m_d->data);
    return ElementPrivate::elementFromObject(vo);
}

Value ScopedValue::toValue(Engine* engine) const{
    if ( isBool() ){
        return Value(toBool(engine));
    } else if ( isInt() ){
        return Value(toInt32(engine));
    } else if ( isNumber() ){
        return Value(toNumber(engine));
    } else if ( isString() ){
        return Value(toObject(engine));
    } else if ( isElement() ){
        return Value(toElement(engine));
    } else if ( isCallable() ){
        return Value(toCallable(engine));
    } else if ( isObject() ){
        return Value(toObject(engine));
    }
    return Value();
}

bool ScopedValue::isNull() const{
    return m_d->data->IsNull();
}

bool ScopedValue::isBool() const{
    return m_d->data->IsBoolean();
}

bool ScopedValue::isInt() const{
    return m_d->data->IsInt32();
}

bool ScopedValue::isNumber() const{
    return m_d->data->IsNumber();
}

bool ScopedValue::isString() const{
    return m_d->data->IsStringObject() || m_d->data->IsString();
}

bool ScopedValue::isCallable() const{
    return m_d->data->IsFunction();
}

bool ScopedValue::isBuffer() const{
    return m_d->data->IsArrayBuffer();
}

bool ScopedValue::isObject() const{
    return m_d->data->IsObject();
}

bool ScopedValue::isArray() const{
    return m_d->data->IsArray();
}

bool ScopedValue::isElement() const{
    if ( !m_d->data->IsObject() )
        return false;

    v8::Local<v8::Object> vo = m_d->data.As<v8::Object>();
    return vo->InternalFieldCount() == 1;
}

ScopedValue::ScopedValue(const v8::Local<v8::Value> &data)
    : m_d(new LocalValuePrivate(data))
    , m_ref(new int)
{
    ++(*m_ref);
}

template<>
bool convertFromV8(Engine *, const v8::Local<v8::Value> &value){
    return value->BooleanValue();
}


template<>
Value::Int32 convertFromV8(Engine *, const v8::Local<v8::Value> &value){
    return value->Int32Value();
}

template<>
Value::Int64 convertFromV8(Engine *, const v8::Local<v8::Value> &value){
    return value->IntegerValue();
}

template<>
Value::Number convertFromV8(Engine *, const v8::Local<v8::Value> &value){
    return value->NumberValue();
}

template<>
std::string convertFromV8(Engine *engine, const v8::Local<v8::Value> &value){
    return *v8::String::Utf8Value(value->ToString(engine->isolate()));
}

template<>
Callable convertFromV8(Engine *engine, const v8::Local<v8::Value> &value){
    return Callable(engine, v8::Local<v8::Function>::Cast(value));
}

template<>
Object convertFromV8(Engine *engine, const v8::Local<v8::Value> &value){
    return Object(engine, v8::Local<v8::Object>::Cast(value));
}

template<>
ScopedValue convertFromV8(Engine *engine, const v8::Local<v8::Value> &value){
    return ScopedValue(engine, value);
}

template<>
Value convertFromV8(Engine *engine, const v8::Local<v8::Value> &value){
    return ScopedValue(engine, value).toValue(engine);
}

template<>
Buffer convertFromV8(Engine *, const v8::Local<v8::Value> &value){
    return Buffer(v8::Local<v8::ArrayBuffer>::Cast(value));
}

template<>
Element *convertFromV8(Engine *e, const v8::Local<v8::Value> &value){
    if ( value->IsNullOrUndefined() )
        return nullptr;

    v8::Local<v8::Object> vo = v8::Local<v8::Object>::Cast(value);
    if ( vo->InternalFieldCount() != 1 )
    {
        auto exc = CREATE_EXCEPTION(lv::Exception, "Given value is not an Element", lv::Exception::toCode("~Value"));
        e->throwError(&exc, nullptr);
        return nullptr;
    }

    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(vo->GetInternalField(0));
    void* ptr = wrap->Value();
    return reinterpret_cast<Element*>(ptr);
}


template<>
v8::Local<v8::Integer> convertToV8<int>(Engine* engine, const int &value){
    return v8::Int32::New(engine->isolate(), value);
}

// Value Implementation
// ------------------------------------------------------------------------

Value::Data::Data(Callable value) : asCallable(new Callable(value)){}

Value::Value()
    : m_data(nullptr)
    , m_type(Value::Stored::Element)
{
}

Value::Value(bool v)
    : m_data((Value::Int64)v)
    , m_type(Value::Stored::Boolean)
{
}

Value::Value(Value::Int32 v)
    : m_data((Value::Int64)v)
    , m_type(Value::Stored::Integer)
{
}

Value::Value(Value::Int64 v)
    : m_data(v)
    , m_type(Value::Stored::Integer)
{
}

Value::Value(Value::Number v)
    : m_data(v)
    , m_type(Value::Stored::Double)
{
}

Value::Value(Object v)
    : m_data(v)
    , m_type(Value::Stored::Object)
{
}

Value::Value(Callable v)
    : m_data(v)
    , m_type(Value::Stored::Callable)
{
}

Value::Value(Element *v)
    : m_data(v)
    , m_type(Value::Stored::Element)
{
}

Value::Value(const Value &other)
    : m_type(other.m_type)
{
    switch( m_type ){
    case Value::Stored::Boolean:
    case Value::Stored::Integer: m_data = Value::Data(other.m_data.asInteger); break;
    case Value::Stored::Double: m_data = Value::Data(other.m_data.asNumber); break;
    case Value::Stored::Object: m_data = Value::Data(*other.m_data.asObject); break;
    case Value::Stored::Callable: m_data = Value::Data(*other.m_data.asCallable); break;
    case Value::Stored::Element: m_data = Value::Data(other.m_data.asElement); break;
    }
}

Value::~Value(){
    if ( m_type == Value::Stored::Object )
        delete m_data.asObject;
    else if ( m_type == Value::Stored::Callable )
        delete m_data.asCallable;
}

bool Value::operator ==(const Value &other){
    if ( m_type != other.m_type )
        return false;

    switch( m_type ){
    case Value::Stored::Boolean:
    case Value::Stored::Integer: return m_data.asInteger == other.m_data.asInteger;
    case Value::Stored::Double: return m_data.asNumber == other.m_data.asNumber;
    case Value::Stored::Object: return *m_data.asObject == *other.m_data.asObject;
    case Value::Stored::Callable: return *m_data.asCallable == *other.m_data.asCallable;
    case Value::Stored::Element: return m_data.asElement == other.m_data.asElement;
    }
    return false;
}

Value &Value::operator =(const Value &other){
    m_type = other.m_type;
    switch( m_type ){
    case Value::Stored::Boolean:
    case Value::Stored::Integer: m_data = Value::Data(other.m_data.asInteger); break;
    case Value::Stored::Double: m_data = Value::Data(other.m_data.asNumber); break;
    case Value::Stored::Object: m_data = Value::Data(*other.m_data.asObject); break;
    case Value::Stored::Callable: m_data = Value::Data(*other.m_data.asCallable); break;
    case Value::Stored::Element: m_data = Value::Data(other.m_data.asElement); break;
    }
    return *this;
}

bool Value::isNull() const{
    return ( m_type == Value::Stored::Element && m_data.asElement == nullptr);
}

bool Value::asBool() const{
    if ( m_type != Value::Stored::Boolean )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Boolean type", Exception::toCode("~Value"));
    return (bool)(m_data.asInteger);
}

Value::Int32 Value::asInt32() const{
    if ( m_type != Value::Stored::Integer )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Int32 type", Exception::toCode("~Value"));
    return (Value::Int32)(m_data.asInteger);
}

Value::Int64 Value::asInt64() const{
    if ( m_type != Value::Stored::Integer )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Int64 type", Exception::toCode("~Value"));
    return (Value::Int64)(m_data.asInteger);
}

Value::Number Value::asNumber() const{
    if ( m_type == Value::Stored::Double )
        return m_data.asNumber;
    if ( m_type == Value::Stored::Integer )
        return m_data.asInteger;
    THROW_EXCEPTION(lv::Exception, "Can't cast value into Number type", Exception::toCode("~Value"));
}

Object Value::asObject() const{
    if ( m_type != Value::Stored::Object )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Object type", Exception::toCode("~Value"));
    return *(m_data.asObject);
}

Callable Value::asCallable() const{
    if ( m_type != Value::Stored::Callable )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Callable", Exception::toCode("~Value"));
    return *(m_data.asCallable);
}

Element *Value::asElement() const{
    if ( m_type != Value::Stored::Element )
        THROW_EXCEPTION(lv::Exception, "Can't cast value into Element", Exception::toCode("~Value"));
    return m_data.asElement;
}



}}// namespace lv, el
