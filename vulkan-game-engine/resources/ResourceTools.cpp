#include "ResourceTools.hpp"

noxcain::EndianSafeStream::EndianSafeStream( std::streambuf* sb, bool switch_endian ) : std::istream( sb ), switch_endian( switch_endian )
{
}
