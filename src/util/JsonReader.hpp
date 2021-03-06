#ifndef ChessPlusPlus_Util_JsonReaderClass_HeaderPlusPlus
#define ChessPlusPlus_Util_JsonReaderClass_HeaderPlusPlus

#include "Exception.hpp"

#include <json.h>

#include <string>
#include <istream>
#include <streambuf>
#include <cstdint>
#include <map>
#include <utility>

namespace chesspp
{
    namespace util
    {
        /**
         * Respresents an immutable JSON file in memory.
         * Current implementation uses json-parser by
         * James McLaughlin. Thus, it wraps a json_value.
         */
        class JsonReader
        {
            /**
             * Underlying json_value pointer.
             */
            json_value *json {nullptr};
        public:
            JsonReader() = delete;
            JsonReader(JsonReader const &) = delete;
            JsonReader &operator=(JsonReader const &) = delete;
            /**
             * Constructs this JsonReader from the given stream.
             * The stream is read to EOF.
             * \param s The stream containing the JSON.
             */
            JsonReader(std::istream &s)
            {
                if(!s)
                {
                    throw Exception("stream given to JsonReader in bad state");
                }
                std::string str ((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
                json_settings options {0, 0, nullptr, nullptr, nullptr};
                char error[json_error_max];
                json = json_parse_ex(&options, str.c_str(), str.length(), error);
                if(json == nullptr)
                {
                    //no manual cleanup needed
                    throw Exception(std::string("Error loading JSON: ") + error);
                }
            }
            /**
             * Constructs this JsonReader from the given temporary stream.
             * The stream is read to EOF.
             * \param s The temporary stream containing the JSON.
             */
            JsonReader(std::istream &&s)
            : JsonReader{s}
            {
            }
            /**
             * Move-constructs this JsonReader from another.
             * \param from the JsonReader to move.
             */
            JsonReader(JsonReader &&from)
            : json{from.json}
            {
                from.json = nullptr;
            }
            /**
             * Move-assigns this JsonReader from a given stream rvalue.
             * The stream is read to EOF. Returns *this
             * \param s The stream containing the JSON.
             */
            JsonReader &operator=(std::istream &&s)
            {
                *this = JsonReader(s); //move assign
                return *this;
            }
            /**
             * Move-assigns this JsonReader from another.
             * \param from the JsonReader to move.
             */
            JsonReader &operator=(JsonReader &&from)
            {
                std::swap(json, from.json);
                return *this;
            }
            /**
             * Destructs this JsonReader freeing any allocated memory.
             */
            ~JsonReader()
            {
                json_value_free(json), json = nullptr;
            }

            /**
             * Represents a value in the JSON.
             * Instances of this class should not exceed the
             * lifetime of the JsonReader that they came from.
             * Attempting to access a NestedValue for which the
             * associated JsonReader has been destructed results
             * in undefined behavior. Two or more instances of
             * this class may represent the same nested value in
             * the JSON.
             */
            class NestedValue
            {
                friend class ::chesspp::util::JsonReader;
                /**
                 * The particular value this instance is associated with.
                 */
                json_value const &value;
                /**
                 * Constructs this NestedValue from a json_value.
                 * Note that this constructor is private and for
                 * implementation use only.
                 * \param value_ The json_value this instance shall represent.
                 */
                NestedValue(json_value const &value_) noexcept
                : value(value_) //can't use {}
                {
                }
                NestedValue &operator=(NestedValue const &) noexcept = delete;
            public:
                NestedValue(NestedValue const &) = default;
                NestedValue(NestedValue &&) = default;
                NestedValue &operator=(NestedValue &&) = default;
                ~NestedValue() = default;

                /**
                 * Returns the type of this value.
                 * Note that the return value is a type
                 * from the json-parse library. It may
                 * be compared to these constants:
                 * json_null
                 * json_boolean
                 * json_integer
                 * json_double
                 * json_string
                 * json_array
                 * json_object
                 * \return The type of value represented.
                 */
                json_type type() const noexcept //may wish to add an abstraction layer between json-parser
                {
                    return value.type;
                }
                /**
                 * Returns the parent NestedValue or throws
                 * ::chesspp::Exception.
                 * \return the parent NestedValue.
                 * \throws ::chesspp::Exception
                 */
                NestedValue parent() const
                {
                    if(value.parent)
                    {
                        return *value.parent;
                    }
                    throw Exception("No parent json value");
                }

                /**
                 * Returns a NestedValue within this one by name.
                 * Only works if type() == json_object
                 * \param name the name of the nested JSON value.
                 * \return the NestedValue by name.
                 */
                NestedValue operator[](std::string const &name) const noexcept(noexcept(name.c_str()))
                {
                    return value[name.c_str()];
                }
                /**
                 * Returns a NestedValue within this one by name.
                 * Only works if type() == json_object
                 * \param name the name of the nested JSON value.
                 * \return the NestedValue by name.
                 */
                NestedValue operator[](char const *name) const //without this, ambiguity occurs
                {
                    return value[name];
                }
                /**
                 * Returns the length of array values.
                 * \return The length of the array, or 0 if not an array.
                 */
                std::size_t length() const noexcept
                {
                    if(value.type == json_array)
                    {
                        return value.u.array.length;
                    }
                    return 0;
                }
                /**
                 * Returns the nested value at the given array index.
                 * Only works if type() == json_array
                 * \param index The index within the array.
                 * \return the nested value at the given index.
                 */
                NestedValue operator[](std::size_t index) const noexcept
                {
                    return value[static_cast<int>(index)];
                }
                /**
                 * Provides a std::map-based view of
                 * an object value, mapping object keys
                 * to object values. Only works if
                 * type() == json_object
                 * \return a std::map of object keys ot obejct values
                 */
                std::map<std::string, NestedValue> object() const
                {
                    std::map<std::string, NestedValue> obj;
                    if(value.type == json_object)
                    {
                        for(std::size_t i = 0; i < value.u.object.length; ++i)
                        {
                            obj.insert(std::make_pair<std::string const, NestedValue>(value.u.object.values[i].name, *value.u.object.values[i].value));
                        }
                    }
                    return obj;
                }
                /**
                 * Returns the string representation of this string value.
                 * Only works if type() == json_string
                 * \return The string
                 */
                operator std::string() const noexcept(noexcept(std::string("")))
                {
                    return static_cast<char const *>(value);
                }
                //I tried an approach with templates, but the compiler was never able to deduce the corect template argument
                operator std::  int8_t() const noexcept { return static_cast<std::  int8_t>(static_cast<json_int_t>(value)); }
                operator std:: uint8_t() const noexcept { return static_cast<std:: uint8_t>(static_cast<json_int_t>(value)); }
                operator std:: int16_t() const noexcept { return static_cast<std:: int16_t>(static_cast<json_int_t>(value)); }
                operator std::uint16_t() const noexcept { return static_cast<std::uint16_t>(static_cast<json_int_t>(value)); }
                operator std:: int32_t() const noexcept { return static_cast<std:: int32_t>(static_cast<json_int_t>(value)); }
                operator std::uint32_t() const noexcept { return static_cast<std::uint32_t>(static_cast<json_int_t>(value)); }
                operator std:: int64_t() const noexcept { return static_cast<std:: int64_t>(static_cast<json_int_t>(value)); }
                operator std::uint64_t() const noexcept { return static_cast<std::uint64_t>(static_cast<json_int_t>(value)); }
                /**
                 * Returns the boolean state of this bool value.
                 * Only works if type() == json_bool.
                 * \return The boolean.
                 */
                operator bool() const noexcept
                {
                    return value;
                }
                /**
                 * Returns the floating-point value of this double.
                 * Only works if type() == json_double
                 * \return The double.
                 */
                operator double() const noexcept
                {
                    return value;
                }

                /**
                 * Only for extreme use cases: returns the
                 * underlying json_value being wrapped.
                 * \return the underlying json_value
                 */
                json_value const &implementation() noexcept
                {
                    return value;
                }
            };

            /**
             * Returns a NestedValue view into
             * this JSON.
             * \return a NestedValue view into this JSON.
             */
            NestedValue access() const noexcept
            {
                return *json;
            }
            /**
             * Returns a NestedValue view into
             * this JSON.
             * \return a NestedValue view into this JSON.
             */
            NestedValue operator()() const noexcept
            {
                return access();
            }
            /**
             * Navigates through obejct values and array values
             * and returns a NestedValue at the destination.
             * \tparam Args The types for navigation, generally deduced by the compiler.
             * \param path The path to take to arrive at the destination. May be strings and indicies.
             */
            template<typename... Args>
            NestedValue navigate(Args... path) const
            {
                return navigate(access(), path...);
            }
        private:
            /**
             * Helper for the public navigate.
             */
            template<typename First, typename... Rest>
            NestedValue navigate(NestedValue v, First const &first, Rest const &... rest) const
            {
                return navigate(v[first], rest...);
            }
            /**
             * Helper for the public navigate.
             */
            template<typename Last>
            NestedValue navigate(NestedValue v, Last const &last) const
            {
                return v[last];
            }
        };
    }
}

#endif
