#include "xlog_test_utils.h"

#include <array>

namespace xlog_test::rng
{
    constexpr std::array VALID_STRING_CHARACTERS =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        ' ', '<', '>', ';', ':', ',',
        '[', ']', /*'{', '}',*/ '(', ')',
        '.', '/', '-', '=', '_', '+',
        '?', '!', '@', '#', '$', '%',
        '^', '&', '*', '"', '\'', '\\',
        '|', '`', '~'
    };

    std::random_device& get_device()
    {
        static thread_local std::random_device device;
        return device;
    }

    std::mt19937& get_rng()
    {
        static thread_local std::mt19937 generator(get_device()());
        return generator;
    }

    std::string make_random_string(size_t length)
    {
        return make_random_string(length, length);
    }

    std::string make_random_string(size_t min_length, size_t max_length)
    {
        auto length = (min_length == max_length) ? max_length : rng::get<size_t>(min_length, max_length);

        std::string str(length, 0);
        std::vector<int> c = rng::get(0, static_cast<int>(VALID_STRING_CHARACTERS.size()), length);

        for(auto i = 0; i < length; ++i)
        {
            str[i] = VALID_STRING_CHARACTERS[c[i]];
        }

        return str;
    }
}
