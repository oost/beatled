#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <utility>

using namespace std;

class A {
public:
  // Defining enum1 Gender
  enum ContructorType {
    DefaultConstructor,
    CopyConstructor,
    CopyAssignment,
    MoveConstructor,
    MoveAssignment,
    MovedFrom
  };
  A() {
    cout << "Default constructor" << endl;
    constructorType_ = DefaultConstructor;
    counter_++;
  }
  A(const A &a) {
    cout << "Copy constructor" << endl;
    constructorType_ = CopyConstructor;
    counter_ = a.counter_ + 1;
  }
  A(A &&rhs) {
    cout << "Move constructor" << endl;
    constructorType_ = MoveConstructor;
    counter_ = rhs.counter_ + 1;
    rhs.constructorType_ = MovedFrom;
  }

  A &operator=(A &rhs) {
    cout << "Copy assignment operator" << endl;
    constructorType_ = CopyAssignment;
    counter_ = rhs.counter_ + 1;
    return *this;
  }
  A &operator=(A &&rhs) {
    cout << "Move assignment operator" << endl;
    constructorType_ = MoveAssignment;
    counter_ = rhs.counter_ + 1;
    rhs.constructorType_ = MovedFrom;

    return *this;
  }
  ~A() { cout << "destructor" << endl; }

  int counter() const { return counter_; }
  ContructorType constructorType() const { return constructorType_; }

private:
  ContructorType constructorType_;
  int counter_ = 0;
};

bool test_pass_by_value(A param, A::ContructorType expected_constructor_type,
                        int expect_counter) {
  return (param.counter() == expect_counter) &&
         (param.constructorType() == expected_constructor_type);
}

bool test_pass_by_lvalue_reference(A &param,
                                   A::ContructorType expected_constructor_type,
                                   int expect_counter) {
  return (param.counter() == expect_counter) &&
         (param.constructorType() == expected_constructor_type);
}

bool test_pass_by_const_lvalue_reference(
    const A &param, A::ContructorType expected_constructor_type,
    int expect_counter) {
  return (param.counter() == expect_counter) &&
         (param.constructorType() == expected_constructor_type);
}
bool test_pass_by_rvalue_reference(A &&param,
                                   A::ContructorType expected_constructor_type,
                                   int expect_counter) {
  return (param.counter() == expect_counter) &&
         (param.constructorType() == expected_constructor_type);
}

A test_return_by_value_with_rvo_copy_elision() { return A(); }
A test_return_by_value_with_nrvo_copy_elision() {
  A e;
  return e;
}
A &&test_return_by_rvalue_reference() {
  A e;
  return move(e);
}

TEST_CASE("Move semantics ", "[move]") {
  A a;

  SECTION("Constructors") {
    SECTION("Default Constructor") {
      REQUIRE(a.constructorType() == A::DefaultConstructor);
      REQUIRE(a.counter() == 1);
    }

    SECTION("Copy Constructor") {
      A b{a};
      REQUIRE(b.constructorType() == A::CopyConstructor);
      REQUIRE(b.counter() == 2);
    }

    SECTION("Copy Constructor with Copy Elision") {
      A c = a;
      REQUIRE(c.constructorType() == A::CopyConstructor);
      REQUIRE(c.counter() == 2);
    }

    SECTION("CopyAssignment") {
      A d;
      REQUIRE(d.constructorType() == A::DefaultConstructor);
      REQUIRE(d.counter() == 1);
      d = a; //

      REQUIRE(d.constructorType() == A::CopyAssignment);
      REQUIRE(d.counter() == 2);
    }

    SECTION("Move Constructor") {
      A e{move(a)}; // Move constructor
      REQUIRE(e.constructorType() == A::MoveConstructor);
      REQUIRE(e.counter() == 2);
      REQUIRE(a.constructorType() == A::MovedFrom);
    }

    SECTION("Move Constructor with Copy Elision") {
      A f = move(a); // Move assignment
      REQUIRE(f.constructorType() == A::MoveConstructor);
      REQUIRE(f.counter() == 2);
      REQUIRE(a.constructorType() == A::MovedFrom);
    }

    SECTION("Copy Assignment") {
      A g;
      g = move(a); // Move assignment
      REQUIRE(g.constructorType() == A::MoveAssignment);
      REQUIRE(g.counter() == 2);
      REQUIRE(a.constructorType() == A::MovedFrom);
    }
  }
  SECTION("Function parameters") {
    SECTION("Pass by value") {
      REQUIRE(test_pass_by_value(a, A::CopyConstructor, 2));
    }
    SECTION("Pass by value and move") {
      REQUIRE(test_pass_by_value(move(a), A::MoveConstructor, 2));
      REQUIRE(a.constructorType() == A::MovedFrom);
    }
    SECTION("Pass by lvalue reference") {
      REQUIRE(test_pass_by_lvalue_reference(a, A::DefaultConstructor, 1));
    }
    // SECTION("Pass by lvalue reference") {
    //   REQUIRE(test_pass_by_lvalue_reference(move(a), A::DefaultConstructor,
    //   1));
    // }
    SECTION("Pass by const lvalue reference") {
      REQUIRE(test_pass_by_const_lvalue_reference(a, A::DefaultConstructor, 1));
    }
    // SECTION("Pass by rvalue reference") {
    //   REQUIRE(test_pass_by_rvalue_reference(a, A::DefaultConstructor, 1));
    // }
    SECTION("Pass by rvalue reference") {
      REQUIRE(test_pass_by_rvalue_reference(move(a), A::DefaultConstructor, 1));
      /// ????
      REQUIRE(a.constructorType() == A::DefaultConstructor);
      /// ???
    }
  }

  SECTION("Function return") {
    SECTION("Return by value") {
      A r = test_return_by_value_with_nrvo_copy_elision();
      REQUIRE(r.counter() == 1);
      REQUIRE(r.constructorType() == A::DefaultConstructor);
    }
    SECTION("Return by value with copy elision") {
      A r = test_return_by_value_with_rvo_copy_elision();

      REQUIRE(r.counter() == 1);
      REQUIRE(r.constructorType() == A::DefaultConstructor);
    }
    SECTION("Return by rvalue reference") {
      A r = test_return_by_rvalue_reference();

      REQUIRE(test_pass_by_value(move(a), A::MoveConstructor, 2));
      REQUIRE(a.constructorType() == A::MovedFrom);
    }
  }
}
