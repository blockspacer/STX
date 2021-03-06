/**
 * @file option_result.h
 * @author Basit Ayantunde <rlamarrr@gmail.com>
 * @brief
 * @version  0.1
 * @date 2020-04-16
 *
 * @copyright Copyright (c) 2020
 *
 */

#pragma once

#include "stx/internal/panic_helpers.h"

// Why so long? Option and Result depend on each other. I don't know of a
// way to break the cyclic dependency, primarily because they are templated
//
// Lifetime notes:
// - Every change of state must be followed by a destruction (and construction
// if it has a non-null variant)
// - The object must be destroyed immediately after the contained value is moved
// from it.
//
// Notes:
// - Result and Option are perfect-forwarding types. It is unique to an
// interaction. It functions like a std::unique_ptr, as it doesn't allow
// implicitly copying its data content. Unless explicitly stated via the
// .clone() method.
// - We strive to make lifetime paths as visible and predictable as
// possible.
//
//

/// @file
///
/// to run tests, use:
///
/// ``` cpp
///
/// #include <iostream>
/// #include <string>
/// #include <string_view>
///
/// #include <fmt/format.h>
///
///
/// using std::move, std::string, std::string_view;
/// using namespace std::string_literals; // makes '"Hello"s' give std::string
///                                       // directly
/// using namespace std::string_view_literals;
///
/// ```

namespace stx {

/// value-variant Type for `Option<T>` representing no-value
class [[nodiscard]] NoneType {
 public:
  [[nodiscard]] constexpr NoneType() noexcept = default;
  [[nodiscard]] constexpr NoneType(NoneType const&) noexcept = default;
  [[nodiscard]] constexpr NoneType(NoneType&&) noexcept = default;
  constexpr NoneType& operator=(NoneType const&) noexcept = default;
  constexpr NoneType& operator=(NoneType&&) noexcept = default;
  constexpr ~NoneType() noexcept = default;

  [[nodiscard]] constexpr bool operator==(NoneType const&) const noexcept {
    return true;
  }
};

// value-variant for `Option<T>` representing no-value
constexpr const NoneType None = NoneType{};

/// value-variant for `Option<T>` wrapping the contained value
///
/// # Usage
///
/// Note that `Some` is only a value-forwarding type. It doesn't make copies of
/// it's constructor arguments and only accepts r-values.
///
/// What does this mean?
///
/// For example, You can:
///
/// ```cpp
/// Option a = Some(vector{1, 2, 3, 4});
/// ```
/// You can't:
///
/// ```cpp
/// vector<int> x {1, 2, 3, 4};
/// Option a = Some(x);
/// ```
/// But, to explicitly make `a` take ownership, you will:
///
/// ```cpp
/// vector<int> x {1, 2, 3, 4};
/// Option a = Some(std::move(x));
/// ```
template <Swappable T>
struct [[nodiscard]] Some {
  static_assert(!std::is_reference_v<T>,
                "Cannot use T& nor T&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");

  using value_type = T;

  /// a `Some<T>` can only be constructed with an r-value of type `T`
  [[nodiscard]] explicit constexpr Some(T&& value)
      : value_(std::forward<T&&>(value)) {}

  [[nodiscard]] constexpr Some(Some&& rhs) = default;
  constexpr Some& operator=(Some&& rhs) = default;

  constexpr Some(Some const&) = delete;
  constexpr Some& operator=(Some const&) = delete;

  constexpr ~Some() = default;

  [[nodiscard]] constexpr T const& value() const& noexcept { return value_; }
  [[nodiscard]] constexpr T& value() & noexcept { return value_; }
  [[nodiscard]] constexpr T const value() const&& { return std::move(value_); }
  [[nodiscard]] constexpr T value() && { return std::move(value_); }

  [[nodiscard]] constexpr bool operator==(Some const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Some<MutRef<T>> const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Some<ConstRef<T>> const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Some<T*> const& cmp) const
      requires equality_comparable<T> {
    return value() == *cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Some<T const*> const& cmp) const
      requires equality_comparable<T> {
    return value() == *cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(NoneType const&) const noexcept {
    return false;
  }

 private:
  T value_;

  template <Swappable Tp>
  friend class Option;
};

template <Swappable E>
struct Err;

/// value-variant for `Result<T, E>` wrapping the contained value
///
///
template <Swappable T>
struct [[nodiscard]] Ok {
  static_assert(!std::is_reference_v<T>,
                "Cannot use T& nor T&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");
  using value_type = T;

  /// an `Ok<T>` can only be constructed with an r-value of type `T`
  [[nodiscard]] explicit constexpr Ok(T&& value)
      : value_(std::forward<T&&>(value)) {}

  [[nodiscard]] constexpr Ok(Ok&& rhs) = default;
  constexpr Ok& operator=(Ok&& rhs) = default;

  constexpr Ok(Ok const&) = delete;
  constexpr Ok& operator=(Ok const&) = delete;

  constexpr ~Ok() = default;

  [[nodiscard]] constexpr bool operator==(Ok const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Ok<ConstRef<T>> const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Ok<MutRef<T>> const& cmp) const
      requires equality_comparable<T> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Ok<T*> const& cmp) const
      requires equality_comparable<T> {
    return value() == *cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Ok<const T*> const& cmp) const
      requires equality_comparable<T> {
    return value() == *cmp.value();
  }

  template <typename U>
  [[nodiscard]] constexpr bool operator==(Err<U> const&) const noexcept {
    return false;
  }

  [[nodiscard]] constexpr T const& value() const& noexcept { return value_; }
  [[nodiscard]] constexpr T& value() & noexcept { return value_; }
  [[nodiscard]] constexpr T const value() const&& { return std::move(value_); }
  [[nodiscard]] constexpr T value() && { return std::move(value_); }

 private:
  T value_;

  template <Swappable Tp, Swappable Err>
  friend class Result;
};

/// error-value variant for `Result<T, E>` wrapping the contained error
template <Swappable E>
struct [[nodiscard]] Err {
  static_assert(!std::is_reference_v<E>,
                "Cannot use E& nor E&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");
  using value_type = E;

  // an `Err<E>` can only be constructed with an r-value of type `E`
  [[nodiscard]] explicit constexpr Err(E&& value)
      : value_(std::forward<E&&>(value)) {}

  [[nodiscard]] constexpr Err(Err&& rhs) = default;
  constexpr Err& operator=(Err&& rhs) = default;

  constexpr Err(Err const&) = delete;
  constexpr Err& operator=(Err const&) = delete;

  constexpr ~Err() = default;

  [[nodiscard]] constexpr bool operator==(Err const& cmp) const
      requires equality_comparable<E> {
    return value() == cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Err<ConstRef<E>> const& cmp) const
      requires equality_comparable<E> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Err<MutRef<E>> const& cmp) const
      requires equality_comparable<E> {
    return value() == cmp.value().get();
  }

  [[nodiscard]] constexpr bool operator==(Err<E*> const& cmp) const
      requires equality_comparable<E> {
    return value() == *cmp.value();
  }

  [[nodiscard]] constexpr bool operator==(Err<E const*> const& cmp) const
      requires equality_comparable<E> {
    return value() == *cmp.value();
  }

  [[nodiscard]] constexpr E const& value() const& noexcept { return value_; }
  [[nodiscard]] constexpr E& value() & noexcept { return value_; }
  [[nodiscard]] constexpr E const value() const&& { return std::move(value_); }
  [[nodiscard]] constexpr E value() && { return std::move(value_); }

 private:
  E value_;

  template <Swappable Tp, Swappable Err>
  friend class Result;
};

template <Swappable T, Swappable E>
class [[nodiscard]] Result;

//! Optional values.
//!
//! Type `Option` represents an optional value: every `Option`
//! is either `Some` and contains a value, or `None`, and
//! does not.
//! They have a number of uses:
//!
//! * Initial values
//! * Return values for functions that are not defined over their entire input
//! range (partial functions)
//! * Return value for otherwise reporting simple errors, where `None` is
//! returned on error
//! * Optional struct fields
//! * Struct fields that can be loaned or "taken"
//! * Optional function arguments
//! * Nullable pointers
//! * Swapping things out of difficult situations
//!
//! `Option`'s are commonly paired with pattern matching to query the
//! presence of a value and take action, always accounting for the `None`s
//! case.
//!
//! ```
//! auto divide = [](double numerator, double denominator) -> Option<double> {
//!   if (denominator == 0.0) {
//!     return None;
//!   } else {
//!     return Some(numerator / denominator);
//!   }
//! };
//!
//! // The return value of the function is an option
//! auto result = divide(2.0, 3.0);
//! move(result).match([](double value) { fmt::print("{}\n", value); },
//!                     []() { fmt::print("has no value"); });
//! ```
//!
//!
template <Swappable T>
class [[nodiscard]] Option {
 public:
  using value_type = T;

  static_assert(!std::is_reference_v<T>,
                "Cannot use T& nor T&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");

  [[nodiscard]] constexpr Option(Some<T>&& some)
      : storage_value_(std::move(some.value_)), is_none_(false) {}

  [[nodiscard]] constexpr Option(NoneType const&) noexcept
      : is_none_(true) {}  // NOLINT

  // constexpr?
  // placement-new!!
  // we can't make this constexpr
  [[nodiscard]] Option(Option&& rhs) : is_none_(rhs.is_none_) {
    if (rhs.is_some()) {
      new (&storage_value_) T(std::move(rhs.storage_value_));
    }
  }

  Option& operator=(Option&& rhs) {
    // contained object is destroyed as appropriate in the parent scope
    if (is_some() && rhs.is_some()) {
      std::swap(storage_value_, rhs.storage_value_);
    } else if (is_some() && rhs.is_none()) {
      new (&rhs.storage_value_) T(std::move(storage_value_));
      storage_value_.~T();
      is_none_ = true;
      rhs.is_none_ = false;
      // we let the ref'd object destroy the object instead
    } else if (is_none() && rhs.is_some()) {
      new (&storage_value_) T(std::move(rhs.storage_value_));
      rhs.storage_value_.~T();
      rhs.is_none_ = true;
      is_none_ = false;
    }

    return *this;
  }

  constexpr Option() = delete;
  constexpr Option(Option const&) = delete;
  constexpr Option& operator=(Option const&) = delete;

  constexpr ~Option() noexcept {
    if (is_some()) {
      storage_value_.~T();
    }
  }

