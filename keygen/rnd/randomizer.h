#pragma once

#include <random>
#include <chrono>
#include <type_traits>
#include <cstdint>

class Randomizer
{
public:
    Randomizer();
    ~Randomizer();

    template<typename _Set>
    typename std::add_pointer<typename _Set::value_type>::type
        randomize(_Set set, typename _Set::size_type _times);

    template<typename _Set>
    typename std::add_pointer<typename _Set::value_type>::type
        unique_randomize(_Set set, typename _Set::size_type _times);

    template<typename _SeedType>
    void set_seed(_SeedType seed);

    void change_seed(int seed);

private:
    void clear();
    void clear_distribution();
    void clear_generator();

    static bool has_rdseed();
    static bool has_rdrand();

    static bool rdseed64(std::uint64_t& out);
    static bool rdrand64(std::uint64_t& out);

    static std::uint64_t fallback_entropy();

private:
    std::mt19937_64* m_generator;
    std::uniform_int_distribution<size_t>* m_distribution;
};

template<typename _Set>
typename std::add_pointer<typename _Set::value_type>::type
Randomizer::randomize(_Set _set, typename _Set::size_type _times)
{
    if (!_times || _set.size() < _times)
        return nullptr;

    typename std::add_pointer<typename _Set::value_type>::type rand_array =
        new typename _Set::value_type[_times];

    clear_distribution();
    m_distribution = new std::uniform_int_distribution<size_t>(0, _set.size() - 1);

    for (int i = 0; i < _times; ++i)
    {
        auto rand_index = (*m_distribution)(*m_generator);
        rand_array[i] = _set[rand_index];
    }
    return rand_array;
}

template<typename _Set>
typename std::add_pointer<typename _Set::value_type>::type
Randomizer::unique_randomize(_Set _set, typename _Set::size_type _times)
{
    if (!_times || _set.size() < _times)
        return nullptr;

    typename std::add_pointer<typename _Set::value_type>::type rand_array =
        new typename _Set::value_type[_times];

    for (int i = 0; i < _times; ++i)
    {
        clear_distribution();
        m_distribution = new std::uniform_int_distribution<size_t>(0, _set.size() - 1);
        auto rand_index = (*m_distribution)(*m_generator);
        rand_array[i] = _set[rand_index];
        _set.erase(_set.begin() + rand_index);
    }
    return rand_array;
}

template<typename _SeedType>
void Randomizer::set_seed(_SeedType _seed)
{
    clear_generator();
    std::uint64_t s = static_cast<std::uint64_t>(_seed);
    m_generator = new std::mt19937_64(s);
}
