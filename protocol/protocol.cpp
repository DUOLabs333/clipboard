#include <glaze/glaze.hpp>
#include "protocol.hpp"

std::string protocol::Encode(const protocol::Selection& sel){
	return glz::write_json(sel);
}

protocol::Selection protocol::Decode(std::string data){
	return glz::read_json<protocol::Selection>(data).value();
}


