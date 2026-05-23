#ifndef ARGS_TYPETAG
#define ARGS_TYPETAG

namespace args {
template <typename T>
struct Typetag {
    using type_t = T;
};

template <typename T>
inline constexpr auto tag = Typetag<T>{};
}  // namespace args

#endif
