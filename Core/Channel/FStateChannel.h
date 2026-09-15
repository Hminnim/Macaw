#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

template<typename T>
class FStateChannel {
public:
    using VersionType = std::uint64_t;

    struct FReadResult {
        const T* Value = nullptr;
        bool Changed = false;
    };

public:
    class FReader {
    public:
        explicit FReader(const FStateChannel& InChannel) noexcept : Channel(&InChannel) {}
		FReader() noexcept = default;
        ~FReader() noexcept = default;

        FReader(const FReader&) = default;
        FReader& operator=(const FReader&) = default;

        FReader(FReader&&) = default;
        FReader& operator=(FReader&&) = default;

    public:
        [[nodiscard]] bool HasChanged() const noexcept {
            return LastReadVersion != Channel->Version;
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return Channel->State.has_value();
        }

        [[nodiscard]] const T& Peek() const {
            return Channel->State.value();
        }

        [[nodiscard]] const T& Read() {
            const T& Value = Channel->State.value();
            LastReadVersion = Channel->Version;
            return Value;
        }

        [[nodiscard]] FReadResult ReadIfChanged() noexcept {
            if (!HasChanged()) {
                return {};
            }

            LastReadVersion = Channel->Version;

            return {
                .Value = Channel->State ? std::addressof(*Channel->State) : nullptr,
                .Changed = true
            };
        }

    private:
        const FStateChannel* Channel{ nullptr };
        VersionType LastReadVersion{ 0 };
    };

    class FWriter {
    public:
        explicit FWriter(FStateChannel& InChannel) noexcept : Channel(&InChannel) {}
		FWriter() noexcept = default;

		~FWriter() noexcept = default;

		FWriter(const FWriter&) = default;
		FWriter& operator=(const FWriter&) = default;

		FWriter(FWriter&&) = default;
		FWriter& operator=(FWriter&&) = default;

    public:
        void Write(const T& NewState) requires std::copy_constructible<T>&& std::assignable_from<T&, const T&> {
            Channel->Write(NewState);
        }

        void Write(T&& NewState) requires std::move_constructible<T>&& std::assignable_from<T&, T> {
            Channel->Write(std::move(NewState));
        }

        template<typename... Args> requires std::constructible_from<T, Args...>
        const T& Emplace(Args&&... Arguments) {
            return Channel->Emplace(std::forward<Args>(Arguments)...);
        }

        template<typename TCallable> requires std::invocable<TCallable&, T&>
        bool Modify(TCallable&& Callable) {
            return Channel->TryModify(std::forward<TCallable>(Callable));
        }

        void Clear() noexcept {
            Channel->Clear();
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return Channel->State.has_value();
        }

    private:
        FStateChannel<T>* Channel{ nullptr };
    };

    class FReadWriter {
    public:
        explicit FReadWriter(FStateChannel& InChannel) noexcept : Channel(&InChannel) {}
		explicit FReadWriter() noexcept = default;
        ~FReadWriter() noexcept = default;

        FReadWriter(const FReadWriter&) = default;
        FReadWriter& operator=(const FReadWriter&) = default;

        FReadWriter(FReadWriter&&) = default;
        FReadWriter& operator=(FReadWriter&&) = default;

    public:
        [[nodiscard]] bool HasChanged() const noexcept {
            return LastReadVersion != Channel->Version;
        }

        [[nodiscard]] bool HasValue() const noexcept {
            return Channel->State.has_value();
        }

        [[nodiscard]] const T& Peek() const {
            return Channel->State.value();
        }

        [[nodiscard]] const T& Read() {
            const T& Value = Channel->State.value();
            LastReadVersion = Channel->Version;
            return Value;
        }

        [[nodiscard]] FReadResult ReadIfChanged() noexcept {
            if (!HasChanged()) {
                return {};
            }

            LastReadVersion = Channel->Version;

            return {
                .Value = Channel->State ? std::addressof(*Channel->State) : nullptr,
                .Changed = true
            };
        }

        void Write(const T& NewState) requires std::copy_constructible<T>&& std::assignable_from<T&, const T&> {
            Channel->Write(NewState);
        }

        void Write(T&& NewState) requires std::move_constructible<T>&& std::assignable_from<T&, T> {
            Channel->Write(std::move(NewState));
        }

        template<typename... Args> requires std::constructible_from<T, Args...>
        const T& Emplace(Args&&... Arguments) {
            return Channel->Emplace(std::forward<Args>(Arguments)...);
        }

        template<typename TCallable> requires std::invocable<TCallable&, T&>
        bool Modify(TCallable&& Callable) {
            return Channel->TryModify(std::forward<TCallable>(Callable));
        }

        void Clear() noexcept {
            Channel->Clear();
        }

    private:
        FStateChannel<T>* Channel{ nullptr };
        VersionType LastReadVersion{ 0 };
    };

public:
    FStateChannel() = default;

    explicit FStateChannel(const T& InitialState) requires std::copy_constructible<T> : State(InitialState), Version(1) {}
    explicit FStateChannel(T&& InitialState) requires std::move_constructible<T> : State(std::move(InitialState)), Version(1) {}

    template<typename... Args> requires std::constructible_from<T, Args...>
    explicit FStateChannel(std::in_place_t, Args&&... Arguments) : State(std::in_place, std::forward<Args>(Arguments)...), Version(1) {}

	FStateChannel(const FStateChannel&) = delete;
	FStateChannel& operator=(const FStateChannel&) = delete;

	FStateChannel(FStateChannel&&) = delete;
	FStateChannel& operator=(FStateChannel&&) = delete;

public:
    [[nodiscard]] FReader GetReader() const noexcept {
        return FReader{ *this };
    }

    [[nodiscard]] FWriter GetWriter() noexcept {
        return FWriter{ *this };
    }

    [[nodiscard]] FReadWriter GetReadWriter() noexcept {
        return FReadWriter{ *this };
    }

private:
    void Write(const T& NewState) requires std::copy_constructible<T>&& std::assignable_from<T&, const T&> {
        if (State) {
            *State = NewState;
        }
        else {
            State.emplace(NewState);
        }

        ++Version;
    }

    void Write(T&& NewState) requires std::move_constructible<T>&& std::assignable_from<T&, T> {
        if (State) {
            *State = std::move(NewState);
        }
        else {
            State.emplace(std::move(NewState));
        }

        ++Version;
    }

    template<typename... Args> requires std::constructible_from<T, Args...>
    const T& Emplace(Args&&... Arguments) {
        State.emplace(std::forward<Args>(Arguments)...);
        ++Version;

        return *State;
    }

    template<typename TCallable> requires std::invocable<TCallable&, T&>
    bool TryModify(TCallable&& Callable) {
        if (!State) {
            return false;
        }

        std::invoke(std::forward<TCallable>(Callable), *State);
        ++Version;

        return true;
    }

    void Clear() noexcept {
        if (!State) {
            return;
        }

        State.reset();
        ++Version;
    }

private:
    std::optional<T> State{};
    VersionType Version{ 0 };
};