  [[nodiscard]] constexpr bool operator==(Option const& cmp) const
      requires equality_comparable<T> {
    if (is_some() && cmp.is_some()) {
      return value_cref_() == cmp.value_cref_();
    } else if (is_none() && cmp.is_none()) {
      return true;
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Some<T> const& cmp) const
      requires equality_comparable<T> {
    if (is_some()) {
      return value_cref_() == cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Some<ConstRef<T>> const& cmp) const
      requires equality_comparable<T> {
    if (is_some()) {
      return value_cref_() == cmp.value().get();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Some<MutRef<T>> const& cmp) const
      requires equality_comparable<T> {
    if (is_some()) {
      return value_cref_() == cmp.value().get();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Some<const T*> const& cmp) const
      requires equality_comparable<T> {
    if (is_some()) {
      return value_cref_() == *cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Some<T*> const& cmp) const
      requires equality_comparable<T> {
    if (is_some()) {
      return value_cref_() == *cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(NoneType const&) const noexcept {
    return is_none();
  }

  /// Returns `true` if this Option is a `Some` value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// ASSERT_TRUE(x.is_some());
  ///
  /// Option<int> y = None;
  /// ASSERT_FALSE(y.is_some());
  /// ```
  ///
  [[nodiscard]] constexpr bool is_some() const noexcept { return !is_none(); }

  /// Returns `true` if the option is a `None` value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// ASSERT_FALSE(x.is_none());
  ///
  /// Option<int> y = None;
  /// ASSERT_TRUE(y.is_none());
  /// ```
  [[nodiscard]] constexpr bool is_none() const noexcept { return is_none_; }

  /// Returns `true` if the option is a `Some` value containing the given
  /// value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// ASSERT_TRUE(x.contains(2));
  ///
  /// Option y = Some(3);
  /// ASSERT_FALSE(y.contains(2));
  ///
  /// Option<int> z = None;
  /// ASSERT_FALSE(z.contains(2));
  /// ```
  template <typename CmpType>
  requires equality_comparable<CmpType const&, T const&>  //
      [[nodiscard]] constexpr bool contains(CmpType const& cmp) const {
    if (is_some()) {
      return value_cref_() == cmp;
    } else {
      return false;
    }
  }

  /// Returns the value of evaluating the `predicate` on the contained value if
  /// the `Option` is a `Some`, else returns `false`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// auto even = [](auto x) { return x == 2; };
  /// ASSERT_TRUE(x.exists(even));
  ///
  /// ```
  template <typename UnaryPredicate>
  requires invocable<UnaryPredicate&&, T const&>&&
      convertible_to<invoke_result<UnaryPredicate&&, T const&>,
                     bool>  //
      [[nodiscard]] constexpr bool exists(UnaryPredicate&& predicate) const {
    if (is_some()) {
      return std::forward<UnaryPredicate&&>(predicate)(value_cref_());
    } else {
      return false;
    }
  }

  /// Returns an l-value reference to the contained value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is a `None`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto x = make_some(9);
  /// int& y = x.value();
  /// y = 2;
  ///
  /// ASSERT_EQ(x, Some(2));
  /// ```
  [[nodiscard]] T& value() & noexcept {
    if (is_none_) internal::option::no_lref();
    return value_ref_();
  }

  /// Returns a const l-value reference to the contained value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is a `None`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto const x = make_some(9);
  /// int const& y = x.value();
  ///
  /// ASSERT_EQ(y, 9);
  /// ```
  [[nodiscard]] T const& value() const& noexcept {
    if (is_none_) internal::option::no_lref();
    return value_cref_();
  }

  /// Use `unwrap()` instead
  [[deprecated("Use `unwrap()` instead")]] T value() && = delete;
  /// Use `unwrap()` instead
  [[deprecated("Use `unwrap()` instead")]] T const value() const&& = delete;

  /// Converts from `Option<T> const&` or `Option<T> &` to
  /// `Option<ConstRef<T>>`.
  ///
  /// # NOTE
  /// `ConstRef<T>` is an alias for `std::reference_wrapper<T const>` and
  /// guides against reference-collapsing
  [[nodiscard]] constexpr auto as_cref() const& noexcept
      -> Option<ConstRef<T>> {
    if (is_some()) {
      return Some<ConstRef<T>>(ConstRef<T>(value_cref_()));
    } else {
      return None;
    }
  }

  [[deprecated(
      "calling Option::as_cref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
  [[nodiscard]] constexpr auto
  as_cref() const&& noexcept -> Option<ConstRef<T>> = delete;

  /// Converts from `Option<T>` to `Option<MutRef<T>>`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto mutate = [](Option<int>& r) {
  ///  r.as_ref().match([](MutRef<int> ref) { ref.get() = 42; },
  ///                       []() { });
  /// };
  ///
  /// auto x = make_some(2);
  /// mutate(x);
  /// ASSERT_EQ(x, Some(42));
  ///
  /// auto y = make_none<int>();
  /// mutate(y);
  /// ASSERT_EQ(y, None);
  /// ```
  [[nodiscard]] constexpr auto as_ref() & noexcept -> Option<MutRef<T>> {
    if (is_some()) {
      return Some<MutRef<T>>(MutRef<T>(value_ref_()));
    } else {
      return None;
    }
  }

  [[deprecated(
      "calling Option::as_ref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
  [[nodiscard]] constexpr auto
  as_ref() && noexcept -> Option<MutRef<T>> = delete;

  /// Unwraps an option, yielding the content of a `Some`.
  ///
  /// # Panics
  ///
  /// Panics if the value is a `None` with a custom panic message provided by
  /// `msg`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some("value"s);
  /// ASSERT_EQ(move(x).expect("the world is ending"), "value");
  ///
  /// Option<string> y = None;
  /// ASSERT_ANY_THROW(move(y).expect("the world is ending")); // panics with
  ///                                                          // the world is
  ///                                                          // ending
  /// ```
  [[nodiscard]] auto expect(std::string_view msg) && -> T {
    if (is_some()) {
      return std::move(value_ref_());
    } else {
      internal::option::expect_value_failed(std::move(msg));
    }
  }

  /// Moves the value out of the `Option<T>` if it is in the variant state of
  /// `Some<T>`.
  ///
  /// In general, because this function may panic, its use is discouraged.
  /// Instead, prefer to use pattern matching and handle the `None`
  /// case explicitly.
  ///
  /// # Panics
  ///
  /// Panics if its value equals `None`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some("air"s);
  /// ASSERT_EQ(move(x).unwrap(), "air");
  ///
  /// Option<string> y = None;
  /// ASSERT_ANY_THROW(move(y).unwrap());
  /// ```
  [[nodiscard]] auto unwrap() && -> T {
    if (is_some()) {
      return std::move(value_ref_());
    } else {
      internal::option::no_value();
    }
  }

  /// Returns the contained value or an alternative: `alt`.
  ///
  /// Arguments passed to `unwrap_or` are eagerly evaluated; if you are passing
  /// the result of a function call, it is recommended to use `unwrap_or_else`,
  /// which is lazily evaluated.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// ASSERT_EQ(Option(Some("car"s)).unwrap_or("bike"), "car");
  /// ASSERT_EQ(make_none<string>().unwrap_or("bike"), "bike");
  /// ```
  [[nodiscard]] constexpr auto unwrap_or(T&& alt) && -> T {
    if (is_some()) {
      return std::move(value_ref_());
    } else {
      return std::move(alt);
    }
  }

  /// Returns the contained value or computes it from a closure.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// int k = 10;
  /// auto alt = [&k]() { return 2 * k; };
  ///
  /// ASSERT_EQ(make_some(4).unwrap_or_else(alt), 4);
  /// ASSERT_EQ(make_none<int>().unwrap_or_else(alt), 20);
  /// ```
  template <typename Fn>
  requires invocable<Fn&&>  //
      [[nodiscard]] constexpr auto unwrap_or_else(Fn&& op) && -> T {
    if (is_some()) {
      return std::move(value_ref_());
    } else {
      return std::forward<Fn&&>(op)();
    }
  }

  /// Maps an `Option<T>` to `Option<U>` by applying a function to a contained
  /// value and therefore, consuming/moving the contained value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// Converts an `Option<string>` into an `Option<size_t>`,
  /// consuming the original:
  ///
  ///
  /// ```
  /// Option maybe_string = Some("Hello, World!"s);
  /// // `Option::map` will only work on Option as an r-value and assumes the
  /// //  object in it is about to be moved
  /// auto maybe_len = move(maybe_string).map([](auto s){ return s.size();
  /// });
  /// // maybe_string is invalid and should not be used from here since we
  /// // `std::move`-d from it
  ///
  /// ASSERT_EQ(maybe_len, Some<size_t>(13));
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto map(
          Fn&& op) && -> Option<invoke_result<Fn&&, T&&>> {
    if (is_some()) {
      return Some<invoke_result<Fn&&, T&&>>(
          std::forward<Fn&&>(op)(std::move(value_ref_())));
    } else {
      return None;
    }
  }

  /// Applies a function to the contained value (if any),
  /// or returns the provided alternative (if not).
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some("foo"s);
  /// auto alt_fn = [](auto s) { return s.size(); };
  /// ASSERT_EQ(move(x).map_or(alt_fn, 42UL), 3UL);
  ///
  /// Option<string> y = None;
  /// ASSERT_EQ(move(y).map_or(alt_fn, 42UL), 42UL);
  /// ```
  template <typename Fn, typename A>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto map_or(
          Fn&& op, A&& alt) && -> invoke_result<Fn&&, T&&> {
    if (is_some()) {
      return std::forward<Fn&&>(op)(std::move(value_ref_()));
    } else {
      return std::forward<A&&>(alt);
    }
  }

  /// Applies a function to the contained value (if any),
  /// or computes a default (if not).
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// size_t k = 21;
  /// auto map_fn = [] (auto s) { return s.size(); };
  /// auto alt_fn = [&k] () { return 2UL * k; };
  ///
  /// Option x = Some("foo"s);
  /// ASSERT_EQ(move(x).map_or_else(map_fn, alt_fn), 3);
  ///
  /// Option<string> y = None;
  /// ASSERT_EQ(move(y).map_or_else(map_fn, alt_fn), 42);
  /// ```
  template <typename Fn, typename AltFn>
  requires invocable<Fn&&, T&&>&& invocable<AltFn&&>  //
      [[nodiscard]] constexpr auto map_or_else(
          Fn&& op, AltFn&& alt) && -> invoke_result<Fn&&, T&&> {
    if (is_some()) {
      return std::forward<Fn&&>(op)(std::move(value_ref_()));
    } else {
      return std::forward<AltFn&&>(alt)();
    }
  }

  /// Transforms the `Option<T>` into a `Result<T, E>`, mapping `Some<T>` to
  /// `Ok<T>` and `None` to `Err<E>`.
  ///
  /// Arguments passed to `ok_or` are eagerly evaluated; if you are passing the
  /// result of a function call, it is recommended to use `ok_or_else`, which is
  /// lazily evaluated.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some("foo"s);
  /// ASSERT_EQ(move(x).ok_or(0), Ok("foo"s));
  ///
  /// Option<string> y = None;
  /// ASSERT_EQ(move(y).ok_or(0), Err(0));
  /// ```
  // copies the argument if not an r-value
  template <typename E>
  [[nodiscard]] constexpr auto ok_or(E error) && -> Result<T, E> {
    if (is_some()) {
      return Ok<T>(std::move(value_ref_()));
    } else {
      // tries to copy if it is an l-value ref and moves if it is an r-value
      return Err<E>(std::forward<E>(error));
    }
  }

  /// Transforms the `Option<T>` into a `Result<T, E>`, mapping `Some<T>` to
  /// `Ok<T>` and `None` to `Err(op())`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto else_fn = [] () { return 0; };
  ///
  /// Option x = Some("foo"s);
  /// ASSERT_EQ(move(x).ok_or_else(else_fn), Ok("foo"s));
  ///
  /// Option<string> y = None;
  /// ASSERT_EQ(move(y).ok_or_else(else_fn), Err(0));
  /// ```
  // can return reference but the user will get the error
  template <typename Fn>
  requires invocable<Fn&&>  //
      [[nodiscard]] constexpr auto ok_or_else(
          Fn&& op) && -> Result<T, invoke_result<Fn&&>> {
    if (is_some()) {
      return Ok<T>(std::move(value_ref_()));
    } else {
      return Err<invoke_result<Fn&&>>(std::forward<Fn&&>(op)());
    }
  }

  /// Returns `None` if the option is `None`, otherwise returns `cmp`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option a = Some(2);
  /// Option<string> b = None;
  /// ASSERT_EQ(move(a).AND(move(b)), None);
  ///
  /// Option<int> c = None;
  /// Option d = Some("foo"s);
  /// ASSERT_EQ(move(c).AND(move(d)), None);
  ///
  /// Option e = Some(2);
  /// Option f = Some("foo"s);
  /// ASSERT_EQ(move(e).AND(move(f)), Some("foo"s));
  ///
  /// Option<int> g = None;
  /// Option<string> h = None;
  /// ASSERT_EQ(move(g).AND(move(h)), None);
  /// ```
  // won't compile if a normal reference is passed since it is not copyable
  // if an rvalue, will pass. We are not forwarding refences here.
  // a requirement here is for it to be constructible with a None
  template <typename U>  //
  [[nodiscard]] constexpr auto AND(Option<U>&& cmp) && -> Option<U> {
    if (is_some()) {
      return std::forward<Option<U>&&>(cmp);
    } else {
      return None;
    }
  }

  /// Returns `None` if the option is `None`, otherwise calls `op` with the
  /// wrapped value and returns the result.
  ///
  /// Some languages call this operation flatmap.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto sq = [] (auto x) -> Option<int> { return Some(x * x); };
  /// auto nope = [] (auto) -> Option<int> { return None; };
  ///
  /// ASSERT_EQ(make_some(2).and_then(sq).and_then(sq), Some(16));
  /// ASSERT_EQ(make_some(2).and_then(sq).and_then(nope), None);
  /// ASSERT_EQ(make_some(2).and_then(nope).and_then(sq), None);
  /// ASSERT_EQ(make_none<int>().and_then(sq).and_then(sq), None);
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto and_then(
          Fn&& op) && -> invoke_result<Fn&&, T&&> {
    if (is_some()) {
      return std::forward<Fn&&>(op)(std::move(value_ref_()));
    } else {
      return None;
    }
  }

  /// Returns `None` if the option is `None`, otherwise calls `predicate`
  /// with the wrapped value and returns:
  ///
  /// - `Some<T>` if `predicate` returns `true` on invocation on the value.
  /// - `None` if `predicate` returns `false` on invocation on the value.
  ///
  /// `filter()` lets you decide which elements to keep.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto is_even = [](int n) -> bool { return n % 2 == 0; };
  ///
  /// ASSERT_EQ(make_none<int>().filter(is_even), None);
  /// ASSERT_EQ(make_some(3).filter(is_even), None);
  /// ASSERT_EQ(make_some(4).filter(is_even), Some(4));
  /// ```
  template <typename UnaryPredicate>
  requires invocable<UnaryPredicate&&, T const&>&&
      convertible_to<invoke_result<UnaryPredicate&&, T const&>,
                     bool>  //
      [[nodiscard]] constexpr auto filter(
          UnaryPredicate&& predicate) && -> Option {
    if (is_some() && std::forward<UnaryPredicate&&>(predicate)(value_cref_())) {
      return std::move(*this);
    } else {
      return None;
    }
  }

  /// Returns `None` if the option is `None`, otherwise calls `predicate`
  /// with the wrapped value and returns:
  ///
  /// - `Some<T>` if `predicate` returns `false` on invocation on the value.
  /// - `None` if `predicate` returns `true` on invocation on the value.
  ///
  /// `filter_not()` lets you decide which elements to keep.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto is_even = [](int n) -> bool { return n % 2 == 0; };
  ///
  /// ASSERT_EQ(make_none<int>().filter_not(is_even), None);
  /// ASSERT_EQ(make_some(3).filter_not(is_even), Some(3));
  /// ASSERT_EQ(make_some(4).filter_not(is_even), None);
  /// ```
  template <typename UnaryPredicate>
  requires invocable<UnaryPredicate&&, T const&>&&
      convertible_to<invoke_result<UnaryPredicate&&, T const&>,
                     bool>  //
      [[nodiscard]] constexpr auto filter_not(
          UnaryPredicate&& predicate) && -> Option {
    if (is_some() &&
        !std::forward<UnaryPredicate&&>(predicate)(value_cref_())) {
      return std::move(*this);
    } else {
      return None;
    }
  }

  /// Returns the option if it contains a value, otherwise returns `alt`.
  ///
  /// Arguments passed to `OR` are eagerly evaluated; if you are passing the
  /// result of a function call, it is recommended to use `or_else`, which is
  /// lazily evaluated.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option a = Some(2);
  /// Option<int> b = None;
  /// ASSERT_EQ(move(a).OR(move(b)), Some(2));
  ///
  /// Option<int> c = None;
  /// Option d = Some(100);
  /// ASSERT_EQ(move(c).OR(move(d)), Some(100));
  ///
  /// Option e = Some(2);
  /// Option f = Some(100);
  /// ASSERT_EQ(move(e).OR(move(f)), Some(2));
  ///
  /// Option<int> g = None;
  /// Option<int> h = None;
  /// ASSERT_EQ(move(g).OR(move(h)), None);
  /// ```
  [[nodiscard]] constexpr auto OR(Option&& alt) && -> Option {
    if (is_some()) {
      return std::move(*this);
    } else {
      return std::move(alt);
    }
  }

  /// Returns the option if it contains a value, otherwise calls `f` and
  /// returns the result.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto nobody = []() -> Option<string> { return None; };
  /// auto vikings = []() -> Option<string> { return Some("vikings"s); };
  ///
  /// ASSERT_EQ(Option(Some("barbarians"s)).or_else(vikings),
  /// Some("barbarians"s));
  /// ASSERT_EQ(make_none<string>().or_else(vikings), Some("vikings"s));
  /// ASSERT_EQ(make_none<string>().or_else(nobody), None);
  /// ```
  template <typename Fn>
  requires invocable<Fn&&>  //
      [[nodiscard]] constexpr auto or_else(Fn&& op) && -> Option {
    if (is_some()) {
      return std::move(*this);
    } else {
      return std::forward<Fn&&>(op)();
    }
  }

  /// Returns whichever one of this object or `alt` is a `Some<T>` variant,
  /// otherwise returns `None` if neither or both are a `Some<T>` variant.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option a = Some(2);
  /// Option<int> b = None;
  /// ASSERT_EQ(move(a).XOR(move(b)), Some(2));
  ///
  /// Option<int> c = None;
  /// Option d = Some(2);
  /// ASSERT_EQ(move(c).XOR(move(d)), Some(2));
  ///
  /// Option e = Some(2);
  /// Option f = Some(2);
  /// ASSERT_EQ(move(e).XOR(move(f)), None);
  ///
  /// Option<int> g = None;
  /// Option<int> h = None;
  /// ASSERT_EQ(move(g).XOR(move(h)), None);
  /// ```
  [[nodiscard]] constexpr auto XOR(Option&& alt) && -> Option {
    if (is_some() && alt.is_none()) {
      return std::move(*this);
    } else if (is_none() && alt.is_some()) {
      return std::move(alt);
    } else {
      return None;
    }
  }

  /// Takes the value out of the option, leaving a `None` in its place.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option a = Some(2);
  /// auto b = a.take();
  /// ASSERT_EQ(a, None);
  /// ASSERT_EQ(b, Some(2));
  ///
  /// Option<int> c  = None;
  /// auto d = c.take();
  /// ASSERT_EQ(c, None);
  /// ASSERT_EQ(d, None);
  /// ```
  [[nodiscard]] constexpr auto take() -> Option {
    if (is_some()) {
      auto some = Some<T>(std::move(value_ref_()));
      value_ref_().~T();
      is_none_ = true;
      return std::move(some);
    } else {
      return None;
    }
  }

  /// Replaces the actual value in the option by the value given in parameter,
  /// returning the old value if present,
  /// leaving a `Some` in its place without deinitializing either one.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// auto old_x = x.replace(5);
  /// ASSERT_EQ(x, Some(5));
  /// ASSERT_EQ(old_x, Some(2));
  ///
  /// Option<int> y = None;
  /// auto old_y = y.replace(3);
  /// ASSERT_EQ(y, Some(3));
  /// ASSERT_EQ(old_y, None);
  /// ```
  [[nodiscard]] auto replace(T&& replacement) -> Option {
    if (is_some()) {
      std::swap(replacement, value_ref_());
      return Some<T>(std::move(replacement));
    } else {
      new (&storage_value_) T(std::forward<T&&>(replacement));
      is_none_ = false;
      return None;
    }
  }

  /// Replaces the actual value in the option by the value given in parameter,
  /// returning the old value if present,
  /// leaving a `Some` in its place without deinitializing either one.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some(2);
  /// auto old_x = x.replace(5);
  /// ASSERT_EQ(x, Some(5));
  /// ASSERT_EQ(old_x, Some(2));
  ///
  /// Option<int> y = None;
  /// auto old_y = y.replace(3);
  /// ASSERT_EQ(y, Some(3));
  /// ASSERT_EQ(old_y, None);
  /// ```
  [[nodiscard]] auto replace(T const& replacement) -> Option {
    if (is_some()) {
      T copy = replacement;
      std::swap(copy, value_ref_());
      return Some<T>(std::move(copy));
    } else {
      new (&storage_value_) T(replacement);
      is_none_ = false;
      return None;
    }
  }

  /// Returns a copy of the option and its contents.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x  = Some(8);
  ///
  /// ASSERT_EQ(x, x.clone());
  /// ```
  [[nodiscard]] constexpr auto clone() const -> Option
      requires copy_constructible<T> {
    if (is_some()) {
      return Some<T>(T(value_cref_()));
    } else {
      return None;
    }
  }

  /// Unwraps an option, expecting `None` and returning nothing.
  ///
  /// # Panics
  ///
  /// Panics if the value is a `Some`, with a panic message including the
  /// passed message, and the content of the `Some`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto divide = [](double num, double denom) -> Option<double> {
  /// if (denom == 0.0) return None;
  ///  return Some(num / denom);
  /// };
  ///
  /// EXPECT_DEATH(divide(0.0, 1.0).unwrap_none());
  /// EXPECT_NO_THROW(divide(1.0, 0.0).unwrap_none());
  /// ```
  void expect_none(std::string_view msg) && {
    if (is_some()) {
      internal::option::expect_none_failed(std::move(msg), value_cref_());
    }
  }

  /// Unwraps an option, expecting `None` and returning nothing.
  ///
  /// # Panics
  ///
  /// Panics if the value is a `Some`, with a panic message including the
  /// passed message, and the content of the `Some`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto divide = [](double num, double denom) -> Option<double> {
  /// if (denom == 0.0) return None;
  ///  return Some(num / denom);
  /// };
  ///
  /// EXPECT_DEATH(divide(0.0, 1.0).expect_none("zero dividend"));
  /// EXPECT_NO_THROW(divide(1.0, 0.0).expect_none("zero dividend"));
  /// ```
  void unwrap_none() && {
    if (is_some()) {
      internal::option::no_none(value_cref_());
    }
  }

  /// Returns the contained value or a default of T
  ///
  /// Consumes this object and returns its `Some<T>` value if it is a `Some<T>`
  /// variant, else if this object is a `None` variant, returns the default of
  /// the value type `T`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Option x = Some("Ten"s);
  /// Option<string> y = None;
  ///
  /// ASSERT_EQ(move(x).unwrap_or_default(), "Ten"s);
  /// ASSERT_EQ(move(y).unwrap_or_default(), ""s);
  /// ```
  [[nodiscard]] constexpr auto unwrap_or_default() && -> T
      requires default_constructible<T> {
    if (is_some()) {
      return std::move(value_ref_());
    } else {
      return T();
    }
  }

  /// Dereferences the pointer or iterator, therefore returning a const
  /// reference to the pointed-to value (`Option<ConstRef<V>>`).
  ///
  /// Leaves the original Option in-place, creating a new one with an l-value
  /// reference to the original one.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto str = "Hello"s;
  /// Option x = Some(&str);
  /// ASSERT_EQ(x.as_const_deref().unwrap().get(), "Hello"s);
  ///
  /// Option<string*> y = None;
  /// ASSERT_EQ(y.as_const_deref(), None);
  /// ```
  [[nodiscard]] constexpr auto as_const_deref()
      const& requires ConstDerefable<T> {
    if (is_some()) {
      return Option<ConstDeref<T>>(
          Some<ConstDeref<T>>(ConstDeref<T>(*value_cref_())));
    } else {
      return Option<ConstDeref<T>>(None);
    }
  }

  [[deprecated(
      "calling Result::as_const_deref() on an r-value, therefore binding a "
      "reference to an object that is marked to be moved")]]  //
  [[nodiscard]] constexpr auto
  as_const_deref() const&& requires ConstDerefable<T> = delete;

  /// Dereferences the pointer or iterator, therefore returning a mutable
  /// reference to the pointed-to value (`Option<MutRef<V>>`).
  ///
  /// Leaves the original `Option` in-place, creating a new one containing a
  /// mutable reference to the inner pointer's dereference value type.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto str = "Hello"s;
  /// Option x = Some(&str);
  /// x.as_mut_deref().unwrap().get() = "World"s;
  ///
  /// ASSERT_EQ(str, "World"s);
  ///
  /// Option<string*> z = None;
  /// ASSERT_EQ(z.as_mut_deref(), None);
  /// ```
  [[nodiscard]] constexpr auto as_mut_deref() & requires MutDerefable<T> {
    if (is_some()) {
      // the value it points to is not const lets assume the class's state is
      // mutated through the pointer and not make this a const op
      return Option<MutDeref<T>>(Some<MutDeref<T>>(MutDeref<T>(*value_ref_())));
    } else {
      return Option<MutDeref<T>>(None);
    }
  }

  [[deprecated(
      "calling Result::as_mut_deref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
      [[nodiscard]] constexpr auto
      as_mut_deref() &&
      requires MutDerefable<T> = delete;

  /// Calls the parameter `some_fn` with the value if this `Option` is a
  /// `Some<T>` variant, else calls `none_fn`. This `Option` is consumed
  /// afterward.
  ///
  /// The return type of both parameters must be same (without expecting
  /// implicit conversions). They can also both return nothing `void`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto j = make_some("James"s).match([](string name) { return name; },
  ///                                    []() { return "<unidentified>"s; });
  /// ASSERT_EQ(j, "James"s);
  ///
  /// auto k = make_none<string>().match([](string name) { return name; },
  ///                                    []() { return "<unidentified>"s; });
  /// ASSERT_EQ(k, "<unidentified>"s);
  /// ```
  template <typename SomeFn, typename NoneFn>
  requires invocable<SomeFn&&, T&&>&& invocable<NoneFn&&>  //
      [[nodiscard]] constexpr auto match(
          SomeFn&& some_fn,
          NoneFn&& none_fn) && -> invoke_result<SomeFn&&, T&&> {
    if (is_some()) {
      return std::forward<SomeFn&&>(some_fn)(std::move(value_ref_()));
    } else {
      return std::forward<NoneFn&&>(none_fn)();
    }
  }

 private:
  union {
    T storage_value_;
  };
  bool is_none_;

  [[nodiscard]] constexpr T& value_ref_() { return storage_value_; }

  [[nodiscard]] constexpr T const& value_cref_() const {
    return storage_value_;
  }
};

//! ### Error handling with the `Result` type.
//!
//! `Result<T, E>` is a type used for returning and propagating
//! errors. It is a class with the variants: `Ok<T>`, representing
//! success and containing a value, and `Err<E>`, representing error
//! and containing an error value.
//!
//!
//! Functions return `Result` whenever errors are expected and
//! recoverable.
//!
//! A simple function returning `Result` might be
//! defined and used like so:
//!
//! ``` cpp
//! enum class Version { Version1 = 1, Version2 = 2 };
//!
//! auto parse_version =
//!      [](array<uint8_t, 5> const& header) -> Result<Version, string_view> {
//!    switch (header.at(0)) {
//!      case 1:
//!        return Ok(Version::Version1);
//!      case 2:
//!        return Ok(Version::Version2);
//!      default:
//!        return Err("invalid version"sv);
//!    }
//!  };
//!
//! parse_version({1u, 2u, 3u, 4u, 5u})
//!     .match(
//!         [](auto value) { fmt::print("Working with version: {}\n", value);
//!         },
//!         [](auto err) { fmt::print("Error parsing header: {}\n", err); });
//! ```
//!
//!
//! `Result` comes with some convenience methods that make working with it more
//! succinct.
//!
//! ``` cpp
//! Result<int, int> good_result = Ok(10);
//! Result<int, int> bad_result = Err(10);
//!
//! // The `is_ok` and `is_err` methods do what they say.
//! ASSERT_TRUE(good_result.is_ok() && !good_result.is_err());
//! ASSERT_TRUE(bad_result.is_err() && !bad_result.is_ok());
//! ```
//!
//! `Result` is a type that represents either success (`Ok`) or failure (`Err`).
//!
//! Result is either in the Ok or Err state at any point in time
//!
template <Swappable T, Swappable E>
class [[nodiscard]] Result {
 public:
  static_assert(!std::is_reference_v<T>,
                "Cannot use T& nor T&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");

  static_assert(!std::is_reference_v<E>,
                "Cannot use E& nor E&& for type, To prevent subtleties use "
                "type wrappers like std::reference_wrapper or any of the "
                "`stx::ConstRef` or `stx::MutRef` specialized aliases instead");

  using value_type = T;
  using error_type = E;

  [[nodiscard]] constexpr Result(Ok<T>&& result)
      : is_ok_(true), storage_value_(std::forward<T>(result.value_)) {}

  [[nodiscard]] constexpr Result(Err<E>&& err)
      : is_ok_(false), storage_err_(std::forward<E>(err.value_)) {}

  // not possible as constexpr yet:
  // 1 - we need to check which variant is present
  // 2 - the union will be default-constructed (empty) and we thus need to call
  // placement-new in the constructor block
  [[nodiscard]] Result(Result&& rhs) : is_ok_(rhs.is_ok_) {
    // not correct
    if (rhs.is_ok()) {
      new (&storage_value_) T(std::move(rhs.storage_value_));
    } else {
      new (&storage_err_) E(std::move(rhs.storage_err_));
    }
  }

  [[nodiscard]] Result& operator=(Result&& rhs) {
    if (is_ok() && rhs.is_ok()) {
      std::swap(value_ref_(), rhs.value_ref_());
    } else if (is_ok() && rhs.is_err()) {
      // we need to place a new value in here (discarding old value)
      storage_value_.~T();
      new (&storage_err_) E(std::move(rhs.storage_err_));
      is_ok_ = false;
    } else if (is_err() && rhs.is_ok()) {
      storage_err_.~E();
      new (&storage_value_) T(std::move(rhs.storage_value_));
      is_ok_ = true;
    } else {
      // both are errs
      std::swap(err_ref_(), rhs.err_ref_());  // NOLINT
    }
    return *this;
  }

  Result() = delete;
  Result(Result const& rhs) = delete;
  Result& operator=(Result const& rhs) = delete;

  constexpr ~Result() noexcept {
    if (is_ok()) {
      storage_value_.~T();
    } else {
      storage_err_.~E();
    }
  };

  [[nodiscard]] constexpr bool operator==(Ok<T> const& cmp) const
      requires equality_comparable<T> {
    if (is_ok()) {
      return value_cref_() == cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Ok<ConstRef<T>> const& cmp) const
      requires equality_comparable<T> {
    if (is_ok()) {
      return value_cref_() == cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Ok<MutRef<T>> const& cmp) const
      requires equality_comparable<T> {
    if (is_ok()) {
      return value_cref_() == cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Ok<T const*> const& cmp) const
      requires equality_comparable<T> {
    if (is_ok()) {
      return value_cref_() == *cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Ok<T*> const& cmp) const
      requires equality_comparable<T> {
    if (is_ok()) {
      return value_cref_() == *cmp.value();
    } else {
      return false;
    }
  }

  [[nodiscard]] constexpr bool operator==(Err<E> const& cmp) const
      requires equality_comparable<E> {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == cmp.value();
    }
  }

  [[nodiscard]] constexpr bool operator==(Err<ConstRef<E>> const& cmp) const
      requires equality_comparable<E> {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == cmp.value();
    }
  }

  [[nodiscard]] constexpr bool operator==(Err<MutRef<E>> const& cmp) const
      requires equality_comparable<E> {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == cmp.value();
    }
  }

  [[nodiscard]] constexpr bool operator==(Err<E const*> const& cmp) const
      requires equality_comparable<E> {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == *cmp.value();
    }
  }

  [[nodiscard]] constexpr bool operator==(Err<E*> const& cmp) const
      requires equality_comparable<E> {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == *cmp.value();
    }
  }

  [[nodiscard]] constexpr bool operator==(Result const& cmp) const
      requires equality_comparable<T>&& equality_comparable<E> {
    if (is_ok() && cmp.is_ok()) {
      return value_cref_() == cmp.value_cref_();
    } else if (is_err() && cmp.is_err()) {
      return err_cref_() == cmp.err_cref_();
    } else {
      return false;
    }
  }

  /// Returns `true` if the result is an `Ok<T>` variant.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> x = Ok(-3);
  /// ASSERT_TRUE(x.is_ok());
  ///
  /// Result<int, string_view> y = Err("Some error message"sv);
  /// ASSERT_FALSE(y.is_ok());
  /// ```
  [[nodiscard]] constexpr bool is_ok() const noexcept { return is_ok_; }

  /// Returns `true` if the result is `Err<T>`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> x = Ok(-3);
  /// ASSERT_FALSE(x.is_err());
  ///
  /// Result<int, string_view> y = Err("Some error message"sv);
  /// ASSERT_TRUE(y.is_err());
  /// ```
  [[nodiscard]] constexpr bool is_err() const noexcept { return !is_ok(); }

  /// Returns `true` if the result is an `Ok<T>` variant and contains the given
  /// value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  ///
  /// Result<int, string> x = Ok(2);
  /// ASSERT_TRUE(x.contains(2));
  ///
  /// Result<int, string> y = Ok(3);
  /// ASSERT_FALSE(y.contains(2));
  ///
  /// Result<int, string> z = Err("Some error message"s);
  /// ASSERT_FALSE(z.contains(2));
  /// ```
  template <typename CmpType>
  requires equality_comparable<T const&, CmpType const&>  //
      [[nodiscard]] constexpr bool contains(CmpType const& cmp) const {
    if (is_ok()) {
      return value_cref_() == cmp;
    } else {
      return false;
    }
  }

  /// Returns `true` if the result is an `Err<E>` variant containing the given
  /// value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  ///
  /// Result<int, string> x = Ok(2);
  /// ASSERT_FALSE(x.contains_err("Some error message"s));
  ///
  /// Result<int, string> y = Err("Some error message"s);
  /// ASSERT_TRUE(y.contains_err("Some error message"s));
  ///
  /// Result<int, string> z = Err("Some other error message"s);
  /// ASSERT_FALSE(z.contains_err("Some error message"s));
  /// ```
  template <typename ErrCmp>
  requires equality_comparable<E const&, ErrCmp const&>  //
      [[nodiscard]] constexpr bool contains_err(ErrCmp const& cmp) const {
    if (is_ok()) {
      return false;
    } else {
      return err_cref_() == cmp;
    }
  }

  /// Returns the value of evaluating the `predicate` on the contained value if
  /// the `Option` is a `Some`, else returns `false`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string> x = Ok(2);
  /// auto even = [](auto x) { return x == 2; };
  ///
  /// ASSERT_TRUE(x.exists(even));
  ///
  /// ```
  template <typename UnaryPredicate>
  requires invocable<UnaryPredicate&&, T const&>&&
      convertible_to<invoke_result<UnaryPredicate&&, T const&>,
                     bool>  //
      [[nodiscard]] constexpr bool exists(UnaryPredicate&& predicate) const {
    if (is_ok()) {
      return std::forward<UnaryPredicate&&>(predicate)(value_cref_());
    } else {
      return false;
    }
  }

  /// Returns the value of evaluating the `predicate` on the contained value if
  /// the `Option` is a `Some`, else returns `false`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string> x = Err("invalid"s);
  /// auto invalid = [](auto x) { return x == "invalid"; };
  ///
  /// ASSERT_TRUE(x.err_exists(invalid));
  ///
  /// ```
  template <typename UnaryPredicate>
  requires invocable<UnaryPredicate&&, E const&>&&
      convertible_to<invoke_result<UnaryPredicate&&, E const&>,
                     bool>  //
      [[nodiscard]] constexpr bool err_exists(
          UnaryPredicate&& predicate) const {
    if (is_err()) {
      return std::forward<UnaryPredicate&&>(predicate)(err_cref_());
    } else {
      return false;
    }
  }

  /// Returns an l-value reference to the contained value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Err`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto result = make_ok<int, int>(6);
  /// int& value = result.value();
  /// value = 97;
  ///
  /// ASSERT_EQ(result, Ok(97));
  /// ```
  [[nodiscard]] T& value() & noexcept {
    if (is_err()) internal::result::no_lref(err_cref_());
    return value_ref_();
  }

  /// Returns an l-value reference to the contained value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Err`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto const result = make_ok<int, int>(6);
  /// int const& value = result.value();
  ///
  /// ASSERT_EQ(value, 6);
  /// ```
  [[nodiscard]] T const& value() const& noexcept {
    if (is_err()) internal::result::no_lref(err_cref_());
    return value_cref_();
  }

  /// Use `unwrap()` instead
  [[deprecated("Use `unwrap()` instead")]] T value() && = delete;
  /// Use `unwrap()` instead
  [[deprecated("Use `unwrap()` instead")]] T const value() const&& = delete;

  /// Returns an l-value reference to the contained error value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Ok`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto result = make_err<int, int>(9);
  /// int& err = result.err_value();
  /// err = 46;
  ///
  /// ASSERT_EQ(result, Err(46));
  /// ```
  [[nodiscard]] E& err_value() & noexcept {
    if (is_ok_) internal::result::no_err_lref(value_cref_());
    return err_ref_();
  }

  /// Returns a const l-value reference to the contained error value.
  /// Note that no copying occurs here.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Ok`
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto const result = make_err<int, int>(9);
  /// int const& err = result.err_value();
  ///
  /// ASSERT_EQ(err, 9);
  /// ```
  [[nodiscard]] E const& err_value() const& noexcept {
    if (is_ok_) internal::result::no_err_lref(value_cref_());
    return err_cref_();
  }

  /// Use `unwrap_err()` instead
  [[deprecated("Use `unwrap_err()` instead")]] E err_value() && = delete;
  /// Use `unwrap_err()` instead
  [[deprecated("Use `unwrap_err()` instead")]] E const err_value() const&& =
      delete;

  /// Converts from `Result<T, E>` to `Option<T>`.
  ///
  /// Converts this result into an `Option<T>`, consuming itself,
  /// and discarding the error, if any.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string> x = Ok(2);
  /// ASSERT_EQ(move(x).ok(), Some(2));
  ///
  /// Result<int, string> y = Err("Nothing here"s);
  /// ASSERT_EQ(move(y).ok(), None);
  /// ```
  [[nodiscard]] constexpr auto ok() && -> Option<T> {
    if (is_ok()) {
      return Some<T>(std::move(value_ref_()));
    } else {
      return None;
    }
  }

  /// Converts from `Result<T, E>` to `Option<E>`.
  ///
  /// Converts this result into an `Option<E>`, consuming itself, and discarding
  /// the success value, if any.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string> x = Ok(2);
  /// ASSERT_EQ(move(x).err(), None);
  ///
  /// Result<int, string> y = Err("Nothing here"s);
  /// ASSERT_EQ(move(y).err(), Some("Nothing here"s));
  /// ```
  [[nodiscard]] constexpr auto err() && -> Option<E> {
    if (is_ok()) {
      return None;
    } else {
      return Some<E>(std::move(err_ref_()));
    }
  }

  /// Converts from `Result<T, E> &` to `Result<ConstRef<T>, ConstRef<E>>`.
  ///
  /// Produces a new `Result`, containing an immutable reference
  /// into the original, leaving the original in place.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string> x = Ok(2);
  /// ASSERT_EQ(x.as_cref().unwrap().get(), 2);
  ///
  /// Result<int, string> y = Err("Error"s);
  /// ASSERT_EQ(y.as_cref().unwrap_err().get(), "Error"s);
  /// ```
  [[nodiscard]] constexpr auto as_cref() const& noexcept
      -> Result<ConstRef<T>, ConstRef<E>> {
    if (is_ok()) {
      return Ok<ConstRef<T>>(ConstRef<T>(value_cref_()));
    } else {
      return Err<ConstRef<E>>(ConstRef<E>(err_cref_()));
    }
  }

  [[deprecated(
      "calling Result::as_cref() on an r-value, and "
      "therefore binding an l-value reference to an object that is marked to "
      "be moved")]]  //
  [[nodiscard]] constexpr auto
  as_cref() const&& noexcept -> Result<ConstRef<T>, ConstRef<E>> = delete;

  /// Converts from `Result<T, E> &` to `Result<MutRef<T>, MutRef<E>>`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto mutate = [](Result<int, int>& r) {
  ///  r.as_ref().match([](auto ok) { ok.get() = 42; },
  ///                       [](auto err) { err.get() = 0; });
  /// };
  ///
  /// Result<int, int> x = Ok(2);
  /// mutate(x);
  /// ASSERT_EQ(x, Ok(42));
  ///
  /// Result<int, int> y = Err(13);
  /// mutate(y);
  /// ASSERT_EQ(y, Err(0));
  /// ```
  [[nodiscard]] constexpr auto as_ref() & noexcept
      -> Result<MutRef<T>, MutRef<E>> {
    if (is_ok()) {
      return Ok<MutRef<T>>(MutRef<T>(value_ref_()));
    } else {
      return Err<MutRef<E>>(MutRef<E>(err_ref_()));
    }
  }

  [[deprecated(
      "calling Result::as_ref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
  [[nodiscard]] constexpr auto
  as_ref() && noexcept -> Result<MutRef<T>, MutRef<E>> = delete;

  /// Maps a `Result<T, E>` to `Result<U, E>` by applying the function `op` to
  /// the contained `Ok<T>` value, leaving an `Err<E>` value untouched.
  ///
  /// This function can be used to compose the results of two functions.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// Extract the content-type from an http header
  ///
  /// ``` cpp
  /// enum class Error { InvalidHeader };
  /// auto header = "Content-Type: multipart/form-data"sv;
  ///
  /// auto check_header = [](string_view s) -> Result<string_view, Error> {
  ///  if (!s.starts_with("Content-Type: "sv)) return Err(Error::InvalidHeader);
  ///  return Ok(move(s));
  /// };
  ///
  /// auto content_type =
  /// check_header(header).map([](auto s) { return s.substr(14); });
  ///
  /// ASSERT_EQ(content_type, Ok("multipart/form-data"sv));
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto map(
          Fn&& op) && -> Result<invoke_result<Fn&&, T&&>, E> {
    if (is_ok()) {
      return Ok<invoke_result<Fn&&, T&&>>(
          std::forward<Fn&&>(op)(std::move(value_ref_())));
    } else {
      return Err<E>(std::move(err_ref_()));
    }
  }

  /// Applies a function to the contained value (if any),
  /// or returns the provided default (if not).
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<string, int> x = Ok("foo"s);
  /// auto map_fn = [](auto s) { return s.size(); };
  /// ASSERT_EQ(move(x).map_or(map_fn, 42UL), 3UL);
  ///
  /// Result<string, int> y = Err(-404);
  /// ASSERT_EQ(move(y).map_or(map_fn, 42UL), 42UL);
  /// ```
  template <typename Fn, typename AltType>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto map_or(
          Fn&& op, AltType&& alt) && -> invoke_result<Fn&&, T&&> {
    if (is_ok()) {
      return std::forward<Fn&&>(op)(std::move(value_ref_()));
    } else {
      return std::forward<AltType&&>(alt);
    }
  }

  /// Maps a `Result<T, E>` to `U` by applying a function to a contained `Ok`
  /// value, or a fallback function to a contained `Err` value.
  ///
  /// This function can be used to unpack a successful result
  /// while handling an error.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// size_t const k = 21;
  ///
  /// Result<string_view, size_t> x = Ok("foo"sv);
  /// auto map_fn = [](auto s) { return s.size(); };
  /// auto else_fn = [&](auto) { return k * 2UL; };
  ///
  /// ASSERT_EQ(move(x).map_or_else(map_fn, else_fn), 3);
  ///
  /// Result<string_view, size_t> y = Err(404UL);
  /// ASSERT_EQ(move(y).map_or_else(map_fn, else_fn), 42);
  /// ```
  template <typename Fn, typename A>
  requires invocable<Fn&&, T&&>&& invocable<A&&, E&&>  //
      [[nodiscard]] constexpr auto map_or_else(
          Fn&& op, A&& alt_op) && -> invoke_result<Fn&&, T&&> {
    if (is_ok()) {
      return std::forward<Fn&&>(op)(std::move(value_ref_()));
    } else {
      return std::forward<A&&>(alt_op)(std::move(err_ref_()));
    }
  }

  /// Maps a `Result<T, E>` to `Result<T, F>` by applying a function to a
  /// contained `Err` value, leaving an `Ok` value untouched.
  ///
  /// This function can be used to pass through a successful result while
  /// handling an error.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto stringify = [](auto x) { return "error code: " + std::to_string(x);
  /// };
  ///
  /// Result<int, int> x = Ok(2);
  /// ASSERT_EQ(move(x).map_err(stringify), Ok(2));
  ///
  /// Result<int, int> y = Err(404);
  /// ASSERT_EQ(move(y).map_err(stringify), Err("error code: 404"s));
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, E&&>  //
      [[nodiscard]] constexpr auto map_err(
          Fn&& op) && -> Result<T, invoke_result<Fn&&, E&&>> {
    if (is_ok()) {
      return Ok<T>(std::move(value_ref_()));
    } else {
      return Err<invoke_result<Fn&&, E&&>>(
          std::forward<Fn&&>(op)(std::move(err_ref_())));
    }
  }

  /// Returns `res` if the result is `Ok`, otherwise returns the `Err` value
  /// of itself.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> a = Ok(2);
  /// Result<string_view, string_view> b = Err("late error"sv);
  /// ASSERT_EQ(move(a).AND(move(b)), Err("late error"sv));
  ///
  /// Result<int, string_view> c = Err("early error"sv);
  /// Result<string_view, string_view> d = Ok("foo"sv);
  /// ASSERT_EQ(move(c).AND(move(d)), Err("early error"sv));
  ///
  /// Result<int, string_view> e = Err("not a 2"sv);
  /// Result<string_view, string_view> f = Err("late error"sv);
  /// ASSERT_EQ(move(e).AND(move(f)), Err("not a 2"sv));
  ///
  /// Result<int, string_view> g = Ok(2);
  /// Result<string_view, string_view> h = Ok("different result type"sv);
  /// ASSERT_EQ(move(g).AND(move(h)), Ok("different result type"sv));
  /// ```
  // a copy attempt like passing a const could cause an error
  template <typename U, typename F>
  requires convertible_to<E, F>  //
      [[nodiscard]] constexpr auto AND(Result<U, F>&& res) && -> Result<U, F> {
    if (is_ok()) {
      return std::forward<Result<U, F>&&>(res);
    } else {
      return Err<F>(E(std::move(err_ref_())));
    }
  }

  /// Calls `op` if the result is `Ok`, otherwise returns the `Err` value of
  /// itself.
  ///
  /// This function can be used for control flow based on `Result` values.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto sq = [](int x) { return x * x; };
  ///
  /// auto make_ok = [](int x) -> Result<int, int> { return Ok(move(x)); };
  /// auto make_err = [](int x) -> Result<int, int> { return Err(move(x)); };
  ///
  /// ASSERT_EQ(make_ok(2).and_then(sq).and_then(sq), Ok(16));
  /// ASSERT_EQ(make_err(3).and_then(sq).and_then(sq), Err(3));
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, T&&>  //
      [[nodiscard]] constexpr auto and_then(
          Fn&& op) && -> Result<invoke_result<Fn&&, T&&>, E> {
    if (is_ok()) {
      return Ok<invoke_result<Fn&&, T&&>>(
          std::forward<Fn&&>(op)(std::move(value_ref_())));
    } else {
      return Err<E>(std::move(err_ref_()));
    }
  }

  /// Returns `res` if the result is `Err`, otherwise returns the `Ok` value of
  /// itself.
  ///
  /// Arguments passed to `or` are eagerly evaluated; if you are passing the
  /// result of a function call, it is recommended to use `or_else`, which is
  /// lazily evaluated.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> a = Ok(2);
  /// Result<int, string_view> b = Err("late error"sv);
  /// ASSERT_EQ(move(a).OR(move(b)), Ok(2));
  ///
  /// Result<int, string_view> c = Err("early error"sv);
  /// Result<int, string_view> d = Ok(2);
  /// ASSERT_EQ(move(c).OR(move(d)), Ok(2));
  ///
  /// Result<int, string_view> e = Err("not a 2"sv);
  /// Result<int, string_view> f = Err("late error"sv);
  /// ASSERT_EQ(move(e).OR(move(f)), Err("late error"sv));
  ///
  /// Result<int, string_view> g = Ok(2);
  /// Result<int, string_view> h = Ok(100);
  /// ASSERT_EQ(move(g).OR(move(h)), Ok(2));
  /// ```
  // passing a const ref will cause an error
  template <typename U, typename F>
  requires convertible_to<T&&, U>  //
      [[nodiscard]] constexpr auto OR(Result<U, F>&& alt) && -> Result<U, F> {
    if (is_ok()) {
      return Ok<U>(static_cast<U>(std::move(value_ref_())));
    } else {
      return std::forward<Result<U, F>&&>(alt);
    }
  }

  /// Calls `op` if the result is `Err`, otherwise returns the `Ok` value of
  /// itself.
  ///
  /// This function can be used for control flow based on result values.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto make_ok = [](int x) -> Result<int, int> { return Ok(move(x)); };
  /// auto make_err = [](int x) -> Result<int, int> { return Err(move(x)); };
  /// auto sq = [](int err) -> Result<int, int> { return Ok(err * err); };
  /// auto err = [](int err) -> Result<int, int> { return Err(move(err)); };
  ///
  /// ASSERT_EQ(make_ok(2).or_else(sq).or_else(sq), Ok(2));
  /// ASSERT_EQ(make_ok(2).or_else(err).or_else(sq), Ok(2));
  /// ASSERT_EQ(make_err(3).or_else(sq).or_else(err), Ok(9));
  /// ASSERT_EQ(make_err(3).or_else(err).or_else(err), Err(3));
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, E&&>  //
      [[nodiscard]] constexpr auto or_else(
          Fn&& op) && -> invoke_result<Fn&&, E&&> {
    if (is_ok()) {
      return Ok<T>(std::move(value_ref_()));
    } else {
      return std::forward<Fn&&>(op)(std::move(err_ref_()));
    }
  }

  /// Unwraps a result, yielding the content of an `Ok<T>` variant.
  /// Else, it returns the parameter `alt`.
  ///
  /// Arguments passed to `unwrap_or` are eagerly evaluated; if you are passing
  /// the result of a function call, it is recommended to use
  /// `unwrap_or_else`, which is lazily evaluated.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// int alt = 2;
  /// Result<int, string_view> x = Ok(9);
  /// ASSERT_EQ(move(x).unwrap_or(move(alt)), 9);
  ///
  /// int alt_b = 2;
  /// Result<int, string_view> y = Err("error"sv);
  /// ASSERT_EQ(move(y).unwrap_or(move(alt_b)), 2);
  /// ```
  [[nodiscard]] constexpr auto unwrap_or(T&& alt) && -> T {
    if (is_ok()) {
      return std::move(value_ref_());
    } else {
      return std::move(alt);
    }
  }

  /// Unwraps a result, yielding the content of an `Ok`.
  /// If the value is an `Err` then it calls `op` with its value.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto count = [] (string_view err)  { return err.size(); };
  ///
  /// ASSERT_EQ(make_ok<size_t,string_view>(2UL).unwrap_or_else(count), 2);
  /// ASSERT_EQ(make_err<size_t,string_view>("booo"sv).unwrap_or_else(count),
  /// 4);
  /// ```
  template <typename Fn>
  requires invocable<Fn&&, E&&>  //
      [[nodiscard]] constexpr auto unwrap_or_else(Fn&& op) && -> T {
    if (is_ok()) {
      return std::move(value_ref_());
    } else {
      return std::forward<Fn&&>(op)(std::move(err_ref_()));
    }
  }

  /// Unwraps a result, yielding the content of an `Ok`.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Err`, with a panic message provided by the
  /// `Err`'s value.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// ASSERT_EQ(make_ok<int, string_view>(2).unwrap(), 2);
  /// Result<int, string_view> x = Err("emergency failure"sv);
  /// ASSERT_ANY_THROW(move(x).unwrap());
  /// ```
  [[nodiscard]] auto unwrap() && -> T {
    if (is_err()) {
      internal::result::no_value(err_cref_());
    }
    return std::move(value_ref_());
  }

  /// Unwraps a result, yielding the content of an `Ok`.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Err`, with a panic message including the
  /// passed message, and the content of the `Err`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> x = Err("emergency failure"sv);
  /// ASSERT_ANY_THROW(move(x).expect("Testing expect"));
  /// ```
  [[nodiscard]] auto expect(std::string_view msg) && -> T {
    if (is_err()) {
      internal::result::expect_value_failed(std::move(msg), err_cref_());
    }
    return std::move(value_ref_());
  }

  /// Unwraps a result, yielding the content of an `Err`.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Ok`, with a custom panic message provided
  /// by the `Ok`'s value.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> x = Ok(2);
  /// ASSERT_ANY_THROW(move(x).unwrap_err()); // panics
  ///
  /// Result<int, string_view> y = Err("emergency failure"sv);
  /// ASSERT_EQ(move(y).unwrap_err(), "emergency failure");
  /// ```
  [[nodiscard]] auto unwrap_err() && -> E {
    if (is_ok()) {
      internal::result::no_err(value_cref_());
    }
    return std::move(err_ref_());
  }

  /// Unwraps a result, yielding the content of an `Err`.
  ///
  /// # Panics
  ///
  /// Panics if the value is an `Ok`, with a panic message including the
  /// passed message, and the content of the `Ok`.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view> x = Ok(10);
  /// ASSERT_ANY_THROW(move(x).expect_err("Testing expect_err")); // panics with
  ///                                                             // "Testing
  ///                                                             // expect_err:
  ///                                                             // 10"
  /// ```
  [[nodiscard]] auto expect_err(std::string_view msg) && -> E {
    if (is_ok()) {
      internal::result::expect_err_failed(std::move(msg), value_cref_());
    }
    return std::move(err_ref_());
  }

  /// Returns the contained value or a default
  ///
  /// Consumes itself then, if `Ok`, returns the contained
  /// value, otherwise if `Err`, returns the default value for that
  /// type.
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<string, int> good_year = Ok("1909"s);
  /// Result<string, int> bad_year = Err(-1);
  ///
  /// ASSERT_EQ(move(good_year).unwrap_or_default(), "1909"s);
  /// ASSERT_EQ(move(bad_year).unwrap_or_default(), ""s); // empty string (""s)
  ///                                                     // is the default
  ///                                                     // value
  ///                                                     // for a C++ string
  /// ```
  [[nodiscard]] constexpr auto unwrap_or_default() && -> T
      requires default_constructible<T> {
    if (is_ok()) {
      return std::move(value_ref_());
    } else {
      return T();
    }
  }

  /// Performs a constant dereference on `T`. if `T` is a pointer, C++-style
  /// iterator or smart-pointer. i.e. Converts `Result<T, E>` to
  /// `Result<ConstRef<U>, ConstRef<E>>`
  /// Where:
  /// -  `U` represents the value obtained from dereferencing `T` and
  /// `ConstRef<U>` is a constant reference to a value pointed to by the value's
  /// pointer-type `T`
  ///
  ///
  /// # NOTE
  /// `ConstRef<U>` is an alias for `std::reference_wrapper<U const>`, but
  /// that's too long :)
  /// # NOTE
  /// If `T` is an owning pointer/iterator/object, This result
  /// should live just as long as the dereference result obtained by calling
  /// this method.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// int v = 98;
  ///
  /// Result<int*, string_view> x = Ok(&v);
  /// ConstRef<int> v_ref = x.as_const_deref().unwrap();
  ///
  /// ASSERT_EQ(v_ref.get(), 98);    // check the values
  /// ASSERT_EQ(&v_ref.get(), &v);  // check their addresses
  ///
  /// Result<int*, string_view> y = Err("Errrr...."sv);
  /// ConstRef<string_view> y_err_ref = y.as_const_deref().unwrap_err();
  /// ASSERT_EQ(y_err_ref.get(), "Errrr...."sv);  // check the string-view's
  ///                                             // value
  /// ```
  [[nodiscard]] constexpr auto as_const_deref()
      const& requires ConstDerefable<T> {
    using result_t = Result<ConstDeref<T>, ConstRef<E>>;

    if (is_ok()) {
      return result_t(Ok<ConstDeref<T>>(ConstDeref<T>(*value_cref_())));
    } else {
      return result_t(Err<ConstRef<E>>(ConstRef<E>(err_cref_())));
    }
  }

  [[deprecated(
      "calling Result::as_const_deref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
  [[nodiscard]] constexpr auto
  as_const_deref() const&& requires ConstDerefable<T> = delete;

  /// Performs a constant dereference on `E`. if `E` is a pointer, C++-style
  /// iterator or smart-pointer. i.e. Converts `Result<T, E>` to
  /// `Result<ConstRef<T>, ConstRef<F>>`
  /// Where:
  /// -  `F` represents the value obtained from dereferencing `E` and
  /// `ConstRef<F>` is a constant reference to a value pointed to by the error's
  /// pointer-type `E`
  ///
  ///
  /// # NOTE
  ///
  /// `ConstRef<F>` is an alias for `std::reference_wrapper<F const>`, but
  /// that's too long :)
  ///
  /// # NOTE
  ///
  /// If `E` is an owning pointer/iterator/object, This result
  /// should live just as long as the dereference result obtained by calling
  /// this method.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// Result<int, string_view*> x = Ok(2);
  /// ConstRef<int> x_value_ref = x.as_const_deref_err().unwrap();
  /// ASSERT_EQ(x_value_ref.get(), 2);  // check their values
  ///
  /// string_view e = "Errrr...."sv;
  ///
  /// Result<int, string_view*> y = Err(&e);
  /// ConstRef<string_view> y_err_ref = y.as_const_deref_err().unwrap_err();
  ///
  /// ASSERT_EQ(y_err_ref.get(), e);    // check their values
  /// ASSERT_EQ(&y_err_ref.get(), &e);  // check their addresses
  /// ```
  [[nodiscard]] constexpr auto as_const_deref_err()
      const& requires ConstDerefable<E> {
    using result_t = Result<ConstRef<T>, ConstDeref<E>>;

    if (is_ok()) {
      return result_t(Ok<ConstRef<T>>(ConstRef<T>(value_cref_())));
    } else {
      return result_t(Err<ConstDeref<E>>(ConstDeref<E>(*err_cref_())));
    }
  }

  [[deprecated(
      "calling Result::as_const_deref_err() on an r-value, and therefore "
      "binding an l-value reference to an object that is marked to be "
      "moved")]]  //
  [[nodiscard]] constexpr auto
  as_const_deref_err() const&& requires ConstDerefable<E> = delete;

  /// Performs a mutable dereference on `T`, if `T` is a pointer, C++-style
  /// iterator or smart-pointer. i.e. Converts `Result<T, E>` to
  /// `Result<MutRef<U>, MutRef<E>>`
  /// Where:
  /// -  `U` represents the value obtained from dereferencing `T` and
  /// `MutRef<U>` is a mutable reference to a value pointed to by the value's
  /// pointer-type `T`
  ///
  ///
  /// # NOTE
  ///
  /// `MutRef<U>` is an alias for std::reference_wrapper<U>, but
  /// that's too long :)
  /// # NOTE
  ///
  /// If `T` is an owning pointer/iterator/object, This result
  /// should live just as long as the dereference result obtained by calling
  /// this method.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// int v = 98;
  ///
  /// Result<int*, string_view> x = Ok(&v);
  /// MutRef<int> v_ref = x.as_mut_deref().unwrap();
  ///
  /// ASSERT_EQ(v_ref.get(), 98);   // check the values
  /// ASSERT_EQ(&v_ref.get(), &v);  // check their addresses
  ///
  /// v_ref.get() = -404;  // change v's value via the mutable reference
  ///
  /// ASSERT_EQ(v_ref.get(), -404);  // check that the reference's value changed
  /// ASSERT_EQ(v, -404);            // check that v's value changed
  /// ASSERT_EQ(v_ref.get(), v);     // check that both v and v_ref are equal
  /// ASSERT_EQ(&v_ref.get(), &v);   // check that v_ref references v
  ///
  /// Result<int*, string_view> y = Err("Errrr...."sv);
  /// MutRef<string_view> y_err_ref = y.as_mut_deref().unwrap_err();
  /// ASSERT_EQ(y_err_ref.get(), "Errrr...."sv);
  ///
  /// y_err_ref.get() = "Omoshiroi!..."sv;  // change the error's value
  ///
  /// ASSERT_EQ(y, Err("Omoshiroi!..."sv));  // check that the error's value was
  ///                                        // actually changed
  /// ```
  [[nodiscard]] constexpr auto as_mut_deref() & requires MutDerefable<T> {
    using result_t = Result<MutDeref<T>, MutRef<E>>;
    if (is_ok()) {
      return result_t(Ok<MutDeref<T>>(MutDeref<T>(*value_ref_())));
    } else {
      return result_t(Err<MutRef<E>>(MutRef<E>(err_ref_())));
    }
  }

  [[deprecated(
      "calling Result::as_mut_deref() on an r-value, and therefore binding a "
      "reference to an object that is marked to be moved")]]  //
      [[nodiscard]] constexpr auto
      as_mut_deref() &&
      requires MutDerefable<T> = delete;

  /// Performs a mutable dereference on `E`, if `E` is a pointer, C++-style
  /// iterator or smart-pointer. i.e. Converts `Result<T, E>` to
  /// `Result<MutRef<T>, MutRef<F>>`
  /// Where:
  /// -  `F` represents the value obtained from dereferencing `E` and
  /// `MutRef<F>` is a mutable reference to a value pointed to by the value's
  /// pointer-type `T`
  ///
  ///
  /// # NOTE
  ///
  /// `MutRef<F>` is an alias for std::reference_wrapper<F>, but
  /// that's too long :)
  ///
  /// # NOTE
  ///
  /// If `E` is an owning pointer/iterator/object, This result
  /// should live just as long as the dereference result obtained by calling
  /// this method.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// int e = 98;
  ///
  /// Result<string_view, int*> x = Err(&e);
  /// MutRef<int> e_ref = x.as_mut_deref_err().unwrap_err();
  ///
  /// ASSERT_EQ(e_ref.get(), 98);   // check the values
  /// ASSERT_EQ(&e_ref.get(), &e);  // check their addresses
  ///
  /// e_ref.get() = -404;  // change e's value via the mutable reference
  ///
  /// ASSERT_EQ(e_ref.get(), -404);  // check that the reference's value changed
  /// ASSERT_EQ(e, -404);            // check that v's value changed
  /// ASSERT_EQ(e_ref.get(), e);     // check that both v and v_ref are equal
  /// ASSERT_EQ(&e_ref.get(), &e);   // check that v_ref references v
  ///
  /// Result<string_view, int*> y = Ok("Errrr...."sv);
  /// MutRef<string_view> y_err_ref = y.as_mut_deref_err().unwrap();
  /// ASSERT_EQ(y_err_ref.get(), "Errrr...."sv);
  ///
  /// y_err_ref.get() = "Omoshiroi!..."sv;  // change the error's value
  ///
  /// ASSERT_EQ(y, Ok("Omoshiroi!..."sv));  // check that the error's value was
  ///                                       // actually changed
  /// ```
  [[nodiscard]] constexpr auto as_mut_deref_err() & requires MutDerefable<E> {
    using result_t = Result<MutRef<T>, MutDeref<E>>;
    if (is_ok()) {
      return result_t(Ok<MutRef<T>>(MutRef<T>(value_ref_())));
    } else {
      return result_t(Err<MutDeref<E>>(MutDeref<E>(*err_ref_())));
    }
  }

  [[deprecated(
      "calling Result::as_mut_deref_err() on an r-value, and therefore binding "
      "an l-value reference to an object that is marked to be moved")]]  //
      [[nodiscard]] constexpr auto
      as_mut_deref_err() &&
      requires MutDerefable<E> = delete;

  /// Calls the parameter `ok_fn` with the value if this result is an `Ok<T>`,
  /// else calls `err_fn` with the error. This result is consumed afterward.
  ///
  /// The return type of both parameters must be same (without expecting
  /// implicit conversions). They can also both return nothing `void`.
  ///
  ///
  /// # Examples
  ///
  /// Basic usage:
  ///
  /// ``` cpp
  /// auto i = make_ok<int, string_view>(99);
  ///
  /// auto j = move(i).match([](int value) { return value; },
  ///                        [](string_view) { return -1; });
  /// ASSERT_EQ(j, 99);
  ///
  ///
  /// auto x = make_err<int, string_view>("404 Not Found"sv);
  /// // you can return nothing
  /// move(x).match([](int) {  },
  ///               [](string_view s) { fmt::print("Error: {}\n", s); });
  /// ```
  template <typename OkFn, typename ErrFn>
  requires invocable<OkFn&&, T&&>&& invocable<ErrFn&&, E&&>  //
      [[nodiscard]] constexpr auto match(
          OkFn&& ok_fn, ErrFn&& err_fn) && -> invoke_result<OkFn&&, T&&> {
    if (is_ok()) {
      return std::forward<OkFn&&>(ok_fn)(std::move(value_ref_()));
    } else {
      return std::forward<ErrFn&&>(err_fn)(std::move(err_ref_()));
    }
  }

  [[nodiscard]] constexpr auto clone() const
      -> Result<T, E> requires copy_constructible<T>&& copy_constructible<E> {
    if (is_ok()) {
      return Ok<T>(T(value_cref_()));
    } else {
      return Err<E>(E(err_cref_()));
    }
  }

 private:
  bool is_ok_;
  union {
    T storage_value_;
    E storage_err_;
  };

  [[nodiscard]] constexpr T& value_ref_() noexcept { return storage_value_; }

  [[nodiscard]] constexpr T const& value_cref_() const noexcept {
    return storage_value_;
  }

  [[nodiscard]] constexpr E& err_ref_() noexcept { return storage_err_; }

  [[nodiscard]] constexpr E const& err_cref_() const noexcept {
    return storage_err_;
  }
};

/// Helper function to construct an `Option<T>` with a `Some<T>` value.
/// if the template parameter is not specified, it is auto-deduced from the
/// parameter's value.
///
/// # Examples
///
/// Basic usage:
///
///
/// ``` cpp
/// // these are some of the various ways to construct on Option<T> with a
/// // Some<T> value
/// Option g = Some(9);
/// Option h = Some<int>(9);
/// Option<int> i = Some(9);
/// auto j = make_some(9);
/// auto k = Option<int>(Some<int>(9));
/// auto l = Option<int>(Some(9));
/// // ... and a few more
///
/// // to make it easier and less verbose:
/// auto m = make_some(9);
/// ASSERT_EQ(m, Some(9));
///
/// auto n = make_some<int>(9);
/// ASSERT_EQ(m, Some(9));
///
/// // observe that m is constructed as an Option<int> (=Option<T>) and T (=int)
/// // is auto-deduced from make_some's parameter type.
/// ```
template <typename T>
[[nodiscard]] STX_FORCE_INLINE constexpr auto make_some(T value) -> Option<T> {
  return Some<T>(std::forward<T>(value));
}

/// Helper function to construct an `Option<T>` with a `None` value.
/// note that the value parameter `T` must be specified.
///
/// # Examples
///
/// Basic usage:
///
///
/// ``` cpp
/// // these are some of the various ways to construct on Option<T> with
/// // a None value
/// Option<int> h = None;
/// auto i = make_none<int>();
/// Option j = make_none<int>();
/// Option<int> k = make_none<int>();
///
/// // to make it easier and less verbose:
/// auto m = make_none<int>();
/// ASSERT_EQ(m, None);
///
/// // observe that m is constructed as an Option<int> (=Option<T>) and T(=int).
/// ```
template <typename T>
[[nodiscard]] STX_FORCE_INLINE constexpr auto make_none() -> Option<T> {
  return None;
}

/// Helper function to construct a `Result<T, E>` with an `Ok<T>` value.
///
/// # NOTE
///
/// The error type `E` must be specified and is the first template
/// parameter.
///
/// # Examples
///
/// Basic usage:
///
///
/// ``` cpp
/// // these are some of the various ways to construct on Result<T, E> with an
/// // Ok<T> value
/// Result<int, string> a = Ok(8);
/// Result<int, string> b = Ok<int>(8);
///
/// // to make it easier and less verbose:
/// auto c = make_ok<string, int>(9);
/// ASSERT_EQ(c, Ok(9));
///
/// auto d = make_ok<string, int>(9);
/// ASSERT_EQ(d, Ok(9));
///
/// // observe that c is constructed as Result<int, string>
/// // (=Result<T, E>).
/// ```
template <typename T, typename E>
[[nodiscard]] STX_FORCE_INLINE constexpr auto make_ok(T value) -> Result<T, E> {
  return Ok<T>(std::forward<T>(value));
}

/// Helper function to construct a `Result<T, E>` with an `Err<E>` value.
/// if the template parameter `E` is not specified, it is auto-deduced from the
/// parameter's value.
///
/// # NOTE
/// The value type `T` must be specified and is the first template
/// parameter.
///
/// # Examples
///
/// Basic usage:
///
///
/// ``` cpp
///
/// // these are some of the various ways to construct on Result<T, E> with an
/// // Ok<T> value
/// Result<int, string> a = Err("foo"s);
/// Result<int, string> b = Err<string>("foo"s);
///
/// // to make it easier and less verbose:
/// auto c = make_err<int, string>("bar"s);
/// ASSERT_EQ(c, Err("bar"s));
///
/// // observe that c is constructed as Result<int, string>
/// // (=Result<T, E>).
///
/// ```
template <typename T, typename E>
[[nodiscard]] STX_FORCE_INLINE constexpr auto make_err(E err) -> Result<T, E> {
  return Err<E>(std::forward<E>(err));
}

};  // namespace stx

// normal return tries

#define TRY_OK(identifier, result_expr)                                        \
  decltype(result_expr) stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh = (result_expr); \
  if (stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh.is_err())                          \
    return Err<decltype(result_expr)::error_type>(                             \
        std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap_err());        \
  decltype(result_expr)::value_type identifier =                               \
      std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap();

#define TRY_SOME(identifier, option_expr)                                      \
  decltype(option_expr) stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh = (option_expr); \
  if (stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh.is_none()) return stx::None;       \
  decltype(option_expr)::value_type identifier =                               \
      std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap();

// Coroutines

#define CO_TRY_OK(identifier, result_expr)                                     \
  decltype(result_expr) stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh = (result_expr); \
  if (stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh.is_err())                          \
    co_return Err<decltype(result_expr)::error_type>(                          \
        std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap_err());        \
  decltype(result_expr)::value_type identifier =                               \
      std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap();

#define CO_TRY_SOME(identifier, option_expr)                                   \
  decltype(option_expr) stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh = (option_expr); \
  if (stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh.is_none()) co_return stx::None;    \
  decltype(option_expr)::value_type identifier =                               \
      std::move(stx_TmpVaRYoUHopEfUllYwOnTcoLlidEwiTh).unwrap();
