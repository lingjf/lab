
class A
{
public:
  long long a;
  A() {}
  const char* f0() { return "fa0"; }
  virtual const char* f1() = 0;
  virtual const char* f2() { return "fa2"; }
};

class B : public A
{
public:
  long long b;
  B() {}
  const char* f0() { return "fb0"; }
  virtual const char* f1() { return "fb1"; }
  virtual const char* f2() { return "fb2"; }
};

class C : public A
{
public:
  long long c;
  C() {}
  const char* f0() { return "fc0"; }
  virtual const char* f1() { return "fc1"; }
  virtual const char* f2() { return "fc2"; }
};

class D
  : public B
  , public C
{
public:
  long long d;
  D() {}
  const char* f0() { return "fd0"; }
  virtual const char* f1() { return "fd1"; }
  virtual const char* f2() { return "fd2"; }
};

int
main(int argc, char* argv[])
{
  B b;
  C c;
  D d;

  return 0;
}