#ifndef TEST_PACKET_H
#define TEST_PACKET_H

enum PacketType : short {
	PACKET_TYPE_A = 1,
};

struct A {
	char c_data;
	int i_data;
	float f_data;

	template <class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& c_data;
		ar& i_data;
		ar& f_data;
	}
};

#endif