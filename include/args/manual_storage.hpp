#ifndef ARGS_MANUAL_STORAGE_HPP
#define ARGS_MANUAL_STORAGE_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>

namespace args::detail {

template <typename T>
struct ManualStorage {
    T *ptr{nullptr};

    alignas(T) char storage[sizeof(T)]{};  // NOLINT

    // ---

    ManualStorage() = default;

    ManualStorage(ManualStorage &&) = delete;
    auto operator=(ManualStorage &&) -> ManualStorage & = delete;

    ManualStorage(ManualStorage const &other) {
        assign(other);
    }

    // NOLINTNEXTLINE(cert-oop54-cpp, bugprone-unhandled-self-assignment)
    auto operator=(ManualStorage const &other) -> ManualStorage & {
        if (ptr == other.ptr) {
            return *this;
        }
        assign(other);
        return *this;
    };

    auto operator=(T const &other) -> ManualStorage & {
        if (ptr != nullptr) {
            destroy();
        }
        in_place(&other);
        return *this;
    };

    constexpr ~ManualStorage() requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~ManualStorage() {
        if (ptr != nullptr) {
            destroy();
        }
    }

    // ---
    constexpr operator T const &() {  // NOLINT(hicpp-explicit-conversions)
        return as_ref();
    }

    [[nodiscard]] constexpr auto as_ref() const -> T const & {
        assert(ptr != nullptr);
        return *ptr;
    }

private:
    auto assign(ManualStorage const &other) -> void {
        if (ptr == nullptr && other.ptr == nullptr) {
            return;
        }
        if (ptr == nullptr && other.ptr != nullptr) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::ranges::copy(other.storage, storage);
                ptr = std::start_lifetime_as<T>(storage);
            } else {
                in_place(other.ptr);
            }
            return;
        }
        if (ptr != nullptr && other.ptr != nullptr) {
            *ptr = *other.ptr;
            return;
        }
        if (ptr != nullptr && other.ptr == nullptr) {
            destroy();
        }
    }

    auto destroy() -> void {
        ptr->~T();
        ptr = nullptr;
    }

    auto in_place(T const *other) -> void {
        ptr = ::new (static_cast<void *>(storage)) T{*other};
    }
};


}  // namespace args::detail


#endif
