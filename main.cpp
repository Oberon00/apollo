#include <iostream>
#include <memory>

int main(int argc, char **argv) {
    std::unique_ptr<int> p_i;
    std::cout << "Hello, world! " << p_i.get() << std::endl;
    return 0;
}
