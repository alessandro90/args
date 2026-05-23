#ifndef CPP_ARGS_TYPETAG
#define CPP_ARGS_TYPETAG

namespace args {
template <typename T>
struct Typetag {
    using type_t = T;
};

template <typename T>
inline constexpr auto tag = Typetag<T>{};
}  // namespace args

#endif
