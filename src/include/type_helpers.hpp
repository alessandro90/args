#ifndef CPP_ARGS_TYPE_HELPERS
#define CPP_ARGS_TYPE_HELPERS

#include <string_view>
#include <type_traits>
#include <vector>

namespace args::detail {
template <typename T>
struct IsVector: std::false_type {};

template <typename T>
struct IsVector<std::vector<T>>: std::true_type {};

template <typename T>
struct IsStringView: std::false_type {};

template <>
struct IsStringView<std::string_view>: std::true_type {};

template <typename T>
struct IsString: std::false_type {};

template <>
struct IsString<std::string>: std::true_type {};
}  // namespace args::detail

#endif
