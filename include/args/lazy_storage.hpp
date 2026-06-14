#ifndef ARGS_LAZY_STORAGE_HPP
#define ARGS_LAZY_STORAGE_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>

namespace args {

template <typename T>
struct LazyStorage {
    T *_ptr{nullptr};

    alignas(T) char _storage[sizeof(T)]{};  // NOLINT

    // ---

    LazyStorage() = default;

    LazyStorage(LazyStorage &&) = delete;
    auto operator=(LazyStorage &&) -> LazyStorage & = delete;

    LazyStorage(LazyStorage const &other) {
        assign(other);
    }

    // NOLINTNEXTLINE(cert-oop54-cpp, bugprone-unhandled-self-assignment)
    auto operator=(LazyStorage const &other) -> LazyStorage & {
        if (_ptr == other._ptr) {
            return *this;
        }
        assign(other);
        return *this;
    };

    auto operator=(T const &other) -> LazyStorage & {
        if (_ptr != nullptr) {
            destroy();
        }
        in_place(&other);
        return *this;
    };

    constexpr ~LazyStorage() requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~LazyStorage() {
        if (_ptr != nullptr) {
            destroy();
        }
    }

    // ---
    constexpr operator T const &() {  // NOLINT(hicpp-explicit-conversions)
        return as_ref();
    }

    [[nodiscard]] constexpr auto as_ref() const -> T const & {
        assert(_ptr != nullptr);
        return *_ptr;
    }

private:
    auto assign(LazyStorage const &other) -> void {
        if (_ptr == nullptr && other._ptr == nullptr) {
            return;
        }
        if (_ptr == nullptr && other._ptr != nullptr) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::ranges::copy(other._storage, _storage);
                _ptr = std::start_lifetime_as<T>(_storage);
            } else {
                in_place(other._ptr);
            }
            return;
        }
        if (_ptr != nullptr && other._ptr != nullptr) {
            *_ptr = *other._ptr;
            return;
        }
        if (_ptr != nullptr && other._ptr == nullptr) {
            destroy();
        }
    }

    auto destroy() -> void {
        _ptr->~T();
        _ptr = nullptr;
    }

    auto in_place(T const *other) -> void {
        _ptr = ::new (static_cast<void *>(_storage)) T{*other};
    }
};


}  // namespace args


#endif
