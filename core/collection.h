#ifndef COLLECTION_H
#define COLLECTION_H

#include <algorithm>

template <typename C>
inline bool contains(C const & c, typename C::value_type const & t)
{
    return std::find(c.begin(), c.end(), t) != c.end();
}

template <typename C>
inline void remove(C & c, typename C::value_type const & t)
{
    c.erase(std::remove(c.begin(), c.end(), t), c.end());
}

template <typename C>
inline typename C::value_type takeFirst(C & c)
{
    typename C::value_type t = std::move(c.front());
    c.pop_front();
    return t;
}

template <typename C>
inline bool mapContains(C const & c, typename C::key_type const & k)
{
    return c.find(k) != c.end();
}

template <typename C>
inline bool mapContains(C const & c, typename C::key_type const & k, typename C::mapped_type const & t)
{
    auto it = c.lower_bound(k);
    for (it; it != c.end(); ++it) {
        if (it->first != k)
            return false;
        if (it->second == t)
            return true;
    }
    return false;
}

template <typename C>
inline typename C::mapped_type & mapValue(C & c, typename C::key_type const & k)
{
    static typename C::mapped_type dft;
    auto it = c.find(k);
    return it == c.end() ? dft : it->second;
}

template <typename C>
inline typename C::mapped_type const & mapValue(C const & c, typename C::key_type const & k)
{
    static typename C::mapped_type dft;
    auto it = c.find(k);
    return it == c.end() ? dft : it->second;
}

template <typename C>
inline typename C::mapped_type mapTake(C & c, typename C::key_type const & k)
{
    auto it = c.find(k);
    if (it == c.end())
        return typename C::mapped_type();
    typename C::mapped_type t = std::move(it->second);
    c.erase(it);
    return t;
}

#endif // COLLECTION_H
