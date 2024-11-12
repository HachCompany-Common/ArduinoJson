// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2024, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Strings/Adapters/RamString.hpp>
#include <ArduinoJson/Strings/JsonString.hpp>
#include <ArduinoJson/Strings/StringAdapter.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <>
struct StringAdapter<JsonString> {
  using AdaptedString = RamString;

  static AdaptedString adapt(const JsonString& s) {
    return AdaptedString(s.c_str(), s.size(), s.isLinked());
  }
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
