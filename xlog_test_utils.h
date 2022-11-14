#ifndef XLOG_XLOG_TEST_UTILS_H
#define XLOG_XLOG_TEST_UTILS_H

#include <string>
#include <random>
#include <limits>

namespace xlog_test
{
    namespace rng
    {
        std::random_device& get_device();
        std::mt19937& get_rng();

        std::string make_random_string(size_t length);
        std::string make_random_string(size_t min_length, size_t max_length);

        std::string make_random_string_safe(size_t length);
        std::string make_random_string_safe(size_t min_length, size_t max_length);

        template<class T>
        T get(T min, T max)
        {
            auto& rng = get_rng();

            if constexpr(std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> rd(min, max);
                return rd(rng);
            }
            else
            {
                std::uniform_int_distribution<T> ri(min, max);
                return ri(rng);
            }
        }

        template<class T>
        T get()
        {
            return get(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        }

        template<class T>
        std::vector<T> get(T min, T max, size_t count)
        {
            auto& rng = get_rng();
            std::vector<T> values(count);

            if constexpr(std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> rd(min, max);
                for(auto i = 0; i < count; ++i)
                {
                    values[i] = rd(rng);
                }

                return values;
            }
            else
            {
                std::uniform_int_distribution<T> ri(min, max);
                for(auto i = 0; i < count; ++i)
                {
                    values[i] = ri(rng);
                }

                return values;
            }
        }

        template<class T>
        std::vector<T> get(size_t count)
        {
            return get(std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), count);
        }
    }
}

#endif //XLOG_XLOG_TEST_UTILS_H
