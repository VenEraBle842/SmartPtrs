#include <gtest/gtest.h>
#include "SharedPtr.hpp"
#include "UniquePtr.hpp"

// Вспомогательный класс для проверки вызова конструкторов/деструкторов
struct Tracker {
    static int alive_count;
    int value;

    explicit Tracker(int val = 0) : value(val) {
        ++alive_count;
    }

    Tracker(const Tracker& other) : value(other.value) {
        ++alive_count;
    }

    ~Tracker() {
        --alive_count;
    }
};

int Tracker::alive_count = 0;


TEST(UniquePtrTest, DefaultAndNullptrConstructor) {
    UniquePtr<int> p1;
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1.get(), nullptr);

    UniquePtr<int> p2(nullptr);
    EXPECT_FALSE(p2);
    EXPECT_EQ(p2.get(), nullptr);
}

TEST(UniquePtrTest, OwnershipAndDereference) {
    Tracker::alive_count = 0;
    {
        UniquePtr<Tracker> p(new Tracker(42));
        EXPECT_TRUE(p);
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ((*p).value, 42);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
}

TEST(UniquePtrTest, MoveSemantics) {
    Tracker::alive_count = 0;
    {
        UniquePtr<Tracker> p1(new Tracker(10));
        UniquePtr<Tracker> p2(std::move(p1));

        EXPECT_FALSE(p1);
        EXPECT_EQ(p1.get(), nullptr);
        EXPECT_TRUE(p2);
        EXPECT_EQ(p2->value, 10);
        EXPECT_EQ(Tracker::alive_count, 1);

        UniquePtr<Tracker> p3(new Tracker(20));
        EXPECT_EQ(Tracker::alive_count, 2);

        p3 = std::move(p2); // Старый объект p3 должен удалиться
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(p3->value, 10);
        EXPECT_FALSE(p2);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
}

TEST(UniquePtrTest, SelfMoveAssignment) {
    UniquePtr<int> p(new int(100));
    UniquePtr<int>& self_ref = p;
    p = std::move(self_ref); // Проверка ветки if (this != &other)
    EXPECT_TRUE(p);
    EXPECT_EQ(*p, 100);
}

TEST(UniquePtrTest, ReleaseAndReset) {
    Tracker::alive_count = 0;
    {
        UniquePtr<Tracker> p(new Tracker(99));
        Tracker* raw = p.release();

        EXPECT_FALSE(p);
        EXPECT_EQ(Tracker::alive_count, 1); // Объект не должен удалиться
        EXPECT_EQ(raw->value, 99);

        delete raw;
        EXPECT_EQ(Tracker::alive_count, 0);
    }

    {
        UniquePtr<Tracker> p(new Tracker(1));
        p.reset(new Tracker(2)); // Старый объект удаляется
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(p->value, 2);

        p.reset(p.get()); // Сброс на самого себя не должен приводить к UB
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(p->value, 2);

        p.reset(nullptr);
        EXPECT_EQ(Tracker::alive_count, 0);
        EXPECT_FALSE(p);
    }
}

TEST(UniquePtrArrayTest, ArrayOperationsAndDestruction) {
    Tracker::alive_count = 0;
    {
        UniquePtr<Tracker[]> arr(new Tracker[3]{Tracker(10), Tracker(20), Tracker(30)});
        EXPECT_EQ(Tracker::alive_count, 3);
        EXPECT_EQ(arr[0].value, 10);
        EXPECT_EQ(arr[1].value, 20);
        EXPECT_EQ(arr[2].value, 30);

        arr[1].value = 25;
        EXPECT_EQ(arr[1].value, 25);
    }
    EXPECT_EQ(Tracker::alive_count, 0); // Проверка корректного delete[]
}


TEST(SharedPtrTest, DefaultAndNullptrConstructor) {
    SharedPtr<int> sp1;
    EXPECT_FALSE(sp1);
    EXPECT_EQ(sp1.get(), nullptr);
    EXPECT_EQ(sp1.use_cnt(), 0);

    SharedPtr<int> sp2(nullptr);
    EXPECT_FALSE(sp2);
    EXPECT_EQ(sp2.use_cnt(), 0);
}

TEST(SharedPtrTest, BasicSharedOwnership) {
    Tracker::alive_count = 0;
    {
        SharedPtr<Tracker> sp1(new Tracker(55));
        EXPECT_EQ(sp1.use_cnt(), 1);
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(sp1->value, 55);

        {
            SharedPtr<Tracker> sp2 = sp1; // Copy ctor
            EXPECT_EQ(sp1.use_cnt(), 2);
            EXPECT_EQ(sp2.use_cnt(), 2);
            EXPECT_EQ(sp1.get(), sp2.get());
        }
        EXPECT_EQ(sp1.use_cnt(), 1);
        EXPECT_EQ(Tracker::alive_count, 1);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
}

TEST(SharedPtrTest, CopyAssignmentEdgeCases) {
    Tracker::alive_count = 0;
    {
        SharedPtr<Tracker> a(new Tracker(1));
        SharedPtr<Tracker> b(new Tracker(2));
        EXPECT_EQ(Tracker::alive_count, 2);

        a = b; // Объект 1 удаляется, a и b разделяют объект 2
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(a.use_cnt(), 2);
        EXPECT_EQ(b.use_cnt(), 2);
        EXPECT_EQ(a->value, 2);

        // Проверка ветки (this == &other)
        SharedPtr<Tracker>& a_ref = a;
        a = a_ref;
        EXPECT_EQ(a.use_cnt(), 2);

        // Проверка ветки (ref_cnt == other.ref_cnt)
        a = b;
        EXPECT_EQ(a.use_cnt(), 2);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
}

TEST(SharedPtrTest, MoveSemantics) {
    Tracker::alive_count = 0;
    {
        SharedPtr<Tracker> sp1(new Tracker(77));
        SharedPtr<Tracker> sp2(std::move(sp1));

        EXPECT_FALSE(sp1);
        EXPECT_EQ(sp1.use_cnt(), 0);
        EXPECT_TRUE(sp2);
        EXPECT_EQ(sp2.use_cnt(), 1);
        EXPECT_EQ(sp2->value, 77);

        SharedPtr<Tracker> sp3(new Tracker(88));
        sp3 = std::move(sp2);
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(sp3->value, 77);
        EXPECT_FALSE(sp2);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
}

TEST(SharedPtrTest, ResetMethod) {
    Tracker::alive_count = 0;
    {
        SharedPtr<Tracker> sp(new Tracker(10));
        sp.reset(sp.get()); // Проверка if (ptr == p) return;
        EXPECT_EQ(sp.use_cnt(), 1);
        EXPECT_EQ(Tracker::alive_count, 1);

        sp.reset(new Tracker(20));
        EXPECT_EQ(Tracker::alive_count, 1);
        EXPECT_EQ(sp->value, 20);
        EXPECT_EQ(sp.use_cnt(), 1);

        sp.reset();
        EXPECT_EQ(Tracker::alive_count, 0);
        EXPECT_FALSE(sp);
        EXPECT_EQ(sp.use_cnt(), 0);
    }
}

TEST(SharedPtrArrayTest, ArrayOperations) {
    Tracker::alive_count = 0;
    {
        SharedPtr<Tracker[]> arr1(new Tracker[3]{Tracker(1), Tracker(2), Tracker(3)});
        EXPECT_EQ(Tracker::alive_count, 3);
        EXPECT_EQ(arr1.use_cnt(), 1);

        {
            SharedPtr<Tracker[]> arr2 = arr1;
            EXPECT_EQ(arr1.use_cnt(), 2);
            EXPECT_EQ(arr2[2].value, 3);
        }

        EXPECT_EQ(arr1.use_cnt(), 1);
        EXPECT_EQ(Tracker::alive_count, 3);
    }
    EXPECT_EQ(Tracker::alive_count, 0); // Проверка вызова delete[] для массива
}


struct Shape {
    virtual ~Shape() = default;
    virtual std::string GetName() const { return "Shape"; }
};

struct Circle : public Shape {
    std::string GetName() const override { return "Circle"; }
};

struct UnrelatedClass {};


// 1. Circle является подклассом Shape -> преобразование разрешено
static_assert(std::is_constructible_v<SharedPtr<Shape>, SharedPtr<Circle>>);
static_assert(std::is_assignable_v<SharedPtr<Shape>&, SharedPtr<Circle>>);
static_assert(std::is_constructible_v<UniquePtr<Shape>, UniquePtr<Circle>&&>);

// 2. UnrelatedClass НЕ подкласс Shape -> компилятор запрещает операци
static_assert(!std::is_constructible_v<SharedPtr<Shape>, SharedPtr<UnrelatedClass>>);
static_assert(!std::is_assignable_v<SharedPtr<Shape>&, SharedPtr<UnrelatedClass>>);
static_assert(!std::is_constructible_v<UniquePtr<Shape>, UniquePtr<UnrelatedClass>&&>);

// 3. Shape НЕ подкласс Circle (Downcasting запрещен) -> компилятор запрещает операцию
static_assert(!std::is_constructible_v<SharedPtr<Circle>, SharedPtr<Shape>>);
static_assert(!std::is_assignable_v<SharedPtr<Circle>&, SharedPtr<Shape>>);


TEST(SubtypingTest, SharedPtrPolymorphism) {
    SharedPtr<Circle> circle(new Circle());
    EXPECT_EQ(circle.use_cnt(), 1);

    // Копирование производного в базовый
    SharedPtr<Shape> shape = circle;
    EXPECT_EQ(shape.use_cnt(), 2);
    EXPECT_EQ(circle.use_cnt(), 2);
    EXPECT_EQ(shape->GetName(), "Circle"); // Проверка виртуального полиморфизма

    // Присваивание производного базовому
    SharedPtr<Shape> another_shape;
    another_shape = circle;
    EXPECT_EQ(shape.use_cnt(), 3);

    // Перемещение производного в базовый
    SharedPtr<Shape> moved_shape = std::move(circle);
    EXPECT_FALSE(circle);
    EXPECT_EQ(moved_shape->GetName(), "Circle");
    EXPECT_EQ(moved_shape.use_cnt(), 3);
}

TEST(SubtypingTest, UniquePtrPolymorphism) {
    UniquePtr<Circle> circle(new Circle());

    // Перемещение производного в базовый
    UniquePtr<Shape> shape = std::move(circle);
    EXPECT_FALSE(circle);
    EXPECT_TRUE(shape);
    EXPECT_EQ(shape->GetName(), "Circle");

    // Перемещающее присваивание
    UniquePtr<Circle> circle2(new Circle());
    shape = std::move(circle2);
    EXPECT_FALSE(circle2);
    EXPECT_EQ(shape->GetName(), "Circle");
}
