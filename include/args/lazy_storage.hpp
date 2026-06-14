#ifndef ARGS_LAZY_STORAGE_HPP
#define ARGS_LAZY_STORAGE_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace args {
template <typename T>
struct LazyStorage;

namespace lazy_storage::detail {
template <typename>
struct IsAssignNoExcept {};

template <typename T>
struct IsAssignNoExcept<LazyStorage<T> &>: std::conjunction<
                                               std::is_nothrow_copy_constructible<T>,
                                               std::is_nothrow_copy_assignable<T>,
                                               std::is_nothrow_destructible<T>> {};

template <typename T>
struct IsAssignNoExcept<LazyStorage<T> &&>: std::conjunction<
                                                std::is_nothrow_move_constructible<T>,
                                                std::is_nothrow_move_assignable<T>,
                                                std::is_nothrow_destructible<T>> {};
}  // namespace lazy_storage::detail

template <typename T>
struct LazyStorage {
    T *_ptr{nullptr};

    alignas(T) char _storage[sizeof(T)]{};  // NOLINT

    // ---

    constexpr LazyStorage() = default;

    constexpr LazyStorage(LazyStorage &&other) noexcept(
        noexcept(assign(std::declval<LazyStorage<T>>()))) {
        assign(std::move(other));
    }

    constexpr auto operator=(LazyStorage &&other) noexcept(
        noexcept(assign(std::declval<LazyStorage<T>>()))) -> LazyStorage & {
        assign(std::move(other));
        return *this;
    }

    constexpr LazyStorage(LazyStorage const &other) {
        assign(other);
    }

    // NOLINTNEXTLINE(cert-oop54-cpp, bugprone-unhandled-self-assignment)
    constexpr auto operator=(LazyStorage const &other) -> LazyStorage & {
        if (_ptr == other._ptr) {
            return *this;
        }
        assign(other);
        return *this;
    };

    constexpr auto operator=(T const &other) -> LazyStorage & {
        if (_ptr != nullptr) {
            destroy();
        }
        if (other._ptr == nullptr) {
            return *this;
        }
        in_place(*other._ptr);
        return *this;
    };

    constexpr auto operator=(T &&other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>)
        -> LazyStorage & {
        if (_ptr != nullptr) {
            destroy();
        }
        in_place(std::move(other));
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
    template <typename Other>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    constexpr auto assign(Other &&other) noexcept(
        lazy_storage::detail::IsAssignNoExcept<Other &&>::value) -> void {
        if (_ptr == nullptr && other._ptr == nullptr) {
            return;
        }
        if (_ptr == nullptr && other._ptr != nullptr) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::ranges::copy(other._storage, _storage);
                _ptr = std::start_lifetime_as<T>(_storage);
            } else {
                in_place(std::forward_like<Other>(*other._ptr));
            }
            return;
        }
        if (_ptr != nullptr && other._ptr != nullptr) {
            *_ptr = std::forward_like<Other>(*other._ptr);
            return;
        }
        if (_ptr != nullptr && other._ptr == nullptr) {
            destroy();
        }
    }

    constexpr auto destroy() -> void {
        _ptr->~T();
        _ptr = nullptr;
    }

    template <typename K>
    constexpr auto in_place(K &&other) -> void {
        _ptr = ::new (static_cast<void *>(_storage)) T{std::forward<K>(other)};
    }
};

}  // namespace args

#endif
