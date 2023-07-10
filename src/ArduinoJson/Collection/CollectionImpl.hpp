// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2023, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Collection/CollectionData.hpp>
#include <ArduinoJson/Strings/StringAdapters.hpp>
#include <ArduinoJson/Variant/VariantCompare.hpp>
#include <ArduinoJson/Variant/VariantData.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

inline JsonString CollectionIterator::key() const {
  ARDUINOJSON_ASSERT(slot_ != nullptr);
  return slot_->key();
}

inline CollectionIterator& CollectionIterator::operator++() {
  ARDUINOJSON_ASSERT(slot_ != nullptr);
  slot_ = slot_->next();
  return *this;
}

inline void CollectionData::addSlot(VariantSlot* slot) {
  ARDUINOJSON_ASSERT(slot != nullptr);

  if (tail_) {
    tail_->setNextNotNull(slot);
    tail_ = slot;
  } else {
    head_ = slot;
    tail_ = slot;
  }
}

inline void CollectionData::clear(ResourceManager* resources) {
  for (auto slot = head_; slot; slot = slot->next())
    slot->release(resources);
  head_ = 0;
  tail_ = 0;
}

inline VariantSlot* CollectionData::getPreviousSlot(VariantSlot* target) const {
  VariantSlot* current = head_;
  while (current) {
    VariantSlot* next = current->next();
    if (next == target)
      return current;
    current = next;
  }
  return 0;
}

inline void CollectionData::remove(iterator it, ResourceManager* resources) {
  if (!it)
    return;
  auto curr = it.slot_;
  auto prev = getPreviousSlot(curr);
  auto next = curr->next();
  if (prev)
    prev->setNext(next);
  else
    head_ = next;
  if (!next)
    tail_ = prev;
  curr->release(resources);
}

inline size_t CollectionData::memoryUsage() const {
  size_t total = 0;
  for (VariantSlot* s = head_; s; s = s->next()) {
    total += sizeof(VariantSlot) + s->data()->memoryUsage();
    auto key = s->key();
    if (!key.isLinked())
      total += sizeofString(s->key().size());
  }
  return total;
}

inline size_t CollectionData::nesting() const {
  size_t maxChildNesting = 0;
  for (const VariantSlot* s = head_; s; s = s->next()) {
    size_t childNesting = s->data()->nesting();
    if (childNesting > maxChildNesting)
      maxChildNesting = childNesting;
  }
  return maxChildNesting + 1;
}

inline size_t CollectionData::size() const {
  return slotSize(head_);
}

template <typename T>
inline void movePointer(T*& p, ptrdiff_t offset) {
  if (!p)
    return;
  p = reinterpret_cast<T*>(
      reinterpret_cast<void*>(reinterpret_cast<char*>(p) + offset));
  ARDUINOJSON_ASSERT(isAligned(p));
}

inline void CollectionData::movePointers(ptrdiff_t variantDistance) {
  movePointer(head_, variantDistance);
  movePointer(tail_, variantDistance);
  for (VariantSlot* slot = head_; slot; slot = slot->next())
    slot->data()->movePointers(variantDistance);
}

ARDUINOJSON_END_PRIVATE_NAMESPACE
