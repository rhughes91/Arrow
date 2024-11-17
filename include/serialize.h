#pragma once

#include <cstring>
#include <stdint.h>
#include <string>
#include <typeinfo>
#include <vector>
#include <iostream>

template<typename T>
struct Serialization
{
    static size_t (*length)(const T&); /** @brief A user-defined function to define the length of provided data T.*/
    static size_t (*serialize)(const T&, std::vector<uint8_t>&, size_t); /** A user-defined function to define the serialization of provided data T.*/
    static T (*deserialize)(std::vector<uint8_t>&, size_t); /** A user-defined functoin to define the deserialization of provided data T.*/
};

void printSerializationError(const std::string& );

template<typename T>
size_t defaultLengthSetter(const T& value)
{
    // printSerializationError("ERROR: " + std::string(typeid(T).name()) + std::string(" does not have a function to define serialization length.\n"));
    return T::length(value);
}

template<typename T>
size_t (*Serialization<T>::length)(const T&) = defaultLengthSetter;

template<typename T>
size_t defaultSerialization(const T& value, std::vector<uint8_t>& stream, size_t index)
{
    // printSerializationError("ERROR: " + std::string(typeid(T).name()) + " does not have a defined serialization function.\n");
    return T::serialize(value, stream, index);
}

template<typename T>
size_t(*Serialization<T>::serialize)(const T&, std::vector<uint8_t>&, size_t) = defaultSerialization;

template<typename T>
T defaultDeserialization(std::vector<uint8_t>& stream, size_t index)
{
    // printSerializationError("ERROR: " + std::string(typeid(T).name()) + " does not have a defined deserialization function.\n");
    return T::deserialize(stream, index);
}

template<typename T>
T (*Serialization<T>::deserialize)(std::vector<uint8_t>&, size_t) = defaultDeserialization;

namespace object
{
    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t length(const T& value)
    {
        return sizeof(value);
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t length(const T& value)
    {
        return Serialization<T>::length(value) + sizeof(size_t);
    }

    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t serialize(const T& value, std::vector<uint8_t>& stream, size_t index, size_t length = sizeof(T))
    {
        std::memcpy(&stream[index], &value, length);
        return length;
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t serialize(const T& value, std::vector<uint8_t>& stream, size_t index, size_t length = sizeof(T))
    {
        object::serialize(length, stream, index);
        return Serialization<T>::serialize(value, stream, index + sizeof(size_t)) + sizeof(size_t);
    }

    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    T& deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        return *reinterpret_cast<T*>(&stream[index]);
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    T deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        return Serialization<T>::deserialize(stream, index + sizeof(size_t));
    }

    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t resize(size_t, std::vector<uint8_t>&, size_t)
    {
        return 0;
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t resize(size_t length, std::vector<uint8_t>& stream, size_t index)
    {
        size_t original = object::deserialize<size_t>(stream, index);
        size_t offset = length - original;

        std::vector<uint8_t> copy = stream;
        
        if(original < length)
        {
            stream.resize(stream.size() + offset);
            std::memcpy(&stream[index + length], &copy[index + original], copy.size() - index - original);
        }
        else if(original > length)
        {
            std::memcpy(&stream[index + length], &copy[index + original], copy.size() - index - original);
            stream.resize(stream.size() + offset);
        }
        
        return offset;
    }
};


template <>
struct Serialization<std::string>
{
    static size_t length(const std::string& data)
    {        
        return data.size()*sizeof(char) + sizeof(size_t);
    }

    static size_t serialize(const std::string& value, std::vector<uint8_t>& stream, size_t index)
    {
        size_t stringLength = value.size();
        size_t sizing = sizeof(size_t);
        size_t count = 0;

        std::memcpy(&stream[index], &stringLength, sizing);
        for(size_t i = 0; i<stringLength; i++)
        {
            auto v = value[i];
            count += object::serialize<char>(v, stream, index + sizing + count);
        }
        return count + sizing;
    }

    static std::string deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        size_t size = object::deserialize<size_t>(stream, index);
        size_t offset = sizeof(size_t);
        size_t count = 0;

        std::string result = "";

        for(size_t i=0; i<size; i++)
        {
            auto v = object::deserialize<char>(stream, index + offset + count);
            result += v;
            count += object::length<char>(v);
        }

        return result;
    }
};

template<typename T>
struct Serialization<std::vector<T>>
{
    static size_t length(const std::vector<T>& data)
    {
        size_t result = 0;
        size_t length = data.size();
        for(size_t i=0; i<length; i++)
        {
            result += object::length(data[i]);
        }
        
        return result + sizeof(size_t);
    }

    static size_t serialize(const std::vector<T>& value, std::vector<uint8_t>& stream, size_t index)
    {
        size_t vectorLength = value.size();
        size_t sizing = sizeof(size_t);
        size_t count = 0;

        std::memcpy(&stream[index], &vectorLength, sizing);
        for(size_t i = 0; i<vectorLength; i++)
        {
            auto variable = value[i];

            size_t length = object::length<T>(variable);
            object::serialize<T>(variable, stream, index + sizing + count, length);
            count += length;
        }
        return count + sizing;
    }

    static std::vector<T> deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        size_t size = object::deserialize<size_t>(stream, index);
        size_t offset = sizeof(size_t);
        size_t count = 0;

        std::vector<T> result = std::vector<T>(size);

        for(size_t i=0; i<size; i++)
        {
            result[i] = object::deserialize<T>(stream, index + offset + count);
            count += object::length<T>(result[i]);
        }

        return result;
    }
};