#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#ifdef BOOST_SERIALIZATION
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <string>
#include <sstream>
#endif

#include "Global.h"

namespace flaw {
    class FLAW_API Serialization {
    public:
        template <typename TData>
        static void Serialize(const TData& data, std::vector<char>& result) {
            const long size = data.ByteSizeLong();
            result.resize(size);
            data.SerializeToArray(result.data(), size);
        }

        template <typename T>
        static T Deserialize(const std::vector<char>& data) {
            T result;
            result.ParseFromArray(data.data(), data.size());
            return result;
        }

        template <typename T>
        static void Deserialize(const std::vector<char>& data, T& result) {
            result.ParseFromArray(data.data(), data.size());
        }
    };
}

#endif
