#include "Shader.h"

#include <fstream>

std::vector<char> GetShaderSource( const std::filesystem::path& path )
{
    std::ifstream file( path, std::ios::ate | std::ios::binary );

    if ( !file.is_open() ) {
        return {};
    }

    size_t size = file.tellg();
    std::vector<char> buffer( size );
    file.seekg( 0 );
    file.read( buffer.data(), size );

    file.close();

    return buffer;
}

