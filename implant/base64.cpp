#include "base64.h"

const std::string base64_padding[] = { "", "==","=" };

std::string base64_encode(std::string s)
{
    namespace bai = boost::archive::iterators;

    std::stringstream os;

    // convert binary values to base64 characters
    typedef bai::base64_from_binary

        // retrieve 6 bit integers from a sequence of 8 bit bytes
        <bai::transform_width<char*, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

    std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()), std::ostream_iterator<char>(os));

    os << base64_padding[s.size() % 3];
    return os.str();
}

std::string base64_decode(std::string s)
{
    namespace bai = boost::archive::iterators;

    std::stringstream os;

    typedef bai::transform_width<bai::binary_from_base64<char* >, 8, 6>
        base64_dec;

    unsigned int size = s.size();

    // Remove the padding characters.
    if (size && s[size - 1] == '=') {
        --size;
        if (size && s[size - 1] == '=')
            --size;
    }

    if (size == 0) return std::string();

    unsigned int paddChars = count(s.begin(), s.end(), '=');
    std::replace(s.begin(), s.end(), '=', 'A');
    std::string decoded_val(base64_dec(s.c_str()), base64_dec(s.c_str() + size));
    decoded_val.erase(decoded_val.end() - paddChars, decoded_val.end());
    return decoded_val;
}