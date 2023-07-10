// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2023, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Memory/StringNode.hpp>
#include <ArduinoJson/Misc/SerializedValue.hpp>
#include <ArduinoJson/Numbers/convertNumber.hpp>
#include <ArduinoJson/Strings/JsonString.hpp>
#include <ArduinoJson/Strings/StringAdapters.hpp>
#include <ArduinoJson/Variant/VariantContent.hpp>
#include <ArduinoJson/Variant/VariantSlot.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename T>
T parseNumber(const char* s);

class VariantData {
  VariantContent content_;  // must be first to allow cast from array to variant
  uint8_t flags_;

 public:
  VariantData() : flags_(VALUE_IS_NULL) {}

  template <typename TVisitor>
  typename TVisitor::result_type accept(TVisitor& visitor) const {
    switch (type()) {
      case VALUE_IS_FLOAT:
        return visitor.visitFloat(content_.asFloat);

      case VALUE_IS_ARRAY:
        return visitor.visitArray(content_.asArray);

      case VALUE_IS_OBJECT:
        return visitor.visitObject(content_.asObject);

      case VALUE_IS_LINKED_STRING:
        return visitor.visitString(content_.asLinkedString,
                                   strlen(content_.asLinkedString));

      case VALUE_IS_OWNED_STRING:
        return visitor.visitString(content_.asOwnedString->data,
                                   content_.asOwnedString->length);

      case VALUE_IS_RAW_STRING:
        return visitor.visitRawString(content_.asOwnedString->data,
                                      content_.asOwnedString->length);

      case VALUE_IS_SIGNED_INTEGER:
        return visitor.visitSignedInteger(content_.asSignedInteger);

      case VALUE_IS_UNSIGNED_INTEGER:
        return visitor.visitUnsignedInteger(content_.asUnsignedInteger);

      case VALUE_IS_BOOLEAN:
        return visitor.visitBoolean(content_.asBoolean != 0);

      default:
        return visitor.visitNull();
    }
  }

  template <typename TVisitor>
  static typename TVisitor::result_type accept(const VariantData* var,
                                               TVisitor& visitor) {
    if (var != 0)
      return var->accept(visitor);
    else
      return visitor.visitNull();
  }

  VariantData* addElement(ResourceManager* resources) {
    auto array = isNull() ? &toArray() : asArray();
    return detail::ArrayData::addElement(array, resources);
  }

  static VariantData* addElement(VariantData* var, ResourceManager* resources) {
    if (!var)
      return nullptr;
    return var->addElement(resources);
  }

  bool asBoolean() const {
    switch (type()) {
      case VALUE_IS_BOOLEAN:
        return content_.asBoolean;
      case VALUE_IS_SIGNED_INTEGER:
      case VALUE_IS_UNSIGNED_INTEGER:
        return content_.asUnsignedInteger != 0;
      case VALUE_IS_FLOAT:
        return content_.asFloat != 0;
      case VALUE_IS_NULL:
        return false;
      default:
        return true;
    }
  }

  ArrayData* asArray() {
    return isArray() ? &content_.asArray : 0;
  }

  const ArrayData* asArray() const {
    return const_cast<VariantData*>(this)->asArray();
  }

  CollectionData* asCollection() {
    return isCollection() ? &content_.asCollection : 0;
  }

  const CollectionData* asCollection() const {
    return const_cast<VariantData*>(this)->asCollection();
  }

  template <typename T>
  T asFloat() const {
    static_assert(is_floating_point<T>::value, "T must be a floating point");
    switch (type()) {
      case VALUE_IS_BOOLEAN:
        return static_cast<T>(content_.asBoolean);
      case VALUE_IS_UNSIGNED_INTEGER:
        return static_cast<T>(content_.asUnsignedInteger);
      case VALUE_IS_SIGNED_INTEGER:
        return static_cast<T>(content_.asSignedInteger);
      case VALUE_IS_LINKED_STRING:
      case VALUE_IS_OWNED_STRING:
        return parseNumber<T>(content_.asOwnedString->data);
      case VALUE_IS_FLOAT:
        return static_cast<T>(content_.asFloat);
      default:
        return 0;
    }
  }

