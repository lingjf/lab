
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h> /* SymFromAddr */
#pragma comment(lib, "Dbghelp.lib")

static inline unsigned char*
follow_JMP32REL(unsigned char* target)
{
  int relative_offset;
  memcpy(reinterpret_cast<void*>(&relative_offset),
         reinterpret_cast<void*>(target + 1),
         4);
  return target + 5 + relative_offset;
}
static inline unsigned char*
follow_JMP8REL(unsigned char* target)
{
  signed char relative_offset;
  memcpy(reinterpret_cast<void*>(&relative_offset),
         reinterpret_cast<void*>(target + 1),
         1);
  return target + 2 + relative_offset;
}
static inline unsigned char*
follow_JMP32ABS(unsigned char* target)
{
  void** new_target_p;
  if (sizeof(void*) ==
      8) { // In 64-bit mode JMPs are RIP-relative, not absolute
    int target_offset;
    memcpy(reinterpret_cast<void*>(&target_offset),
           reinterpret_cast<void*>(target + 2),
           4);
    new_target_p = reinterpret_cast<void**>(target + target_offset + 6);
  } else {
    memcpy(&new_target_p, reinterpret_cast<void*>(target + 2), 4);
  }
  return reinterpret_cast<unsigned char*>(*new_target_p);
}

static inline void*
follow_jmp(void* fp, int n = 32)
{
  unsigned char* p = (unsigned char*)fp;
  while (n--) {
    if (p[0] == 0xE9) { // ASM_JMP32REL
      p = follow_JMP32REL(p);
    } else if (p[0] == 0xEB) { // ASM_JMP8REL, Visual Studio 7.1 implements
                               // new[] as an 8 bit jump to new
      p = follow_JMP8REL(p);
    } else if (p[0] == 0xFF && p[1] == 0x25) { // ASM_JMP32ABS_0 ASM_JMP32ABS_1
      p = follow_JMP32ABS(p);
    } else if (sizeof(void*) == 8 && p[0] == 0x48 && p[1] == 0xFF &&
               p[2] == 0x25) { // in Visual Studio 2012 we're seeing jump like
                               // that: rex.W jmpq *0x11d019(%rip)
      p = follow_JMP32ABS(p + 1);
    } else {
      break;
    }
  }
  return (void*)p;
}

void
get1(void* addr)
{
  char buffer[sizeof(SYMBOL_INFO) + 256];
  SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = 256;
  if (!SymFromAddr(GetCurrentProcess(), (DWORD64)(addr), 0, symbol)) {
    ::printf("err1\n");
    return;
  }

  ::printf("%s %p+%lld 0x%x\n",
           symbol->Name,
           symbol->Address,
           (long long)addr - (long long)symbol->Address,
           *(unsigned char*)symbol->Address);
}

template<typename Class, typename Signature>
struct h2_mfp;
template<typename Class, typename ReturnType, typename... Args>
struct h2_mfp<Class, ReturnType(Args...)>
{

  static void* A(ReturnType (*f)(Args...)) { return (void*)f; }

  union U
  {
    ReturnType (Class::*f)(Args...);
    void* p;
    long long v;
  };

  static void* A(ReturnType (Class::*f)(Args...))
  {
    U u{ f };
    return u.p;
  }
};

LPVOID
HookMethod(LPVOID lpVirtualTable, PVOID pHookMethod, uintptr_t dwOffset)
{
  uintptr_t dwVTable = *((uintptr_t*)lpVirtualTable);
  uintptr_t dwEntry = dwVTable + dwOffset;
  uintptr_t dwOrig = *((uintptr_t*)dwEntry);

  DWORD dwOldProtection;
  ::VirtualProtect(
    (LPVOID)dwEntry, sizeof(dwEntry), PAGE_EXECUTE_READWRITE, &dwOldProtection);

  *((uintptr_t*)dwEntry) = (uintptr_t)pHookMethod;

  ::VirtualProtect(
    (LPVOID)dwEntry, sizeof(dwEntry), dwOldProtection, &dwOldProtection);

  return (LPVOID)dwOrig;
}

// CaptureStackBackTrace(0, sizeof(frames) / sizeof(frames[0]), frames, NULL);
//     get1(frames[0]);
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

class Sum
{
public:
  virtual int add(int a, int b) { return a + b; }
  virtual int sub(int a, int b) { return a - b; }
};

typedef int(__thiscall* ADD_t)(Sum*, int, int);

int
add_FAKE(Sum* that, int a, int b)
{
  return a * b;
}

int
sub_FAKE(Sum* that, int a, int b)
{
  return a - b;
}

void
test(Sum* sum)
{
  printf("before  4 + 5 = %d\n", sum->add(4, 5));
  ADD_t oldAdd = (ADD_t)HookMethod((LPVOID)sum, (PVOID)add_FAKE, NULL);
  printf("afters  4 + 5 = %d\n", sum->add(4, 5));
  printf("origin  4 + 5 = %d\n", oldAdd(sum, 4, 5));

  printf("&Sum=%p, &Sum.add=%p\n", sum, &Sum::add );

  void *m1 = h2_mfp<Sum, int(int,int)>::A(&Sum::add);
  printf("&Sum::add: ");get1(m1);
  void *m2 = follow_jmp(m1);
  printf("follow_jmp &Sum::add: ");get1(m2);

  void *p1 = (void*)oldAdd;
  printf("vmt[0]: ");get1(p1);
  void *p2 = follow_jmp(p1);
  printf("follow_jmp vmt[0]: ");get1(p2);
}

void
test_sub(Sum* sum)
{
  printf("before  5 - 3 = %d\n", sum->sub(5,3));
  HookMethod((LPVOID)sum, (PVOID)add_FAKE, 8);
  printf("afters  5 - 3 = %d\n", sum->sub(5,3));
}

int
main(int argc, char* argv[])
{
  SymInitialize(GetCurrentProcess(), NULL, TRUE);

  Sum s2;
  test(&s2);
  test_sub(&s2);

  // Sum* s1 = new Sum();
  // test(s1);
  // delete s1;


  void *m1_f1 = h2_mfp<D, const char*()>::A(&D::f1);
  printf("&D::f1: ");get1(m1_f1);
  void *m2_f1 = follow_jmp(m1_f1);
  printf("follow_jmp &D::f1: ");get1(m2_f1);

  void *m1_f2 = h2_mfp<D, const char*()>::A(&D::f2);
  printf("&D::f2: ");get1(m1_f2);
  void *m2_f2 = follow_jmp(m1_f2);
  printf("follow_jmp &D::f2: ");get1(m2_f2);

  return 0;
}