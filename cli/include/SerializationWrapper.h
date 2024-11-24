#ifndef SERIALIZATION_WRAPPER_H
#define SERIALIZATION_WRAPPER_H

#include "Serialization.h"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace CLIWrapper {
    public ref class SerializationWrapper {
    public:
        static array<Byte>^ Serialize(Object^ obj) {
            std::vector<char> serializedData;
            // Assuming obj is of a type that can be serialized by the existing Serialization functions
            // You may need to add more specific handling based on the actual types you are working with
            // For example, if obj is a specific class, you can cast it and call the appropriate Serialization function
            // Here, we assume obj is a string for simplicity
            std::string str = msclr::interop::marshal_as<std::string>(obj->ToString());
            flaw::Serialization::Serialize(str, serializedData);

            array<Byte>^ managedData = gcnew array<Byte>(serializedData.size());
            Marshal::Copy(IntPtr(serializedData.data()), managedData, 0, serializedData.size());
            return managedData;
        }

        static Object^ Deserialize(array<Byte>^ data) {
            // Deserialize the byte array to an object
            std::vector<char> nativeData(data->Length);
            Marshal::Copy(data, 0, IntPtr(nativeData.data()), data->Length);

            // Assuming the deserialized object is a string for simplicity
            std::string str = flaw::Serialization::Deserialize<std::string>(nativeData);
            return gcnew String(str.c_str());
        }
    };
}

#endif