  template <typename T>
  T asIntegral() const {
    static_assert(is_integral<T>::value, "T must be an integral type");
    switch (type()) {
      case VALUE_IS_BOOLEAN:
        return content_.asBoolean;
      case VALUE_IS_UNSIGNED_INTEGER:
        return convertNumber<T>(content_.asUnsignedInteger);
      case VALUE_IS_SIGNED_INTEGER:
        return convertNumber<T>(content_.asSignedInteger);
      case VALUE_IS_LINKED_STRING:
        return parseNumber<T>(content_.asLinkedString);
      case VALUE_IS_OWNED_STRING:
        return parseNumber<T>(content_.asOwnedString->data);
      case VALUE_IS_FLOAT:
        return convertNumber<T>(content_.asFloat);
      default:
        return 0;
    }
  }

  ObjectData* asObject() {
    return isObject() ? &content_.asObject : 0;
  }

  const ObjectData* asObject() const {
    return const_cast<VariantData*>(this)->asObject();
  }

  JsonString asRawString() const {
    switch (type()) {
      case VALUE_IS_RAW_STRING:
        return JsonString(content_.asOwnedString->data,
                          content_.asOwnedString->length, JsonString::Copied);
      default:
        return JsonString();
    }
  }

  JsonString asString() const {
    switch (type()) {
      case VALUE_IS_LINKED_STRING:
        return JsonString(content_.asLinkedString, JsonString::Linked);
      case VALUE_IS_OWNED_STRING:
        return JsonString(content_.asOwnedString->data,
                          content_.asOwnedString->length, JsonString::Copied);
      default:
        return JsonString();
    }
  }

  bool copyFrom(const VariantData& src, ResourceManager* resources) {
    release(resources);
    switch (src.type()) {
      case VALUE_IS_ARRAY:
        return toArray().copyFrom(src.content_.asArray, resources);
      case VALUE_IS_OBJECT:
        return toObject().copyFrom(src.content_.asObject, resources);
      case VALUE_IS_OWNED_STRING: {
        auto str = adaptString(src.asString());
        auto dup = resources->saveString(str);
        if (!dup)
          return false;
        setOwnedString(dup);
        return true;
      }
      case VALUE_IS_RAW_STRING: {
        auto str = adaptString(src.asRawString());
        auto dup = resources->saveString(str);
        if (!dup)
          return false;
        setRawString(dup);
        return true;
      }
      default:
        content_ = src.content_;
        flags_ = src.flags_;
        return true;
    }
  }

  static bool copy(VariantData* dst, const VariantData* src,
                   ResourceManager* resources) {
    if (!dst)
      return false;
    if (!src) {
      dst->setNull();
      return true;
    }
    return dst->copyFrom(*src, resources);
  }

  VariantData* getElement(size_t index) const {
    auto array = asArray();
    if (!array)
      return nullptr;
    return array->getElement(index);
  }

  static VariantData* getElement(const VariantData* var, size_t index) {
    return var != 0 ? var->getElement(index) : 0;
  }

  template <typename TAdaptedString>
  VariantData* getMember(TAdaptedString key) const {
    auto object = asObject();
    if (!object)
      return nullptr;
    return object->getMember(key);
  }

  template <typename TAdaptedString>
  static VariantData* getMember(const VariantData* var, TAdaptedString key) {
    if (!var)
      return 0;
    return var->getMember(key);
  }

  VariantData* getOrAddElement(size_t index, ResourceManager* resources) {
    auto array = isNull() ? &toArray() : asArray();
    if (!array)
      return nullptr;
    return array->getOrAddElement(index, resources);
  }

