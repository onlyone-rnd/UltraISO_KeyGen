
#include "randomizer.h"
#include <immintrin.h>
#include <intrin.h>

Randomizer::Randomizer()
{
    m_generator = nullptr;
    m_distribution = nullptr;
}

Randomizer::~Randomizer()
{
    clear();
}

bool Randomizer::has_rdseed()
{
    int info[4] = { 0 };

    __cpuid(info, 0);
    if (info[0] < 7)
        return false;

    __cpuidex(info, 7, 0);
    return (info[1] & (1 << 18)) != 0;
}

bool Randomizer::has_rdrand()
{
    int info[4] = { 0 };

    __cpuid(info, 1);
    return (info[2] & (1 << 30)) != 0;
}

bool Randomizer::rdseed64(std::uint64_t& out)
{
    for (int i = 0; i < 10; ++i)
    {
        if (_rdseed64_step(&out))
            return true;
        _mm_pause();
    }
    return false;
}

bool Randomizer::rdrand64(std::uint64_t& out)
{
    for (int i = 0; i < 10; ++i)
    {
        if (_rdrand64_step(&out))
            return true;
        _mm_pause();
    }
    return false;
}

std::uint64_t Randomizer::fallback_entropy()
{
    using clock = std::chrono::high_resolution_clock;

    std::uint64_t t =
        static_cast<std::uint64_t>(clock::now().time_since_epoch().count());

    std::random_device rd;
    std::uint64_t r1 = static_cast<std::uint64_t>(rd());
    std::uint64_t r2 = static_cast<std::uint64_t>(rd());

    return (t ^ (r1 << 32) ^ r2);
}

void Randomizer::change_seed(int seed)
{
    std::uint64_t s = 0;

    if (has_rdseed() && rdseed64(s))
    {
        s ^= static_cast<std::uint64_t>(seed);
    }
    else if (has_rdrand() && rdrand64(s))
    {
        s ^= static_cast<std::uint64_t>(seed);
    }
    else
    {
        s = fallback_entropy() ^ static_cast<std::uint64_t>(seed);
    }

    set_seed(s);
}

void Randomizer::clear()
{
    clear_distribution();
    clear_generator();
}

void Randomizer::clear_distribution()
{
    if (m_distribution != nullptr)
    {
        delete m_distribution;
        m_distribution = nullptr;
    }
}

void Randomizer::clear_generator()
{
    if (m_generator != nullptr)
    {
        delete m_generator;
        m_generator = nullptr;
    }
}
