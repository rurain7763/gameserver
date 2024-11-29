#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <msgpack.hpp>

#include "Global.h"

namespace flaw {
    class FLAW_API Serialization {
    public:
        template <typename TData>
        static void Serialize(const TData& data, std::vector<char>& result) {
            std::ostringstream os;
            msgpack::pack(os, data);
            const std::string& str = os.str();
            result.assign(str.begin(), str.end());
        }

        template <typename T>
        static T Deserialize(const std::vector<char>& data) {
            msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
            msgpack::object deserialized = oh.get();
            T result = deserialized.as<T>();
            return result;
        }

        template <typename T>
        static void Deserialize(const std::vector<char>& data, T& result) {
			msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
			msgpack::object deserialized = oh.get();
            deserialized.convert(result);
        }

        template <typename T>
        static bool TryDeserialize(const std::vector<char>& data, T& result) {
            if (data.empty()) {
                return false;
            }

            try {
                msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
                msgpack::object deserialized = oh.get();
                deserialized.convert(result);
                return true;
            }
            catch (const msgpack::type_error& e) {
                // 타입 변환 실패
                std::cerr << "Type conversion error: " << e.what() << std::endl;
                return false;
            }
        }
    };
}

#endif