  template <typename TAdaptedString>
  VariantData* getOrAddMember(TAdaptedString key, ResourceManager* resources) {
    if (key.isNull())
      return nullptr;
    auto obj = isNull() ? &toObject() : asObject();
    if (!obj)
      return nullptr;
    return obj->getOrAddMember(key, resources);
  }

  bool isArray() const {
    return (flags_ & VALUE_IS_ARRAY) != 0;
  }

  bool isBoolean() const {
    return type() == VALUE_IS_BOOLEAN;
  }

  bool isCollection() const {
    return (flags_ & COLLECTION_MASK) != 0;
  }

  bool isFloat() const {
    return (flags_ & NUMBER_BIT) != 0;
  }

  template <typename T>
  bool isInteger() const {
    switch (type()) {
      case VALUE_IS_UNSIGNED_INTEGER:
        return canConvertNumber<T>(content_.asUnsignedInteger);

      case VALUE_IS_SIGNED_INTEGER:
        return canConvertNumber<T>(content_.asSignedInteger);

      default:
        return false;
    }
  }

  bool isNull() const {
    return type() == VALUE_IS_NULL;
  }

  static bool isNull(const VariantData* var) {
    if (!var)
      return true;
    return var->isNull();
  }

  bool isObject() const {
    return (flags_ & VALUE_IS_OBJECT) != 0;
  }

  bool isString() const {
    return type() == VALUE_IS_LINKED_STRING || type() == VALUE_IS_OWNED_STRING;
  }

  size_t memoryUsage() const {
    switch (type()) {
      case VALUE_IS_OWNED_STRING:
      case VALUE_IS_RAW_STRING:
        return sizeofString(content_.asOwnedString->length);
      case VALUE_IS_OBJECT:
      case VALUE_IS_ARRAY:
        return content_.asCollection.memoryUsage();
      default:
        return 0;
    }
  }

  void movePointers(ptrdiff_t variantDistance) {
    if (flags_ & COLLECTION_MASK)
      content_.asCollection.movePointers(variantDistance);
  }

  size_t nesting() const {
    auto collection = asCollection();
    if (collection)
      return collection->nesting();
    else
      return 0;
  }

  static size_t nesting(const VariantData* var) {
    if (!var)
      return 0;
    return var->nesting();
  }

  void operator=(const VariantData& src) {
    content_ = src.content_;
    flags_ = uint8_t((flags_ & OWNED_KEY_BIT) | (src.flags_ & ~OWNED_KEY_BIT));
  }

  void removeElement(size_t index, ResourceManager* resources) {
    ArrayData::removeElement(asArray(), index, resources);
  }

  static void removeElement(VariantData* var, size_t index,
                            ResourceManager* resources) {
    if (!var)
      return;
    var->removeElement(index, resources);
  }

  template <typename TAdaptedString>
  void removeMember(TAdaptedString key, ResourceManager* resources) {
    ObjectData::removeMember(asObject(), key, resources);
  }

  template <typename TAdaptedString>
  static void removeMember(VariantData* var, TAdaptedString key,
                           ResourceManager* resources) {
    if (!var)
      return;
    var->removeMember(key, resources);
  }

  void reset() {
    flags_ = VALUE_IS_NULL;
  }

  void setBoolean(bool value) {
    setType(VALUE_IS_BOOLEAN);
    content_.asBoolean = value;
  }

  void setBoolean(bool value, ResourceManager* resources) {
    release(resources);
    setBoolean(value);
  }

  void setFloat(JsonFloat value) {
    setType(VALUE_IS_FLOAT);
    content_.asFloat = value;
  }

  void setFloat(JsonFloat value, ResourceManager* resources) {
    release(resources);
    setFloat(value);
  }

  template <typename T>
  typename enable_if<is_signed<T>::value>::type setInteger(T value) {
    setType(VALUE_IS_SIGNED_INTEGER);
    content_.asSignedInteger = value;
  }

  template <typename T>
  typename enable_if<is_unsigned<T>::value>::type setInteger(T value) {
    setType(VALUE_IS_UNSIGNED_INTEGER);
    content_.asUnsignedInteger = static_cast<JsonUInt>(value);
  }

