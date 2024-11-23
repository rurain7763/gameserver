#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <sstream>

#include "Serialization.h"

namespace flaw {
	#pragma pack(push, 1)
	struct PacketHeader {
		int packetSize;
		int senderId;
		short packetId;
	};

	struct Packet {
		PacketHeader header;
		std::vector<char> serializedData;

		Packet();

		Packet(Packet&& other);

		Packet(int senderId, short packetId, const std::vector<char>& data);
		
		template <typename T>
		Packet(int senderId, short packetId, const T& data) {
			header.senderId = senderId;
			header.packetId = packetId;
			Serialization::Serialize(data, serializedData);
			header.packetSize = sizeof(PacketHeader) + serializedData.size();
		}

		void SetHeaderBE(const char* data, int& offset);
		void SetSerializedData(const char* data, int& offset);

		template <typename T>
		T GetData() const {
			return Serialization::Deserialize<T>(serializedData);
		}

		template <typename T>
		void GetData(T& data) const {
			Serialization::Deserialize<T>(serializedData, data);
		}

		void GetData(std::ostream& os) const;

		Packet& operator=(Packet&& other) {
			header = std::move(other.header);
			serializedData = std::move(other.serializedData);
			return *this;
		}

		inline int GetSerializedSize() const { return header.packetSize - sizeof(PacketHeader); }
	};
	#pragma pack(pop)
}

#endif