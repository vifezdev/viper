#pragma once








#include <variant>
#include <string>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace viper {


struct ErrorTag {};
inline constexpr ErrorTag err_tag{};


struct OkTag {};
inline constexpr OkTag ok_tag{};


struct ResultError {
    std::string message;
    explicit ResultError(std::string msg) : message(std::move(msg)) {}
    explicit ResultError(const char* msg) : message(msg) {}
};



template<typename T, typename E = std::string>
class Result {
public:
    
    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    Result(OkTag, U&& value)
        : storage_(std::in_place_index<0>, std::forward<U>(value)) {}

    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    Result(U&& value)
        : storage_(std::in_place_index<0>, std::forward<U>(value)) {}

    
    Result(ErrorTag, E&& error)
        : storage_(std::in_place_index<1>, std::forward<E>(error)) {}

    Result(ErrorTag, const E& error)
        : storage_(std::in_place_index<1>, error) {}

    Result(ResultError err)
        : storage_(std::in_place_index<1>, std::move(err.message)) {}

    static Result<T, E> createError(E err) { return Result<T, E>(err_tag, std::move(err)); }
    static Result<T, E> createSuccess(T val) { return Result<T, E>(ok_tag, std::move(val)); }

    
    [[nodiscard]] bool isOk() const noexcept { return storage_.index() == 0; }

    
    [[nodiscard]] bool isErr() const noexcept { return storage_.index() == 1; }

    
    [[nodiscard]] explicit operator bool() const noexcept { return isOk(); }

    
    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    [[nodiscard]] const U& value() const& {
        if (isErr()) throw std::runtime_error("Result::value() called on error");
        return std::get<0>(storage_);
    }

    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    [[nodiscard]] U& value() & {
        if (isErr()) throw std::runtime_error("Result::value() called on error");
        return std::get<0>(storage_);
    }

    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    [[nodiscard]] U&& value() && {
        if (isErr()) throw std::runtime_error("Result::value() called on error");
        return std::get<0>(std::move(storage_));
    }

    
    [[nodiscard]] const E& error() const& {
        if (isOk()) throw std::runtime_error("Result::error() called on success");
        return std::get<1>(storage_);
    }

    [[nodiscard]] E& error() & {
        if (isOk()) throw std::runtime_error("Result::error() called on success");
        return std::get<1>(storage_);
    }

    
    template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
    [[nodiscard]] U valueOr(U&& defaultValue) const& {
        if (isOk()) return std::get<0>(storage_);
        return std::forward<U>(defaultValue);
    }

private:
    
    
    using StoredT = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    std::variant<StoredT, E> storage_;
};


template<typename E>
class Result<void, E> {
public:
    
    Result() : storage_(std::in_place_index<0>, std::monostate{}) {}

    
    Result(ErrorTag, E&& error)
        : storage_(std::in_place_index<1>, std::forward<E>(error)) {}

    Result(ErrorTag, const E& error)
        : storage_(std::in_place_index<1>, error) {}

    Result(ResultError err)
        : storage_(std::in_place_index<1>, std::move(err.message)) {}

    static Result<void, E> createError(E err) { return Result<void, E>(err_tag, std::move(err)); }
    static Result<void, E> createSuccess() { return Result<void, E>(); }

    [[nodiscard]] bool isOk() const noexcept { return storage_.index() == 0; }
    [[nodiscard]] bool isErr() const noexcept { return storage_.index() == 1; }
    [[nodiscard]] explicit operator bool() const noexcept { return isOk(); }

    [[nodiscard]] const E& error() const& {
        if (isOk()) throw std::runtime_error("Result::error() called on success");
        return std::get<1>(storage_);
    }

private:
    std::variant<std::monostate, E> storage_;
};






template<typename T>
auto Ok(T&& value) -> Result<std::decay_t<T>> {
    return Result<std::decay_t<T>>(ok_tag, std::forward<T>(value));
}


inline auto Ok() -> Result<void> {
    return Result<void>();
}


template<typename T = void, typename E = std::string>
auto Err(E&& error) -> Result<T, std::decay_t<E>> {
    return Result<T, std::decay_t<E>>(err_tag, std::forward<E>(error));
}

} 