  template <typename T>
  void setInteger(T value, ResourceManager* resources) {
    release(resources);
    setInteger(value);
  }

  void setNull() {
    setType(VALUE_IS_NULL);
  }

  void setNull(ResourceManager* resources) {
    release(resources);
    setNull();
  }

  static void setNull(VariantData* var, ResourceManager* resources) {
    if (!var)
      return;
    var->setNull(resources);
  }

  void setRawString(StringNode* s) {
    ARDUINOJSON_ASSERT(s);
    setType(VALUE_IS_RAW_STRING);
    content_.asOwnedString = s;
  }

  template <typename T>
  void setRawString(SerializedValue<T> value, ResourceManager* resources) {
    release(resources);
    auto dup = resources->saveString(adaptString(value.data(), value.size()));
    if (dup)
      setRawString(dup);
    else
      setNull();
  }

  template <typename T>
  static void setRawString(VariantData* var, SerializedValue<T> value,
                           ResourceManager* resources) {
    if (!var)
      return;
    var->setRawString(value, resources);
  }

  template <typename TAdaptedString>
  typename enable_if<string_traits<TAdaptedString>::has_data>::type setString(
      TAdaptedString value, ResourceManager* resources) {
    setNull(resources);

    if (value.isNull())
      return;

    if (value.isLinked()) {
      setLinkedString(value.data());
      return;
    }

    auto dup = resources->saveString(value);
    if (dup)
      setOwnedString(dup);
  }

  template <typename TAdaptedString>
  typename enable_if<!string_traits<TAdaptedString>::has_data>::type setString(
      TAdaptedString value, ResourceManager* resources) {
    setNull(resources);

    if (value.isNull())
      return;

    auto dup = resources->saveString(value);
    if (dup)
      setOwnedString(dup);
  }

  template <typename TAdaptedString>
  static void setString(VariantData* var, TAdaptedString value,
                        ResourceManager* resources) {
    if (!var)
      return;
    var->setString(value, resources);
  }

  void setLinkedString(const char* s) {
    ARDUINOJSON_ASSERT(s);
    setType(VALUE_IS_LINKED_STRING);
    content_.asLinkedString = s;
  }

  void setOwnedString(StringNode* s) {
    ARDUINOJSON_ASSERT(s);
    setType(VALUE_IS_OWNED_STRING);
    content_.asOwnedString = s;
  }

  size_t size() const {
    return isCollection() ? content_.asCollection.size() : 0;
  }

  static size_t size(const VariantData* var) {
    return var != 0 ? var->size() : 0;
  }

  ArrayData& toArray() {
    setType(VALUE_IS_ARRAY);
    new (&content_.asArray) ArrayData();
    return content_.asArray;
  }

  ArrayData& toArray(ResourceManager* resources) {
    release(resources);
    return toArray();
  }

  static ArrayData* toArray(VariantData* var, ResourceManager* resources) {
    if (!var)
      return 0;
    return &var->toArray(resources);
  }

  ObjectData& toObject() {
    setType(VALUE_IS_OBJECT);
    new (&content_.asObject) ObjectData();
    return content_.asObject;
  }

  ObjectData& toObject(ResourceManager* resources) {
    release(resources);
    return toObject();
  }

  static ObjectData* toObject(VariantData* var, ResourceManager* resources) {
    if (!var)
      return 0;
    return &var->toObject(resources);
  }

  uint8_t type() const {
    return flags_ & VALUE_MASK;
  }

 private:
  void release(ResourceManager* resources) {
    if (flags_ & OWNED_VALUE_BIT)
      resources->dereferenceString(content_.asOwnedString);

    auto collection = asCollection();
    if (collection)
      collection->clear(resources);
  }

  void setType(uint8_t t) {
    flags_ &= OWNED_KEY_BIT;
    flags_ |= t;
  }
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
