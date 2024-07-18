#include <glaze/glaze.hpp>
#include "protocol.hpp"

std::string protocol::Encode(protocol::Selection sel){
	return glz::write_json(sel).value();
}

protocol::Selection protocol::Decode(std::string_view data){
	return glz::read_json<protocol::Selection>(data).value();
}


