include "iostream"

void add(int& a, int& b, int& out)
{
  out = a+b;
}

int main()
{
  ThreadPool pool(8);
  int out = 0;
  std::future<void > future = pool.submit(std::bind(&add, this, 1, 2, out));
  future.get();
}
