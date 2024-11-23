#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <string>
#include <sstream>

#include "Global.h"

namespace flaw {
    class FLAW_API Serialization {
    public:
        template <typename TData>
        static void Serialize(const TData& data, std::vector<char>& result) {
            std::ostringstream oss(std::ios::binary);
            boost::archive::binary_oarchive oa(oss, boost::archive::no_header | boost::archive::no_tracking);
            oa << data;
            const std::string str = oss.str();
            result.assign(str.begin(), str.end());
        }

        template <typename T>
        static T Deserialize(const std::vector<char>& data) {
            std::istringstream iss(std::string(data.begin(), data.end()), std::ios::binary);
            boost::archive::binary_iarchive ia(iss, boost::archive::no_header | boost::archive::no_tracking);
            T result;
            ia >> result;
            return result;
        }

        template <typename T>
        static void Deserialize(const std::vector<char>& data, T& result) {
            std::istringstream iss(std::string(data.begin(), data.end()), std::ios::binary);
            boost::archive::binary_iarchive ia(iss, boost::archive::no_header | boost::archive::no_tracking);
            ia >> result;
        }
    };
}

#endif
