#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <map>
#include <string>
#include <vector>

namespace hvs {
class JsonSerializer {
 public:
  // 每次调用都会重新生成文档，并没有重用
  std::string serialize() {
    _writer->StartObject();
    serialize_impl();
    _writer->EndObject();
    std::string document(_buffer->GetString());
    _buffer->Clear();
    _writer->Reset(*_buffer);
    return document;
  }

  void deserialize(std::string& jsonString) {
    rapidjson::Document document;
    document.Parse(jsonString.c_str());
    assert(document.IsObject());
    setJsonValue(document.GetObject());
    deserialize_impl();
  }

  void deserialize(rapidjson::Value* value) {
    assert(value->IsObject());
    setJsonValue(value->GetObject());
    deserialize_impl();
  }

 public:
  JsonSerializer() {
    _buffer = new rapidjson::StringBuffer();
    _writer = new rapidjson::Writer<rapidjson::StringBuffer>(*_buffer);
  }
  JsonSerializer(JsonSerializer& oths) {
    _writer = oths._writer;
    _buffer = oths._buffer;
  }
  ~JsonSerializer() {
    delete _writer;
    delete _buffer;
  }

 protected:
  template <class T>
  void put(std::string key, T& value) {
    _writer->Key(key.c_str());
    encode(value);
  }
  template <class T>
  bool get(std::string key, T& dest) {
    if (_jsonValue.HasMember(key.c_str())) {
      // the value would automaticly use move assignment
      decode(&(_jsonValue[key.c_str()]), dest);
      return true;
    }
    return false;
  }

  virtual void serialize_impl() = 0;
  virtual void deserialize_impl() = 0;

 private:
  template <class T>
  void encode(T& value) {
    // should be an well-formated json object
    std::string raw_json = value.serialize();
    _writer->RawValue(raw_json.c_str(), raw_json.length(),
                      rapidjson::kObjectType);
  }
  void encode(std::string& _str);
  template <class V>
  void encode(std::vector<V>& _vec);
  template <class V>
  void encode(std::map<std::string, V>& _map);

  template <class T>
  void decode(rapidjson::Value* value, T& dest) {
    dest.deserialize(value);
  }
  void decode(rapidjson::Value* value, std::string& _str);
  template <class V>
  void decode(rapidjson::Value* value, std::vector<V>& _vec);
  template <class V>
  void decode(rapidjson::Value* value, std::map<std::string, V>& _map);

  void setJsonValue(rapidjson::Value value) { this->_jsonValue = value; }

 private:
  rapidjson::StringBuffer* _buffer;
  rapidjson::Writer<rapidjson::StringBuffer>* _writer;
  rapidjson::Value _jsonValue;
};

template <>
void JsonSerializer::encode<unsigned>(unsigned& value) {
  _writer->Uint(value);
}
template <>
void JsonSerializer::encode<int>(int& value) {
  _writer->Int(value);
}
template <>
void JsonSerializer::encode<int64_t>(int64_t& value) {
  _writer->Int64(value);
}
template <>
void JsonSerializer::encode<uint64_t>(uint64_t& value) {
  _writer->Uint64(value);
}
template <>
void JsonSerializer::encode<bool>(bool& value) {
  _writer->Bool(value);
}
template <>
void JsonSerializer::encode<double>(double& value) {
  _writer->Double(value);
}
template <>
void JsonSerializer::encode<float>(float& value) {
  _writer->Double(value);
}

void JsonSerializer::encode(std::string& _str) {
  _writer->String(_str.c_str(), _str.length());
}
template <class V>
void JsonSerializer::encode(std::vector<V>& _vec) {
  _writer->StartArray();
  for (V& value : _vec) {
    encode(value);
  }
  _writer->EndArray(_vec.size());
}

template <class T>
void JsonSerializer::encode(std::map<std::string, T>& _map) {
  _writer->StartObject();
  for (auto& [key, value] : _map) {
    _writer->Key(key.c_str());
    encode(value);
  }
  _writer->EndObject();
}

template <>
void JsonSerializer::decode<unsigned>(rapidjson::Value* value, unsigned& dest) {
  assert(value->IsUint());
  dest = value->GetUint();
}

template <>
void JsonSerializer::decode<int>(rapidjson::Value* value, int& dest) {
  assert(value->IsInt());
  dest = value->GetInt();
}

template <>
void JsonSerializer::decode<int64_t>(rapidjson::Value* value, int64_t& dest) {
  assert(value->IsInt64());
  dest = value->GetInt64();
}

template <>
void JsonSerializer::decode<uint64_t>(rapidjson::Value* value, uint64_t& dest) {
  assert(value->IsUint64());
  dest = value->GetUint64();
}

template <>
void JsonSerializer::decode<bool>(rapidjson::Value* value, bool& dest) {
  assert(value->IsBool());
  dest = value->GetBool();
}

template <>
void JsonSerializer::decode<double>(rapidjson::Value* value, double& dest) {
  assert(value->IsDouble());
  dest = value->GetDouble();
}

template <>
void JsonSerializer::decode<float>(rapidjson::Value* value, float& dest) {
  assert(value->IsDouble());
  dest = (float)(value->GetDouble());
}

void JsonSerializer::decode(rapidjson::Value* value, std::string& _str) {
  assert(value->IsString());
  _str.assign(value->GetString());
}

template <class V>
void JsonSerializer::decode(rapidjson::Value* value, std::vector<V>& _vec) {
  assert(value->IsArray());
  auto _arr = value->GetArray();
  std::vector<V> tmp(_arr.Size());
  for (int i = 0; i < _arr.Size(); i++) {
    decode(&(_arr[i]), tmp[i]);
  }
  _vec.swap(tmp);
}

template <class V>
void JsonSerializer::decode(rapidjson::Value* value,
                            std::map<std::string, V>& _map) {
  assert(value->IsObject());
  auto _obj = value->GetObject();
  std::map<std::string, V> tmp;
  for (auto itr = value->MemberBegin(); itr != value->MemberEnd(); ++itr) {
    // the name of itr must be string
    decode(&(itr->value), tmp[itr->name.GetString()]);
  }
  _map.swap(tmp);
}
}  // namespace hvs