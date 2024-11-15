#include <systemc>

using namespace sc_core;

void hello1()
{
  std::cout << "Hello world using approach 1" << std::endl;
}

struct HelloWorld
  : sc_module
{

  SC_CTOR(HelloWorld)
  {
    SC_METHOD(hello2);
  }

  void hello2(void)
  {
    std::cout << "Hello world using approach 2" << std::endl;
  }
};

int sc_main(int, char*[])
{
  hello1();

  HelloWorld helloworld("helloworld");

  sc_start();

  return 0;
}
