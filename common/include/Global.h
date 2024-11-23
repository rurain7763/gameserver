#ifndef GLOBAL_H
#define GLOBAL_H

#include <boost/serialization/serialization.hpp>

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