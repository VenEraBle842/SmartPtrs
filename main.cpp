#include <iostream>
#include <iomanip>
#include <string>
#include "src/SharedPtr.hpp"
#include "src/SmartArraySequence.hpp"

class Shape {
protected:
    std::string name;

public:
    explicit Shape(std::string nm) : name(std::move(nm)) {
        std::cout << "[+] Shape ctor: " << name << "\n";
    }

    virtual ~Shape() {
        std::cout << "[-] Shape dtor: " << name << "\n";
    }

    virtual void Draw() const = 0;
    virtual double Area() const = 0;

    const std::string& GetName() const noexcept { return name; }
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(std::string nm, double rad)
        : Shape(std::move(nm)), radius(rad) {
        std::cout <<"   [+] Circle ctor (r = " << radius << ")\n";
    }

    ~Circle() override {
        std::cout << "   [-] Circle dtor ";
    }

    void Draw() const override {
        std::cout << "Drawing circle " << name << " with radius " << radius
                  << " (Area: " << std::fixed << std::setprecision(2) << Area() << ")\n";
    }

    double Area() const override {
        return 3.1415926535 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width;
    double height;

public:
    Rectangle(std::string nm, double w, double h)
        : Shape(std::move(nm)), width(w), height(h) {
        std::cout << "   [+] Rectangle ctor (" << width << "x" << height << ")\n";
    }

    ~Rectangle() override {
        std::cout << "   [-] Rectangle dtor\n";
    }

    void Draw() const override {
        std::cout << "Drawing rectangle " << name << " with dimensions "
                  << width << "x" << height
                  << " (Area: " << std::fixed << std::setprecision(2) << Area() << ")\n";
    }

    double Area() const override {
        return width * height;
    }
};

int main() {
    std::cout << "\nSmartArraySequence container with polymorphic pointers\n";
    {
        std::cout << "Init SmartArraySequence<SharedPtr<Shape>> canvas\n\n";
        SmartArraySequence<SharedPtr<Shape>> canvas;

        std::cout << "Appending different shapes (Circle and Rectangle):\n";
        canvas.Append(SharedPtr<Circle>(new Circle("Canvas circle 1", 2.5)));
        canvas.Append(SharedPtr<Rectangle>(new Rectangle("Canvas rect 1", 5.0, 2.0)));
        canvas.Append(SharedPtr<Circle>(new Circle("Canvas circle 2", 3.0)));

        std::cout << "\nShapes on the canvas: " << canvas.GetLengh() << "\n\n";
        std::cout << "Iter over the canvas and draw all the shapes:\n";
        for (int i = 0; i < canvas.GetLengh(); ++i) {
            std::cout << "[" << i << "] ";
            canvas[i]->Draw();
        }

        std::cout << "\nRemoving the middle shape (rectangle):\n";
        canvas.RemoveAt(1);

        std::cout << "\nShapes remaining: " << canvas.GetLengh() << "\n\n";
        for (int i = 0; i < canvas.GetLengh(); ++i) {
            std::cout << "[" << i << "] ";
            canvas[i]->Draw();
        }

        std::cout << "\nScope exit (all remaining shapes will now be removed)...\n";
    }
    std::cout << "\nThe scenario has been succesfully completed!";

    return 0;
}
