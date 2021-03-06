//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef MAP_T
#error "define the map template to use in MAP_T"
#include <immer/map.hpp>
#define MAP_T ::immer::map
#endif

#include <immer/algorithm.hpp>

#include "test/util.hpp"
#include "test/dada.hpp"

#include <catch.hpp>

#include <unordered_set>
#include <random>

template <typename T=unsigned>
auto make_generator()
{
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<T>{};
    return std::bind(dist, engine);
}

struct conflictor
{
    unsigned v1;
    unsigned v2;

    bool operator== (const conflictor& x) const
    { return v1 == x.v1 && v2 == x.v2; }
};

struct hash_conflictor
{
    std::size_t operator() (const conflictor& x) const
    { return x.v1; }
};

auto make_values_with_collisions(unsigned n)
{
    auto gen   = make_generator();
    auto vals  = std::vector<std::pair<conflictor, unsigned>>{};
    auto vals_ = std::unordered_set<conflictor, hash_conflictor>{};
    auto i = 0u;
    generate_n(back_inserter(vals), n, [&] {
        auto newv = conflictor{};
        do {
            newv = { unsigned(gen() % (n / 2)), gen() };
        } while (!vals_.insert(newv).second);
        return std::pair<conflictor, unsigned>{newv, i++};
    });
    return vals;
}

auto make_test_map(unsigned n)
{
    auto s = MAP_T<unsigned, unsigned>{};
    for (auto i = 0u; i < n; ++i)
        s = s.insert({i, i});
    return s;
}

auto make_test_map(const std::vector<std::pair<conflictor, unsigned>>& vals)
{
    auto s = MAP_T<conflictor, unsigned, hash_conflictor>{};
    for (auto&& v : vals)
        s = s.insert(v);
    return s;
}

TEST_CASE("instantiation")
{
    SECTION("default")
    {
        auto v = MAP_T<int, int>{};
        CHECK(v.size() == 0u);
    }
}

TEST_CASE("basic insertion")
{
    auto v1 = MAP_T<int, int>{};
    CHECK(v1.count(42) == 0);

    auto v2 = v1.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);

    auto v3 = v2.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);
    CHECK(v3.count(42) == 1);
};

TEST_CASE("accessor")
{
    const auto n = 666u;
    auto v = make_test_map(n);
    CHECK(v[0] == 0);
    CHECK(v[42] == 42);
    CHECK(v[665] == 665);
    CHECK(v[666] == 0);
    CHECK(v[1234] == 0);
}

TEST_CASE("at")
{
    const auto n = 666u;
    auto v = make_test_map(n);
    CHECK(v.at(0) == 0);
    CHECK(v.at(42) == 42);
    CHECK(v.at(665) == 665);
    CHECK_THROWS_AS(v.at(666), std::out_of_range);
    CHECK_THROWS_AS(v.at(1234), std::out_of_range);
}

TEST_CASE("find")
{
    const auto n = 666u;
    auto v = make_test_map(n);
    CHECK(*v.find(0) == 0);
    CHECK(*v.find(42) == 42);
    CHECK(*v.find(665) == 665);
    CHECK(v.find(666) == nullptr);
    CHECK(v.find(1234) == nullptr);
}

TEST_CASE("equals and setting")
{
    const auto n = 666u;
    auto v = make_test_map(n);

    CHECK(v == v);
    CHECK(v != v.insert({1234, 42}));
    CHECK(v != v.erase(32));
    CHECK(v == v.insert({1234, 42}).erase(1234));
    CHECK(v == v.erase(32).insert({32, 32}));

    CHECK(v.set(1234, 42) == v.insert({1234, 42}));
    CHECK(v.update(1234, [] (auto&& x) { return x + 1; }) ==
          v.set(1234, 1));
    CHECK(v.update(42, [] (auto&& x) { return x + 1; }) ==
          v.set(42, 43));
}

TEST_CASE("iterator")
{
    const auto N = 666u;
    auto v = make_test_map(N);

    SECTION("empty set")
    {
        auto s = MAP_T<unsigned, unsigned>{};
        CHECK(s.begin() == s.end());
    }

    SECTION("works with range loop")
    {
        auto seen = std::unordered_set<unsigned>{};
        for (const auto& x : v)
            CHECK(seen.insert(x.first).second);
        CHECK(seen.size() == v.size());
    }

    SECTION("iterator and collisions")
    {
        auto vals = make_values_with_collisions(N);
        auto s = make_test_map(vals);
        auto seen = std::unordered_set<conflictor, hash_conflictor>{};
        for (const auto& x : s)
            CHECK(seen.insert(x.first).second);
        CHECK(seen.size() == s.size());
    }
}

TEST_CASE("accumulate")
{
    const auto n = 666u;
    auto v = make_test_map(n);

    auto expected_n =
        [] (auto n) {
            return n * (n - 1) / 2;
        };

    SECTION("sum collection")
    {
        auto acc = [] (unsigned acc, const std::pair<unsigned, unsigned>& x) {
            return acc + x.first + x.second;
        };
        auto sum = immer::accumulate(v, 0u, acc);
        CHECK(sum == 2 * expected_n(v.size()));
    }

    SECTION("sum collisions") {
        auto vals = make_values_with_collisions(n);
        auto s = make_test_map(vals);
        auto acc = [] (unsigned r, std::pair<conflictor, unsigned> x) {
            return r + x.first.v1 + x.first.v2 + x.second;
        };
        auto sum1 = std::accumulate(vals.begin(), vals.end(), 0u, acc);
        auto sum2 = immer::accumulate(s, 0u, acc);
        CHECK(sum1 == sum2);
    }
}

TEST_CASE("update a lot")
{
    auto v = make_test_map(666u);

    for (decltype(v.size()) i = 0; i < v.size(); ++i) {
        v = v.update(i, [] (auto&& x) { return x + 1; });
        CHECK(v[i] == i+1);
    }
}

TEST_CASE("exception safety")
{
    constexpr auto n = 2666u;

    using dadaist_map_t = typename dadaist_wrapper<MAP_T<unsigned, unsigned>>::type;
    using dadaist_conflictor_map_t = typename dadaist_wrapper<MAP_T<conflictor, unsigned, hash_conflictor>>::type;

    SECTION("update collisions")
    {
        auto v = dadaist_map_t{};
        auto d = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = v.set(i, i);
        for (auto i = 0u; i < v.size();) {
            try {
                auto s = d.next();
                v = v.update(i, [] (auto x) { return x + 1; });
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.at(i) == i + 1);
            for (auto i : test_irange(i, n))
                CHECK(v.at(i) == i);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("update collisisions")
    {
        auto vals = make_values_with_collisions(n);
        auto v = dadaist_conflictor_map_t{};
        auto d = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(vals[i]);
        for (auto i = 0u; i < v.size();) {
            try {
                auto s = d.next();
                v = v.update(vals[i].first, [] (auto x) { return x + 1; });
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.at(vals[i].first) == vals[i].second + 1);
            for (auto i : test_irange(i, n))
                CHECK(v.at(vals[i].first) == vals[i].second);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}
