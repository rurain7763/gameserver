#include "Packet.h"

namespace flaw {
	Packet::Packet() {
		header.packetSize = sizeof(PacketHeader);
		header.senderId = 0;
		header.packetId = 0;
	}

	Packet::Packet(Packet&& other) {
		header = std::move(other.header);
		serializedData = std::move(other.serializedData);
	}

	Packet::Packet(int senderId, short packetId, const std::vector<char>& data) {
		header.senderId = senderId;
		header.packetId = packetId;
		serializedData = data;
		header.packetSize = sizeof(PacketHeader) + serializedData.size();
	}

	void Packet::SetHeaderBE(const char* data, int& offset) {
		header = *reinterpret_cast<const PacketHeader*>(data + offset);
		offset += sizeof(PacketHeader);
	}

	void Packet::SetSerializedData(const char* data, int& offset) {
		const int serializedSize = GetSerializedSize();
		serializedData.assign(data + offset, data + offset + serializedSize);
		offset += serializedSize;
	}

	void Packet::GetData(std::ostream& os) const {
		os.write(reinterpret_cast<const char*>(&header), sizeof(PacketHeader));
		os.write(serializedData.data(), serializedData.size());
	}	
}